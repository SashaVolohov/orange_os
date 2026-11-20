#include "core.h"
#include "pci.h"
#include "xhci.h"

int man_begin = 0;

xhci_caps_t caps;

xhci_trb_t*         event_ring;
xhci_erst_entry_t*  erst;
uint8_t             current_cycle = 1;
uint64_t            dequeue_index = 0;

xhci_trb_t*         cmd_ring = 0;
uint64_t            cmd_enqueue_idx = 0;
uint8_t             cmd_cycle = 1;

uintptr_t xhci_addr;

uint8_t usb_device_slot_id = 0;

uint8_t usb_bulk_in_burst = 0;
uint8_t usb_bulk_out_burst = 0;

uint64_t* dcbaa_virt = NULL;
uint64_t  dcbaa_phys = 0;

uint64_t cmd_enable_slot_phys_addr = 0;
uint64_t cmd_address_device_phys_addr = 0;
uint64_t cmd_configure_ep_phys_addr = 0;

uint64_t trb_get_descriptor_status_phys = 0;
uint64_t trb_set_configuration_status_phys = 0;
uint64_t trb_get_config_status_phys = 0;
uint64_t trb_get_full_config_status_phys = 0;

uint64_t                        desc_buffer_phys = 0;

uint8_t usb_bulk_in_endpoint = 0;
uint8_t usb_bulk_out_endpoint = 0;
uint16_t usb_bulk_max_packet_size = 0;

uint64_t disk_buffer_phys = 0;
uint8_t* disk_buffer_virt = NULL;

extern uint32_t* fb_back_buffer;

usb_device_t usb_devices[MAX_USB_DEVICES];

volatile uint64_t last_completed_cmd_trb = 0;
volatile uint64_t last_completed_cmd_cc = 0;

int current_enumerating_slot = -1;

void intel_xhci_route_ports(pci_device_t* xhci) {
    uint32_t id = pci_read_config_32(xhci->bus, xhci->device, xhci->function, 0x00);
    if ((id & 0xFFFF) != 0x8086) {
        return;
    }

    uint32_t usb2_mask = pci_read_config_32(xhci->bus, xhci->device, xhci->function, 0xD4);
    uint32_t usb3_mask = pci_read_config_32(xhci->bus, xhci->device, xhci->function, 0xDC);

    pci_write_config_32(xhci->bus, xhci->device, xhci->function, 0xD8, usb3_mask);
    pci_write_config_32(xhci->bus, xhci->device, xhci->function, 0xD0, usb2_mask);
}

void xhci_bios_handoff(uintptr_t xhci_base) {
    uint32_t hccparams1 = *(volatile uint32_t*)(xhci_base + 0x10);

    uint32_t ext_offset = ((hccparams1 >> 16) & 0xFFFF) * 4;
    if (ext_offset == 0) return;

    uintptr_t ext_ptr = xhci_base + ext_offset;
    while (1) {
        uint32_t cap = *(volatile uint32_t*)ext_ptr;
        uint8_t cap_id = cap & 0xFF;
        uint8_t next = (cap >> 8) & 0xFF;

        if (cap_id == 1) {
            *(volatile uint32_t*)ext_ptr |= (1 << 24);
            int timeout = 1000;
            while ((*(volatile uint32_t*)ext_ptr & (1 << 16)) && --timeout > 0) {
                sleep_ms(1);
            }
            *(volatile uint32_t*)(ext_ptr + 4) &= ~0xE0000000;
            break;
        }
        if (next == 0) break;
        ext_ptr += next * 4;
    }
}

pci_device_t* find_xhci_controller() {
    for (uint64_t i = 0; i < total_devices; i++) {
        pci_device_t* dev = &pci_devices[i];
        
        if (dev->class_code == PCI_CLASS_SERIAL_BUS &&
            dev->subclass_code == PCI_SUBCLASS_USB &&
            dev->prog_if == PCI_PROGIF_XHCI) 
        {
            return dev;
        }
    }
    return 0;
}

uint64_t get_xhci_base_address(pci_device_t* xhci) {
    uint32_t bar0 = pci_read_config_32(xhci->bus, xhci->device, xhci->function, PCI_CONFIG_BAR0);
    
    if (bar0 & 1) {
        return 0; 
    }

    uint64_t physical_address = 0;

    if ((bar0 & 0x06) == BAR_MEM_64BIT) {
        uint32_t bar1 = pci_read_config_32(xhci->bus, xhci->device, xhci->function, PCI_CONFIG_BAR1);
        
        physical_address = ((uint64_t)bar1 << 32) | (bar0 & 0xFFFFFFF0);
    } else {
        physical_address = bar0 & 0xFFFFFFF0;
    }

    uint64_t phys_page = (uint64_t)physical_address & ~0xFFF;
    for (uint64_t offset = 0; offset < 0x10000; offset += 0x1000) {
        map_page(phys_page + offset, phys_page + offset, 0x1B);
    }

    return physical_address;
}

void enable_xhci_pci_bus(pci_device_t* xhci) {
    uint32_t command = pci_read_config_32(xhci->bus, xhci->device, xhci->function, PCI_CONFIG_COMMAND);
    
    command |= (1 << 1) | (1 << 2); 
    
    pci_write_config_32(xhci->bus, xhci->device, xhci->function, PCI_CONFIG_COMMAND, command);
}

void xhci_read_capabilities(uintptr_t xhci_base) {
    caps.cap_length = *(volatile uint8_t*)(xhci_base + XHCI_CAP_CAPLENGTH);
    caps.op_regs_offset = caps.cap_length;

    caps.hci_version = *(volatile uint16_t*)(xhci_base + XHCI_CAP_HCIVERSION);

    uint32_t hcsparams1 = *(volatile uint32_t*)(xhci_base + XHCI_CAP_HCSPARAMS1);
    caps.max_slots = hcsparams1 & 0xFF;
    caps.max_ports = (hcsparams1 >> 24) & 0xFF;

    caps.doorbell_regs_offset = *(volatile uint32_t*)(xhci_base + XHCI_CAP_DBOFF) & ~0x3;
    caps.runtime_regs_offset   = *(volatile uint32_t*)(xhci_base + XHCI_CAP_RTSOFF) & ~0x1F;
}

void xhci_reset_controller(uintptr_t xhci_base) {
    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    *(volatile uint32_t*)(op_base + XHCI_OP_USBCMD) &= ~1;

    while ((*(volatile uint32_t*)(op_base + XHCI_OP_USBSTS) & 1) == 0);

    *(volatile uint32_t*)(op_base + XHCI_OP_USBCMD) |= CMD_RESET;

    while (*(volatile uint32_t*)(op_base + XHCI_OP_USBCMD) & CMD_RESET);

    while (*(volatile uint32_t*)(op_base + XHCI_OP_USBSTS) & STS_CNR);
    
}

void xhci_init_event_ring(uintptr_t xhci_base) {
    uintptr_t rt_base = xhci_base + caps.runtime_regs_offset;

    uint64_t event_ring_phys = alloc_page();
    uint64_t erst_phys       = alloc_page();

    if(event_ring_phys == 0 || erst_phys == 0) {
        KeBugCheck(0x20);
    }

    event_ring = (xhci_trb_t*)event_ring_phys;
    erst       = (xhci_erst_entry_t*)erst_phys;

    for(int i = 0; i < EVENT_RING_SIZE; i++) {
        event_ring[i].parameter_lo = event_ring[i].parameter_hi = 0;
        event_ring[i].status = event_ring[i].control = 0;
    }

    erst[0].seg_base_addr = event_ring_phys;
    erst[0].seg_size      = EVENT_RING_SIZE;
    erst[0].reserved      = 0;

    uint64_t phys_page = (uint64_t)rt_base & ~0xFFF;
    map_page(phys_page, phys_page, 0x1B);

    uintptr_t db_base = (uint64_t)xhci_base + (uint64_t)caps.doorbell_regs_offset;

    phys_page = (uint64_t)db_base & ~0xFFF;
    map_page(phys_page, phys_page, 0x1B);

    *(volatile uint32_t*)(rt_base + XHCI_INT_ERSTSZ) = 1;

    *(volatile uint64_t*)(rt_base + XHCI_INT_ERSTBA) = erst_phys;

    *(volatile uint64_t*)(rt_base + XHCI_INT_ERDP) = event_ring_phys;

    *(volatile uint32_t*)(rt_base + XHCI_INT_IMAN) |= (1 << 0) | (1 << 1);
}

void xhci_check_events(uintptr_t xhci_base) {
    uintptr_t rt_base = xhci_base + caps.runtime_regs_offset;
    
    while (1) {
        xhci_trb_t* trb = &event_ring[dequeue_index];

        asm volatile("clflush %0" : : "m"(*trb));
        asm volatile("mfence" ::: "memory");

        uint8_t trb_cycle = trb->control & 1;

        if (trb_cycle != current_cycle) {
            break;
        }

        uint8_t trb_type = (trb->control >> 10) & 0x3F;

        if (trb_type == TRB_TYPE_CHANGE_SLOT) {
            //uint8_t port_id = (trb->parameter_lo >> 24) & 0xFF;

            // Тут в будущем дописать что типо устройство чё-то сделало

        }
        else if (trb_type == TRB_TYPE_COMMAND_COMPLETION) {
            uint64_t cmd_trb_phys = ((uint64_t)trb->parameter_hi << 32) | trb->parameter_lo;
            uint8_t completion_code = (trb->status >> 24) & 0xFF;
            uint8_t slot_id = (trb->control >> 24) & 0xFF;

            last_completed_cmd_trb = cmd_trb_phys;
            last_completed_cmd_cc  = completion_code;

            if (completion_code == XHCI_TRB_SUCCESS) {
                xhci_trb_t* actual_cmd = (xhci_trb_t*)cmd_trb_phys;
                uint8_t cmd_type = (actual_cmd->control >> 10) & 0x3F;

                if (cmd_type == XHCI_CMD_ENABLE_SLOT) {
                    usb_device_slot_id = slot_id;
                }
                else if (cmd_type == 11) {
                    usb_devices[slot_id].device_addresed = 1;
                }
                else if (cmd_type == XHCI_CMD_CONFIGURE_ENDPOINT) {
                    usb_devices[slot_id].device_opened = 1;
                }

            } else {

                draw_rect(15, 230, 150, 150, 0x00, 0x00, 0x00, 0xFF);
            }

        }
        else if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
            uint64_t event_trb_phys = ((uint64_t)trb->parameter_hi << 32) | trb->parameter_lo;
            uint8_t completion_code = (trb->status >> 24) & 0xFF;
            uint8_t event_slot_id = (trb->control >> 24) & 0xFF;

            usb_device_descriptor_t* desc_buffer_virt = (usb_device_descriptor_t*)usb_devices[event_slot_id].desc_buffer_virt;

            asm volatile("clflush %0" : : "m"(*(volatile uint32_t*)desc_buffer_virt));

            last_completed_cmd_trb = event_trb_phys;
            last_completed_cmd_cc  = completion_code;

            if (completion_code == XHCI_TRB_SUCCESS || completion_code == 13) {

                usb_device_t* ev_dev = &usb_devices[event_slot_id];

                if (event_trb_phys == trb_get_descriptor_status_phys) {

                    asm volatile("clflush %0" : : "m"(*(volatile uint32_t*)desc_buffer_virt));

                    usb_devices[event_slot_id].config_returned = 1;
                }
                
                else if (event_trb_phys == trb_set_configuration_status_phys) {
                    usb_devices[event_slot_id].config_configurated = 1;
                }
                
                else if (event_trb_phys == trb_get_config_status_phys) {
                    asm volatile("clflush %0" : : "m"(*(volatile uint32_t*)desc_buffer_virt));

                    usb_devices[event_slot_id].cfg = kmalloc(sizeof(usb_config_descriptor_t));
                    memcpy(usb_devices[event_slot_id].cfg, desc_buffer_virt, sizeof(usb_config_descriptor_t));
                }

                else if (event_trb_phys == trb_get_full_config_status_phys) {

                    volatile uint8_t* cache_ptr = (volatile uint8_t*)desc_buffer_virt;
                    for (int i = 0; i < 256; i += 64) {
                        asm volatile("clflush %0" : : "m"(*(cache_ptr + i)));
                    }
                    
                    uint8_t* raw_bytes = (uint8_t*)desc_buffer_virt;
                    uint32_t offset = 0;
                    uint32_t total_len = usb_devices[event_slot_id].cfg->wTotalLength;

                    usb_device_t* dev = &usb_devices[current_enumerating_slot]; 
                    dev->slot_id = current_enumerating_slot;
                    dev->is_connected = 1;
                    dev->type = USB_DEV_UNKNOWN;

                    usb_endpoint_descriptor_t* last_ep = NULL;
                    int8_t last_ep_is_in = 0;

                    uint8_t in_valid_msc_interface = 0;

                    while (offset < total_len) {
                        uint8_t desc_len  = raw_bytes[offset + 0];
                        uint8_t desc_type = raw_bytes[offset + 1];

                        if (desc_len == 0) break;

                        if (desc_type == 4) {
                            uint8_t alt_setting        = raw_bytes[offset + 3];
                            uint8_t interface_class    = raw_bytes[offset + 5];
                            uint8_t interface_subclass = raw_bytes[offset + 6];
                            uint8_t interface_protocol = raw_bytes[offset + 7];

                            if(alt_setting == 0) {

                                if (interface_class == 0x08 &&
                                    interface_subclass == 0x06 &&
                                    interface_protocol == 0x50) {

                                    dev->type = USB_DEV_MASS_STORAGE;
                                    dev->interface_number = raw_bytes[offset + 2];
                                } else if (interface_class == 0x03 && interface_subclass == 0x01 && interface_protocol == 0x02) {
                                    dev->type = USB_DEV_HID_MOUSE;
                                } else if (interface_class == 0x03 && interface_subclass == 0x01 && interface_protocol == 0x01) {
                                    dev->type = USB_DEV_HID_KEYBOARD;
                                } else {
                                    dev->type = USB_DEV_UNKNOWN;
                                }

                                in_valid_msc_interface = 1;

                            } else {
                                in_valid_msc_interface = 0;
                            }
                        }

                        else if (desc_type == 5 && in_valid_msc_interface) { // Endpoint Descriptor
                            usb_endpoint_descriptor_t* ep = (usb_endpoint_descriptor_t*)&raw_bytes[offset];
                            
                            if ((ep->bmAttributes & 0x03) == 0x02) {
                                last_ep = ep;

                                if (ep->bEndpointAddress & 0x80) {
                                    dev->bulk_in_ep = ep->bEndpointAddress & 0x0F;
                                    dev->max_packet_size = ep->wMaxPacketSize;
                                    last_ep_is_in = 1;
                                } else {
                                    dev->bulk_out_ep = ep->bEndpointAddress & 0x0F;
                                    last_ep_is_in = 0;
                                }
                            } else {
                                last_ep = NULL;
                            }
                        }
                        
                        else if (desc_type == 48) {
                            if (last_ep != NULL) {
                                uint8_t max_burst = raw_bytes[offset + 2]; 

                                if (last_ep_is_in) {
                                    dev->bulk_in_burst = max_burst;
                                } else {
                                    dev->bulk_out_burst = max_burst;
                                }
                                
                                last_ep = NULL;
                            }
                        }

                        offset += desc_len;
                    }
                    
                }
                else if (ev_dev->expected_csw_trb_phys != 0 && event_trb_phys == ev_dev->expected_csw_trb_phys) {

                    ev_dev->expected_csw_trb_phys = 0;
                    ev_dev->io_completed = 1;
            
                    volatile uint8_t* cache_ptr = (volatile uint8_t*)disk_buffer_virt;
                    for (int i = 0; i < 512; i += 64) {
                        asm volatile("clflush %0" : : "m"(*(cache_ptr + i)));
                    }
                            
                }

                volatile uint8_t* ptr = (volatile uint8_t*)desc_buffer_virt;
                for (int i = 0; i < 18; i += 64) {
                    asm volatile("clflush %0" : : "m"(*(ptr + i)));
                }

            } else {

                // Надо обработать ошибку да

            }

        } 

        dequeue_index++;
        if (dequeue_index >= EVENT_RING_SIZE) {
            dequeue_index = 0;
            current_cycle = !current_cycle;
        }

        uint64_t next_erdp_phys = erst->seg_base_addr + (dequeue_index * sizeof(xhci_trb_t));
        *(volatile uint64_t*)(rt_base + XHCI_INT_ERDP) = next_erdp_phys | (1 << 3);
    }
}

void xhci_start_controller(uintptr_t xhci_base) {
    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    *(volatile uint32_t*)(op_base + XHCI_OP_USBCMD) |= XHCI_CMD_RS;

    while (*(volatile uint32_t*)(op_base + XHCI_OP_USBSTS) & 1);

    xhci_addr = xhci_base;
}

void xhci_probe_ports(uintptr_t xhci_base) {

    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    for (uint32_t i = 0; i < caps.max_ports; i++) {

        uintptr_t portsc_reg = op_base + XHCI_PORTSC_BASE + (i * 0x10);
        uint32_t status = *(volatile uint32_t*)portsc_reg;
        *(volatile uint32_t*)portsc_reg = (status & ~PORTSC_RWC_MASK) | (1 << 9);
        sleep_ms(20);

        if (status & PORTSC_CCS) {
            uint8_t initial_speed = (status >> 10) & 0x0F;

            if (initial_speed < 4 && !(status & PORTSC_PED)) {
                uint32_t reset_cmd = (status & ~PORTSC_PRESERVE_MASK) | PORTSC_CSC | PORTSC_PR;
                *(volatile uint32_t*)portsc_reg = reset_cmd;

                sleep_ms(60);

                int reset_timeout = 500;
                while ((*(volatile uint32_t*)portsc_reg & PORTSC_PR) && --reset_timeout > 0) {
                    sleep_ms(1);
                }
                sleep_ms(20);

                uint32_t post_reset = *(volatile uint32_t*)portsc_reg;
                *(volatile uint32_t*)portsc_reg = (post_reset & ~PORTSC_PRESERVE_MASK) | PORTSC_PRC;
                sleep_ms(10);
            } 
            else {
                uint32_t clear_flags = (status & ~PORTSC_PRESERVE_MASK) | (status & PORTSC_RWC_MASK);
                *(volatile uint32_t*)portsc_reg = clear_flags;
                sleep_ms(20);
            }

            uint32_t final_check = *(volatile uint32_t*)portsc_reg;
            uint8_t final_speed  = (final_check >> 10) & 0x0F;
            uint8_t link_state   = (final_check >> 5) & 0x0F;

            if ((final_check & PORTSC_PED) || (final_speed >= 4 && link_state == 0)) {
                
                usb_device_slot_id = 0;
                xhci_request_slot();

                if(usb_device_slot_id != 0) {

                    if ((final_check & PORTSC_PED) || (final_speed == 4 && link_state == 0)) {

                        xhci_setup_device(usb_device_slot_id, i+1, final_speed);

                        if(usb_devices[usb_device_slot_id].device_addresed != 0) {

                            xhci_get_device_descriptor(xhci_addr, usb_device_slot_id);
                            usb_device_t* dev = &usb_devices[usb_device_slot_id];

                            sleep_ms(1000);
                            xhci_check_events(xhci_addr);

                            asm volatile("mfence" ::: "memory");
                            asm volatile("clflush %0" : : "m"(*(dev->desc_buffer_virt)));

                            if(usb_devices[usb_device_slot_id].config_returned != 0) {

                                xhci_set_configuration(xhci_base, usb_device_slot_id, 1);
                                sleep_ms(1000);
                                xhci_check_events(xhci_addr);

                                if(usb_devices[usb_device_slot_id].config_configurated != 0) {

                                    xhci_get_configuration_descriptor(xhci_base, usb_device_slot_id);
                                    sleep_ms(1000);
                                    xhci_check_events(xhci_addr);

                                    if(usb_devices[usb_device_slot_id].cfg != 0) {

                                        current_enumerating_slot = usb_device_slot_id;
                                        xhci_get_full_configuration_descriptor(xhci_base, usb_device_slot_id, usb_devices[usb_device_slot_id].cfg->wTotalLength);
                                        sleep_ms(1000);
                                        xhci_check_events(xhci_addr);

                                        if(usb_devices[usb_device_slot_id].type == USB_DEV_MASS_STORAGE && usb_devices[usb_device_slot_id].max_packet_size != 0) {

                                            usb_bulk_in_endpoint = usb_devices[usb_device_slot_id].bulk_in_ep;
                                            usb_bulk_out_endpoint = usb_devices[usb_device_slot_id].bulk_out_ep;
                                            usb_bulk_max_packet_size = usb_devices[usb_device_slot_id].max_packet_size;
                                            usb_bulk_in_burst = usb_devices[usb_device_slot_id].bulk_in_burst;
                                            usb_bulk_out_burst = usb_devices[usb_device_slot_id].bulk_out_burst;

                                            xhci_configure_bulk_endpoints(usb_device_slot_id, usb_bulk_in_endpoint, usb_bulk_out_endpoint, usb_bulk_max_packet_size);

                                            if(usb_devices[usb_device_slot_id].device_opened != 0) {
                                                usb_devices[usb_device_slot_id].ready_for_storage = 1;
                                            }

                                        }

                                    }

                                }
                                

                            }
                        }

                    }

                }

            } else {

                // Log Error
                
            }
        }
    }

}

void xhci_init_command_ring(uintptr_t xhci_base) {
    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    uintptr_t cmd_ring_phys = alloc_page();
    cmd_ring      = (xhci_trb_t*)cmd_ring_phys;

    for (int i = 0; i < COMMAND_RING_SIZE; i++) {
        cmd_ring[i].parameter_lo = cmd_ring[i].parameter_hi = 0;
        cmd_ring[i].status = cmd_ring[i].control = 0;
    }

    xhci_trb_t* link = &cmd_ring[COMMAND_RING_SIZE - 1];

    link->parameter_lo = (uint32_t)cmd_ring_phys;
    link->parameter_hi = (uint32_t)(cmd_ring_phys >> 32);
    link->status = 0;

    link->control =
        (6 << 10) |
        (1 << 1)  |
        1;

    uint64_t crcr_val = cmd_ring_phys | (1 << 0);
    *(volatile uint64_t*)(op_base + XHCI_OP_CRCR) = crcr_val;
}

uint64_t xhci_post_command(uint32_t p_lo, uint32_t p_hi, uint32_t status, uint32_t control_type, uint32_t flags) {
    uintptr_t db_base = xhci_addr + caps.doorbell_regs_offset;
    uint64_t target_trb_phys = (uint64_t)cmd_ring + (cmd_enqueue_idx * sizeof(xhci_trb_t));

    xhci_trb_t* trb = &cmd_ring[cmd_enqueue_idx];
    trb->parameter_lo = p_lo;
    trb->parameter_hi = p_hi;
    trb->status       = status;
    trb->control      = (control_type << 10) | flags | cmd_cycle;

    asm volatile("mfence" ::: "memory");

    cmd_enqueue_idx++;
    if (cmd_enqueue_idx >= COMMAND_RING_SIZE) {
        cmd_enqueue_idx = 0;
        cmd_cycle = !cmd_cycle;
    }

    asm volatile("mfence" ::: "memory");

    *(volatile uint32_t*)(db_base + 0) = 0; 

    asm volatile("mfence" ::: "memory");

    int timeout_ms = 3000;

    while (timeout_ms-- > 0) {
        xhci_check_events(xhci_addr);

        if (last_completed_cmd_trb == target_trb_phys) {
            break;
        }

        sleep_ms(1);
    }

    return target_trb_phys;
}

void xhci_request_slot() {
    cmd_enable_slot_phys_addr = xhci_post_command(0, 0, 0, XHCI_CMD_ENABLE_SLOT, 0);
}

void xhci_init_dcbaa(uintptr_t xhci_base) {
    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    uint32_t dcbaa_size = (caps.max_slots + 1) * sizeof(uint64_t);
    dcbaa_phys = alloc_page();
    while (dcbaa_size > 4096) {
        dcbaa_size -= 4096;
        alloc_page();
    }
    dcbaa_virt = (uint64_t*)dcbaa_phys;

    for (uint32_t i = 0; i <= caps.max_slots; i++) {
        dcbaa_virt[i] = 0;
    }

    uint32_t hcsparams2 = *(volatile uint32_t*)(xhci_base + 0x08);
    uint32_t max_sp_hi = (hcsparams2 >> 21) & 0x1F;
    uint32_t max_sp_lo = (hcsparams2 >> 27) & 0x1F;
    uint32_t max_scratchpad = (max_sp_hi << 5) | max_sp_lo;

    if (max_scratchpad > 0) {
        uint64_t sp_array_phys = alloc_page();
        uint64_t* sp_array_virt = (uint64_t*)sp_array_phys;

        for (uint32_t i = 0; i < max_scratchpad; i++) {
            uint64_t page_phys = alloc_page();
            uint8_t* p = (uint8_t*)page_phys;
            for (int b = 0; b < 4096; b++) p[b] = 0;
            for (int b = 0; b < 4096; b += 64) {
                asm volatile("clflush %0" : : "m"(p[b]));
            }
            sp_array_virt[i] = page_phys;
        }

        for (int b = 0; b < 4096; b += 64) {
            asm volatile("clflush %0" : : "m"(((uint8_t*)sp_array_virt)[b]));
        }

        dcbaa_virt[0] = sp_array_phys;
    }

    for (int b = 0; b < 4096; b += 64) {
        asm volatile("clflush %0" : : "m"(((uint8_t*)dcbaa_virt)[b]));
    }
    asm volatile("mfence" ::: "memory");

    *(volatile uint64_t*)(op_base + XHCI_OP_DCBAAP) = dcbaa_phys;
}

int setup_y = 0;

void xhci_setup_device(uint8_t slot_id, uint8_t port_id, uint8_t speed) {
    usb_device_t* dev = &usb_devices[slot_id];
    dev->slot_id = slot_id;
    dev->is_connected = 1;
    dev->type = USB_DEV_UNKNOWN;

    dev->ep0_ring_phys = alloc_page();
    dev->ep0_ring_virt = (uint32_t*)dev->ep0_ring_phys;
    dev->ep0_idx = 0;

    for (int i = 0; i < 4096 / 4; i++) {
        dev->ep0_ring_virt[i] = 0;
    }

    uint32_t hccparams1 = *(volatile uint32_t*)(xhci_addr + 0x10);
    int ctx_size = (hccparams1 & (1 << 2)) ? 64 : 32;
    int ctx_shift = ctx_size / 4; 

    uint64_t my_input_context_phys = alloc_page();
    uint32_t* input_ctx = (uint32_t*)my_input_context_phys;
    for (int i = 0; i < 4096 / 4; i++) input_ctx[i] = 0;

    uint64_t my_dev_context_phys = alloc_page();
    uint32_t* dev_ctx = (uint32_t*)my_dev_context_phys;
    for (int i = 0; i < 4096 / 4; i++) dev_ctx[i] = 0;

    for (int i = 0; i < 4096 / 4; i++) {
        input_ctx[i] = 0;
        dev_ctx[i] = 0;
    }

    dcbaa_virt[slot_id] = my_dev_context_phys;

    uint32_t* dev_ctx_words = (uint32_t*)my_dev_context_phys;
    for (int i = 0; i < 1024; i++) dev_ctx_words[i] = 0;
    asm volatile("clflush %0" : : "m"(*dev_ctx_words));

    uint32_t ctrl_offset = 0;
    uint32_t slot_offset = ctx_shift;
    uint32_t ep0_offset  = 2 * ctx_shift;

    for (int i = 0; i < 4096 / 4; i++) input_ctx[i] = 0;

    input_ctx[ctrl_offset + 0] = 0;
    input_ctx[ctrl_offset + 1] = (1 << 0) | (1 << 1);

    input_ctx[slot_offset + 0] = (1 << 27) | ((uint32_t)speed << 20);
    input_ctx[slot_offset + 1] = ((uint32_t)port_id << 16);
    input_ctx[slot_offset + 2] = 0;

    uint32_t mps = 64;
    if (speed == 1 || speed == 2) mps = 8;
    else if (speed == 4) mps = 512;

    input_ctx[ep0_offset + 0] = 0;
    input_ctx[ep0_offset + 1] = (3 << 1) | (4 << 3) | (mps << 16);
    input_ctx[ep0_offset + 2] = (uint32_t)(dev->ep0_ring_phys) | 1;
    input_ctx[ep0_offset + 3] = (uint32_t)(dev->ep0_ring_phys >> 32);
    input_ctx[ep0_offset + 4] = 8;

    volatile uint8_t* flush_ctx = (volatile uint8_t*)input_ctx;
    for (int i = 0; i < 512; i += 64) {
        asm volatile("clflush %0" : : "m"(*(flush_ctx + i)));
    }

    volatile uint8_t* cache_flush_ptr = (volatile uint8_t*)my_input_context_phys;
    for (int i = 0; i < 2112; i += 64) {
        asm volatile("clflush %0" : : "m"(*(cache_flush_ptr + i)));
    }

    uint32_t cmd_flags = ((uint32_t)slot_id << 24);
    
    cmd_address_device_phys_addr = xhci_post_command(
        (uint32_t)my_input_context_phys, 
        (uint32_t)(my_input_context_phys >> 32), 
        0, 
        XHCI_CMD_ADDRESS_DEVICE,
        cmd_flags
    );
}

void xhci_finalize_controller_init(uintptr_t xhci_base) {
    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    uint32_t config = *(volatile uint32_t*)(op_base + XHCI_OP_CONFIG);

    config &= ~0xFF;
    config |= (caps.max_slots & 0xFF);

    *(volatile uint32_t*)(op_base + XHCI_OP_CONFIG) = config;
}

void xhci_get_device_descriptor(uintptr_t xhci_base, uint8_t slot_id) {
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    usb_device_t* dev = &usb_devices[slot_id];

    if (!dev->desc_buffer_phys) {
        dev->desc_buffer_phys = alloc_page();
        dev->desc_buffer_virt = (uint32_t*)dev->desc_buffer_phys;
    }


    desc_buffer_phys = dev->desc_buffer_phys; 


    for(int i = 0; i < 1024; i++) dev->desc_buffer_virt[i] = 0;

    volatile uint32_t* ep0_dw = (volatile uint32_t*)dev->ep0_ring_virt;
    int idx = dev->ep0_idx * 4; 
    uint32_t ep0_cycle = 1;

    for(int i = 0; i < 16; i++) ep0_dw[idx + i] = 0;

    ep0_dw[idx + 0] = 0x01000680; 
    ep0_dw[idx + 1] = 0x00120000;
    ep0_dw[idx + 2] = 8;          
    ep0_dw[idx + 3] = (2 << 10) | (3 << 16) | (1 << 6) | (ep0_cycle & 1); 

    ep0_dw[idx + 4] = (uint32_t)desc_buffer_phys;
    ep0_dw[idx + 5] = (uint32_t)(desc_buffer_phys >> 32);
    ep0_dw[idx + 6] = 18;
    ep0_dw[idx + 7] = (3 << 10) | (1 << 16) | (1 << 4) | (ep0_cycle & 1);

    ep0_dw[idx + 8] = 0;
    ep0_dw[idx + 9] = 0;
    ep0_dw[idx + 10] = 0;
    ep0_dw[idx + 11] = (4 << 10) | (1 << 5) | (ep0_cycle & 1);

    asm volatile("clflush %0" : : "m"(ep0_dw[idx + 0]));
    asm volatile("clflush %0" : : "m"(ep0_dw[idx + 4]));
    asm volatile("clflush %0" : : "m"(ep0_dw[idx + 8]));

    trb_get_descriptor_status_phys = dev->ep0_ring_phys + ((dev->ep0_idx + 2) * 16);
    dev->ep0_idx = (dev->ep0_idx + 3) % 64;

    asm volatile("mfence" ::: "memory");
    asm volatile("clflush %0" : : "m"(*(dev->desc_buffer_virt)));

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1;

    volatile uint8_t* flush_ptr = (volatile uint8_t*)dev->desc_buffer_virt;
    for (int i = 0; i < 128; i += 64) {
        asm volatile("clflush %0" : : "m"(*(flush_ptr + i)));
    }

    for (volatile int delay = 0; delay < 5000000; delay++);
    asm volatile("clflush %0" : : "m"(*(dev->desc_buffer_virt)));
}

uint64_t pack_set_config_packet(uint8_t config_value) {
    
    uint64_t packet = 0;
    packet |= (uint64_t)0x00 << 0;
    packet |= (uint64_t)USB_REQ_SET_CONFIGURATION << 8;
    packet |= (uint64_t)config_value << 16;
    packet |= (uint64_t)0x00 << 24;
    return packet;
}

void xhci_set_configuration(uintptr_t xhci_base, uint8_t slot_id, uint8_t config_value) {
    usb_device_t* dev = &usb_devices[slot_id];
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    volatile uint32_t* ep0_dw = (volatile uint32_t*)dev->ep0_ring_virt;

    uint8_t ep0_cycle = 1;

    uint64_t setup_packet = pack_set_config_packet(config_value);
    uint32_t* setup_dw = (uint32_t*)&setup_packet;

    int idx = dev->ep0_idx * 4;

    ep0_dw[idx + 0] = setup_dw[0];
    ep0_dw[idx + 1] = setup_dw[1];
    ep0_dw[idx + 2] = 8;
    ep0_dw[idx + 3] = (2 << 10) | (0 << 16) | (1 << 6) | ep0_cycle;

    ep0_dw[idx + 4] = 0;
    ep0_dw[idx + 5] = 0;
    ep0_dw[idx + 6] = 0;
    ep0_dw[idx + 7] = (4 << 10) | (1 << 16) | (1 << 5) | ep0_cycle;

    trb_set_configuration_status_phys = dev->ep0_ring_phys + ((dev->ep0_idx + 1) * sizeof(xhci_trb_t));

    dev->ep0_idx = (dev->ep0_idx + 2) % 64;

    asm volatile("clflush %0" : : "m"(ep0_dw[idx + 0]));
    asm volatile("clflush %0" : : "m"(ep0_dw[idx + 4]));
    asm volatile("mfence" ::: "memory");

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
}

void xhci_get_configuration_descriptor(uintptr_t xhci_base, uint8_t slot_id) {
    usb_device_t* dev = &usb_devices[slot_id];
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    uint32_t* ep0_dw = (uint32_t*)dev->ep0_ring_virt;
    uint8_t ep0_cycle = 1;

    uint64_t setup_packet = 0;
    setup_packet |= (uint64_t)0x80 << 0;
    setup_packet |= (uint64_t)0x06 << 8;
    setup_packet |= (uint64_t)0x0200 << 16;
    setup_packet |= (uint64_t)0x0000 << 32;
    setup_packet |= (uint64_t)9 << 48;

    uint32_t* setup_dw = (uint32_t*)&setup_packet;

    int idx = dev->ep0_idx * 4;

    trb_get_config_status_phys = dev->ep0_ring_phys + (dev->ep0_idx + 2) * sizeof(xhci_trb_t);

    ep0_dw[idx + 0] = setup_dw[0];
    ep0_dw[idx + 1] = setup_dw[1];
    ep0_dw[idx + 2] = 8;
    ep0_dw[idx + 3] = (2 << 10) | (2 << 16) | (1 << 6) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = (uint32_t)dev->desc_buffer_phys;         
    ep0_dw[idx + 1] = (uint32_t)(dev->desc_buffer_phys >> 32);  
    ep0_dw[idx + 2] = 9;
    ep0_dw[idx + 3] = (3 << 10) | (1 << 16) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = 0;
    ep0_dw[idx + 1] = 0;
    ep0_dw[idx + 2] = 0;
    ep0_dw[idx + 3] = (4 << 10) | (0 << 16) | (1 << 5) | ep0_cycle;

    dev->ep0_idx = (dev->ep0_idx + 3) % 64;

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
}

void xhci_get_full_configuration_descriptor(uintptr_t xhci_base, uint8_t slot_id, uint16_t total_length) {
    usb_device_t* dev = &usb_devices[slot_id];
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    uint32_t* ep0_dw = (uint32_t*)dev->ep0_ring_virt;
    uint8_t ep0_cycle = 1;

    uint64_t setup_packet = 0;
    setup_packet |= (uint64_t)0x80 << 0;   
    setup_packet |= (uint64_t)0x06 << 8;   
    setup_packet |= (uint64_t)0x0200 << 16; 
    setup_packet |= (uint64_t)0x0000 << 32; 
    setup_packet |= (uint64_t)total_length << 48;

    uint32_t* setup_dw = (uint32_t*)&setup_packet;

    int idx = dev->ep0_idx * 4;

    trb_get_full_config_status_phys = dev->ep0_ring_phys + (dev->ep0_idx + 2) * sizeof(xhci_trb_t);

    ep0_dw[idx + 0] = setup_dw[0];
    ep0_dw[idx + 1] = setup_dw[1];
    ep0_dw[idx + 2] = 8;
    ep0_dw[idx + 3] = (2 << 10) | (2 << 16) | (1 << 6) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = (uint32_t)desc_buffer_phys;         
    ep0_dw[idx + 1] = (uint32_t)(desc_buffer_phys >> 32);  
    ep0_dw[idx + 2] = total_length;
    
    ep0_dw[idx + 3] = (3 << 10) | (1 << 16) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = 0;
    ep0_dw[idx + 1] = 0;
    ep0_dw[idx + 2] = 0;
    ep0_dw[idx + 3] = (4 << 10) | (0 << 16) | (1 << 5) | ep0_cycle;

    dev->ep0_idx = (dev->ep0_idx + 3) % 64;

    memset(usb_devices[slot_id].desc_buffer_virt, 0, 4096);

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
}

void xhci_configure_bulk_endpoints(uint8_t slot_id, uint8_t bulk_in, uint8_t bulk_out, uint16_t max_packet_size) {
    
    usb_device_t* dev = &usb_devices[slot_id];

    uint64_t in_ctx_phys = alloc_page();
    uint32_t* in_ctx = (uint32_t*)in_ctx_phys;
    for (int i = 0; i < 4096 / 4; i++) in_ctx[i] = 0;

    dev->bulk_in_ring_phys = alloc_page();
    dev->bulk_in_ring_virt = (uint32_t*)dev->bulk_in_ring_phys;
    for (int i = 0; i < 4096 / 4; i++) dev->bulk_in_ring_virt[i] = 0;

    dev->bulk_out_ring_phys = alloc_page();
    dev->bulk_out_ring_virt = (uint32_t*)dev->bulk_out_ring_phys;
    for (int i = 0; i < 4096 / 4; i++) dev->bulk_out_ring_virt[i] = 0;

    uint32_t hccparams1 = *(volatile uint32_t*)(xhci_addr + 0x10);
    int ctx_size = (hccparams1 & (1 << 2)) ? 64 : 32;
    int ctx_shift = ctx_size / 4;

    uint32_t ctrl_offset = 0;
    uint32_t slot_offset = ctx_shift;

    uint8_t ep_in_index  = (bulk_in * 2) + 1;
    uint8_t ep_out_index = (bulk_out * 2) + 0;

    uint32_t ep_in_offset  = (ep_in_index + 1) * ctx_shift;
    uint32_t ep_out_offset = (ep_out_index + 1) * ctx_shift;

    in_ctx[ctrl_offset + 0] = 0;
    in_ctx[ctrl_offset + 1] =
        (1u << 0) |
        (1u << 1) |
        (1u << ep_in_index) |
        (1u << ep_out_index);

    uint32_t* out_dev_ctx = (uint32_t*)dcbaa_virt[slot_id];
    
    asm volatile("clflush %0" : : "m"(*out_dev_ctx));
    asm volatile("clflush %0" : : "m"(*(out_dev_ctx + ctx_shift)));
    asm volatile("mfence" ::: "memory");

    for (int i = 0; i < ctx_shift; i++) {
        in_ctx[slot_offset + i] = out_dev_ctx[i];
    }

    uint32_t ep0_in_offset  = 2 * ctx_shift;
    uint32_t ep0_out_offset = 1 * ctx_shift;
    for (int i = 0; i < ctx_shift; i++) {
        in_ctx[ep0_in_offset + i] = out_dev_ctx[ep0_out_offset + i];
    }

    in_ctx[slot_offset + 2] = 0;
    in_ctx[slot_offset + 3] = 0;
    uint8_t max_dci = (ep_in_index > ep_out_index) ? ep_in_index : ep_out_index;

    in_ctx[slot_offset + 0] = (in_ctx[slot_offset + 0] & ~(31u << 27)) | ((uint32_t)max_dci << 27);

    uint8_t error_count = 3;

    in_ctx[ep_in_offset + 0] = 0;
    in_ctx[ep_in_offset + 1] =
    ((uint32_t)error_count << 1) |
    ((uint32_t)6 << 3) |
    ((uint32_t)usb_bulk_in_burst << 8) |
    ((uint32_t)max_packet_size << 16);
    in_ctx[ep_in_offset + 2] = (uint32_t)(dev->bulk_in_ring_phys | 1);
    in_ctx[ep_in_offset + 3] = (uint32_t)(dev->bulk_in_ring_phys >> 32);
    in_ctx[ep_in_offset + 4] = 512;

    in_ctx[ep_out_offset + 0] = ((uint32_t)usb_bulk_out_burst) << 8;

    in_ctx[ep_out_offset + 1] =
    ((uint32_t)error_count << 1) |
    ((uint32_t)2 << 3) |
    ((uint32_t)usb_bulk_out_burst << 8) |
    ((uint32_t)max_packet_size << 16);
    in_ctx[ep_out_offset + 2] = (uint32_t)(dev->bulk_out_ring_phys | 1); 
    in_ctx[ep_out_offset + 3] = (uint32_t)(dev->bulk_out_ring_phys >> 32);
    in_ctx[ep_out_offset + 4] = 512;

    for (int i = 0; i < 512; i += 16) {
        asm volatile("clflush %0" : : "m"(in_ctx[i]));
    }

    dev->bulk_in_ring_virt[63 * 4 + 0] = (uint32_t)dev->bulk_in_ring_phys;
    dev->bulk_in_ring_virt[63 * 4 + 1] = (uint32_t)(dev->bulk_in_ring_phys >> 32);
    dev->bulk_in_ring_virt[63 * 4 + 2] = 0;
    dev->bulk_in_ring_virt[63 * 4 + 3] = (6 << 10) | (1 << 1) | 1;

    dev->bulk_out_ring_virt[63 * 4 + 0] = (uint32_t)dev->bulk_out_ring_phys;
    dev->bulk_out_ring_virt[63 * 4 + 1] = (uint32_t)(dev->bulk_out_ring_phys >> 32);
    dev->bulk_out_ring_virt[63 * 4 + 2] = 0;
    dev->bulk_out_ring_virt[63 * 4 + 3] = (6 << 10) | (1 << 1) | 1;

    asm volatile("clflush %0" : : "m"(dev->bulk_in_ring_virt[63 * 4]));
    asm volatile("clflush %0" : : "m"(dev->bulk_out_ring_virt[63 * 4]));
    asm volatile("mfence" ::: "memory");

    dev->bulk_out_idx   = 0;
    dev->bulk_in_idx    = 0;
    dev->bulk_out_cycle = 1;
    dev->bulk_in_cycle  = 1;

    cmd_configure_ep_phys_addr = xhci_post_command(
        (uint32_t)in_ctx_phys,
        (uint32_t)(in_ctx_phys >> 32),
        0,
        XHCI_CMD_CONFIGURE_ENDPOINT,
        ((uint32_t)slot_id << 24)
    );

}

void flush_xhci_event_ring(uintptr_t xhci_base, uintptr_t event_ring_virt) {
    uint8_t caplength = *(volatile uint8_t*)(xhci_base);
    uintptr_t op_base = xhci_base + caplength;
    
    uint32_t rtsoff = *(volatile uint32_t*)(xhci_base + 0x18);
    uintptr_t rt_base = xhci_base + rtsoff;
    
    *(volatile uint32_t*)(op_base + 0x04) = 0x8;
    
    uint64_t etp = event_ring_virt;
    
    *(volatile uint64_t*)(rt_base + 0x38) = etp | (1ULL << 3);
}


int xhci_scsi_command(uintptr_t xhci_base, uint8_t slot_id, uint8_t* cdb, uint8_t cdb_len, uint32_t data_len) {
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    usb_device_t* dev = &usb_devices[slot_id];

    if (!disk_buffer_phys) {
        disk_buffer_phys = alloc_page();
        disk_buffer_virt = (uint8_t*)disk_buffer_phys;
    }

    for (int i = 0; i < 3072; i++) {
        disk_buffer_virt[i] = 0;
    }

    uint64_t cbw_phys = disk_buffer_phys + 1024;
    usb_ms_cbw_t* cbw = (usb_ms_cbw_t*)(disk_buffer_virt + 1024);
    uint64_t csw_phys = disk_buffer_phys + 2048;
    uint8_t* csw_virt = disk_buffer_virt + 2048;

    cbw->signature       = 0x43425355; // "USBC"
    cbw->tag             = 0xDEADBEEF;
    cbw->transfer_length = data_len;
    cbw->flags           = (data_len > 0) ? 0x80 : 0x00;
    cbw->lun             = 0;
    cbw->cmd_length      = cdb_len;

    for (int i = 0; i < cdb_len && i < 16; i++) {
        cbw->scsi_cmd[i] = cdb[i];
    }

    volatile uint32_t* out_ring = (volatile uint32_t*)dev->bulk_out_ring_virt;
    int o_idx = dev->bulk_out_idx * 4;

    out_ring[o_idx + 0] = (uint32_t)cbw_phys;
    out_ring[o_idx + 1] = (uint32_t)(cbw_phys >> 32);
    out_ring[o_idx + 2] = 31;
    asm volatile("sfence" ::: "memory");
    out_ring[o_idx + 3] = (TRB_TYPE_NORMAL << 10) | (dev->bulk_out_cycle & 1);

    dev->bulk_out_idx++;

    if (dev->bulk_out_idx == 62) {
        out_ring[63 * 4 + 0] = (uint32_t)dev->bulk_out_ring_phys;
        out_ring[63 * 4 + 1] = (uint32_t)(dev->bulk_out_ring_phys >> 32);
        out_ring[63 * 4 + 2] = 0;
        out_ring[63 * 4 + 3] = (6 << 10) | (1 << 1) | (dev->bulk_out_cycle & 1);
        asm volatile("clflush %0" : : "m"(out_ring[63 * 4]));
        asm volatile("sfence" ::: "memory");
    }

    if (dev->bulk_out_idx >= 63) {
        out_ring[63 * 4 + 3] = (6 << 10) | (1 << 1) | (dev->bulk_out_cycle & 1);
        asm volatile("clflush %0" : : "m"(out_ring[63 * 4]));
        dev->bulk_out_idx = 0;
        dev->bulk_out_cycle ^= 1;
    }

    asm volatile("clflush %0" : : "m"(*cbw));
    asm volatile("clflush %0" : : "m"(out_ring[o_idx]));
    asm volatile("mfence" ::: "memory");

    volatile uint32_t* in_ring = (volatile uint32_t*)dev->bulk_in_ring_virt;

    if (data_len > 0) {
        int i_idx = dev->bulk_in_idx * 4;
        in_ring[i_idx + 0] = (uint32_t)disk_buffer_phys;
        in_ring[i_idx + 1] = (uint32_t)(disk_buffer_phys >> 32);
        in_ring[i_idx + 2] = data_len;
        asm volatile("sfence" ::: "memory");
        in_ring[i_idx + 3] = (TRB_TYPE_NORMAL << 10) | (1 << 2) | (dev->bulk_in_cycle & 1); // ISP

        asm volatile("clflush %0" : : "m"(in_ring[i_idx]));

        dev->bulk_in_idx++;

        if (dev->bulk_in_idx == 62) {
            in_ring[63 * 4 + 0] = (uint32_t)dev->bulk_in_ring_phys;
            in_ring[63 * 4 + 1] = (uint32_t)(dev->bulk_in_ring_phys >> 32);
            in_ring[63 * 4 + 2] = 0;
            in_ring[63 * 4 + 3] = (6 << 10) | (1 << 1) | (dev->bulk_in_cycle & 1);
            asm volatile("clflush %0" : : "m"(out_ring[63 * 4]));
            asm volatile("sfence" ::: "memory");
        }

        if (dev->bulk_in_idx >= 63) {
            in_ring[63 * 4 + 3] = (6 << 10) | (1 << 1) | (dev->bulk_in_cycle & 1);
            asm volatile("clflush %0" : : "m"(in_ring[63 * 4]));
            dev->bulk_in_idx = 0;
            dev->bulk_in_cycle ^= 1;
        }
    }

    int c_idx = dev->bulk_in_idx * 4;
    in_ring[c_idx + 0] = (uint32_t)csw_phys;
    in_ring[c_idx + 1] = (uint32_t)(csw_phys >> 32);
    in_ring[c_idx + 2] = 13;
    asm volatile("sfence" ::: "memory");
    in_ring[c_idx + 3] = (TRB_TYPE_NORMAL << 10) | (1 << 5) | (1 << 2) | (dev->bulk_in_cycle & 1); // IOC + ISP

    dev->expected_csw_trb_phys = dev->bulk_in_ring_phys + ((uint64_t)(c_idx / 4) * sizeof(xhci_trb_t));
    dev->io_completed = 0;

    asm volatile("clflush %0" : : "m"(in_ring[c_idx]));

    dev->bulk_in_idx++;
    if (dev->bulk_in_idx >= 63) {
        in_ring[63 * 4 + 3] = (6 << 10) | (1 << 1) | (dev->bulk_in_cycle & 1);
        asm volatile("clflush %0" : : "m"(in_ring[63 * 4]));
        dev->bulk_in_idx = 0;
        dev->bulk_in_cycle ^= 1;
    }

    asm volatile("mfence" ::: "memory");

    uint32_t out_ep_id = (dev->bulk_out_ep * 2) + 0;
    uint32_t in_ep_id  = (dev->bulk_in_ep * 2) + 1;

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = out_ep_id;
    sleep_ms(1);
    *(volatile uint32_t*)(db_base + (slot_id * 4)) = in_ep_id;

    int timeout = 5000;
    while (!dev->io_completed && --timeout > 0) {
        xhci_check_events(xhci_base);
        sleep_ms(1);
    }

    for (uint32_t i = 0; i < data_len; i += 64) {
        asm volatile("clflush %0" : : "m"(disk_buffer_virt[i]));
    }
    asm volatile("clflush %0" : : "m"(*csw_virt));
    asm volatile("mfence" ::: "memory");

    if (!dev->io_completed) {
        return -1;
    }

    usb_ms_csw_t* csw = (usb_ms_csw_t*)csw_virt;
    if (csw->dCSWSignature != 0x53425355 || csw->dCSWTag != 0xDEADBEEF) {
        return -2;
    }

    return (int)csw->bCSWStatus;
}

int xhci_scsi_inquiry(uintptr_t xhci_base, uint8_t slot_id) {
    uint8_t cdb[6] = { 0x12, 0, 0, 0, 36, 0 };
    return xhci_scsi_command(xhci_base, slot_id, cdb, 6, 36);
}

int xhci_scsi_test_unit_ready(uintptr_t xhci_base, uint8_t slot_id) {
    uint8_t cdb[6] = { 0x00, 0, 0, 0, 0, 0 };
    return xhci_scsi_command(xhci_base, slot_id, cdb, 6, 0);
}

int xhci_scsi_request_sense(uintptr_t xhci_base, uint8_t slot_id, uint8_t* out_sense_key) {
    uint8_t cdb[6] = { 0x03, 0, 0, 0, 18, 0 };
    int status = xhci_scsi_command(xhci_base, slot_id, cdb, 6, 18);
    if (status == 0 && out_sense_key != NULL) {
        *out_sense_key = disk_buffer_virt[2] & 0x0F;
    }
    return status;
}

int xhci_usb_msc_init(uintptr_t xhci_base, uint8_t slot_id) {
    if (xhci_scsi_inquiry(xhci_base, slot_id) != 0) {
        return -1;
    }

    for (int attempt = 0; attempt < 10; attempt++) {
        int tur = xhci_scsi_test_unit_ready(xhci_base, slot_id);
        if (tur == 0) {
            return 0;
        }

        uint8_t sense_key = 0;
        xhci_scsi_request_sense(xhci_base, slot_id, &sense_key);

        sleep_ms(50);
    }

    return -2;
}


int xhci_usb_read_sector(uintptr_t xhci_base, uint8_t slot_id, uint32_t lba_block) {
    uint8_t cdb[10] = {
        0x28,
        0x00,
        (uint8_t)((lba_block >> 24) & 0xFF),
        (uint8_t)((lba_block >> 16) & 0xFF),
        (uint8_t)((lba_block >> 8)  & 0xFF),
        (uint8_t)((lba_block >> 0)  & 0xFF),
        0x00,
        0x00,
        0x01,
        0x00
    };

    int status = xhci_scsi_command(xhci_base, slot_id, cdb, 10, 512);
    if (status != 0) {
        return -1;
    }

    usb_ms_csw_t* csw = (usb_ms_csw_t*)(disk_buffer_virt + 2048);
    if (csw->dCSWDataResidue != 0) {
        return -1;
    }

    return 0;
}

int xhci_disk_read_sector(int slot_id, uint64_t lba, uint64_t target_addr) {

    if (xhci_addr == 0 || slot_id <= 0 || slot_id >= MAX_USB_DEVICES) {
        return -1;
    }

    int status = xhci_usb_read_sector(xhci_addr, (uint8_t)slot_id, (uint32_t)lba);
    if (status != 0) {
        return status;
    }

    memcpy((void*)target_addr, disk_buffer_virt, 512);
    return 0;
}