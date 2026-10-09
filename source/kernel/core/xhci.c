#include "core.h"
#include "pci.h"
#include "xhci.h"

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
uint8_t usb_device_addresed = 0;
uint8_t usb_device_config_returned = 0;
uint8_t usb_device_config_configurated = 0;
uint8_t usb_device_opened = 0;

uint8_t usb_bulk_in_burst = 0;
uint8_t usb_bulk_out_burst = 0;

uint64_t* dcbaa_virt = NULL;
uint64_t  dcbaa_phys = 0;

xhci_device_context_t* dev_context_virt = NULL;
uint64_t               dev_context_phys = 0;

xhci_input_context_t*  input_context_virt = NULL;
uint64_t               input_context_phys = 0;

xhci_trb_t*            ep0_ring_virt = NULL;
uint64_t               ep0_ring_phys = 0;

uint64_t cmd_enable_slot_phys_addr = 0;
uint64_t cmd_address_device_phys_addr = 0;
uint64_t cmd_configure_ep_phys_addr = 0;

uint64_t trb_get_descriptor_status_phys = 0;
uint64_t trb_set_configuration_status_phys = 0;
uint64_t trb_get_config_status_phys = 0;
uint64_t trb_get_full_config_status_phys = 0;

uint64_t                        desc_buffer_phys = 0;
usb_device_descriptor_t*        desc_buffer_virt = NULL;

uint8_t usb_bulk_in_endpoint = 0;
uint8_t usb_bulk_out_endpoint = 0;
uint16_t usb_bulk_max_packet_size = 0;

usb_config_descriptor_t* cfg = 0;

uint64_t bulk_in_ring_phys = 0;
uint32_t* bulk_in_ring_virt = NULL;

uint64_t bulk_out_ring_phys = 0;
uint32_t* bulk_out_ring_virt = NULL;

uint64_t disk_buffer_phys = 0;
uint8_t* disk_buffer_virt = NULL;

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
    map_page(phys_page, phys_page, 0x1B);

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
        uint8_t trb_cycle = trb->control & 1;

        if (trb_cycle != current_cycle) {
            break;
        }

        uint8_t trb_type = (trb->control >> 10) & 0x3F;

        if (trb_type == TRB_TYPE_CHANGE_SLOT) {
            uint8_t port_id = (trb->parameter_lo >> 24) & 0xFF;

            // Тут в будущем дописать что типо устройство чё-то сделало
        }
        else if (trb_type == TRB_TYPE_COMMAND_COMPLETION) {
            uint64_t cmd_trb_phys = ((uint64_t)trb->parameter_hi << 32) | trb->parameter_lo;
            uint8_t completion_code = (trb->status >> 24) & 0xFF;
            uint8_t slot_id = (trb->control >> 24) & 0xFF;

            if (completion_code == XHCI_TRB_SUCCESS) {
                
                if (cmd_trb_phys == cmd_enable_slot_phys_addr) {
                    usb_device_slot_id = slot_id;
                }
                
                else if (cmd_trb_phys == cmd_address_device_phys_addr) {
                    usb_device_addresed = 1;
                }
                else if (cmd_trb_phys == cmd_configure_ep_phys_addr) {
                    usb_device_opened = 1;
                }
                
                
                else {
                    tprintf(15, 15, 0x1F, "Undefined command!.\n");

                    while(1) {

                    }
                }
            } else {

                tprintf(15, 15, 0x1F, "xHCI: Command at 0x%x failed with code %d", cmd_trb_phys, completion_code);

                while(1) {

                }
            }
        }
        else if (trb_type == TRB_TYPE_TRANSFER_EVENT) {
            uint64_t event_trb_phys = ((uint64_t)trb->parameter_hi << 32) | trb->parameter_lo;
            uint8_t completion_code = (trb->status >> 24) & 0xFF;
            uint8_t event_slot_id = (trb->control >> 24) & 0xFF;

            asm volatile("clflush %0" : : "m"(*(volatile uint32_t*)desc_buffer_virt));
            uint32_t* raw_data = (uint32_t*)desc_buffer_virt;

            tprintf(10, 10, 0x1F, "Event 32! Code: %d. Raw Memory: 0x%x 0x%x 0x%x 0x%x\n", 
            completion_code, raw_data[0], raw_data[1], raw_data[2], raw_data[3]);

            tprintf(10, 11, 0x1F, "%x %x", trb->parameter_lo, trb->parameter_hi);

            if (completion_code == XHCI_TRB_SUCCESS) {

                if (event_trb_phys == trb_get_descriptor_status_phys) {
                    asm volatile("clflush %0" : : "m"(*(volatile uint32_t*)desc_buffer_virt));
                    //tprintf(15, 15, 0x1F, "USB Device Found! VendorID: 0x%x, ProductID: 0x%x", desc_buffer_virt->idVendor, desc_buffer_virt->idProduct);
                    
                    usb_device_config_returned = 1;
                }
                
                else if (event_trb_phys == trb_set_configuration_status_phys) {
                    usb_device_config_configurated = 1;
                }
                
                else if (event_trb_phys == trb_get_config_status_phys) {
                    asm volatile("clflush %0" : : "m"(*(volatile uint32_t*)desc_buffer_virt));

                    cfg = kmalloc(sizeof(usb_config_descriptor_t));
                    memcpy(cfg, desc_buffer_virt, sizeof(usb_config_descriptor_t));
                }

                else if (event_trb_phys == trb_get_full_config_status_phys) {

                    volatile uint8_t* cache_ptr = (volatile uint8_t*)desc_buffer_virt;
                    for (int i = 0; i < 256; i += 64) {
                        asm volatile("clflush %0" : : "m"(*(cache_ptr + i)));
                    }
                    
                    uint8_t* raw_bytes = (uint8_t*)desc_buffer_virt;
                    uint32_t offset = 0;
                    uint32_t total_len = cfg->wTotalLength;

                    usb_endpoint_descriptor_t* last_ep = NULL;
                    int8_t last_ep_is_in = 0;

                    while (offset < total_len) {
                        uint8_t desc_len  = raw_bytes[offset + 0];
                        uint8_t desc_type = raw_bytes[offset + 1];

                        if (desc_len == 0) break;

                        if (desc_type == 5) {
                            usb_endpoint_descriptor_t* ep = (usb_endpoint_descriptor_t*)&raw_bytes[offset];
                            
                            if ((ep->bmAttributes & 0x03) == 0x02) {
                                last_ep = ep;

                                if (ep->bEndpointAddress & 0x80) {
                                    usb_bulk_in_endpoint = ep->bEndpointAddress & 0x0F;
                                    usb_bulk_max_packet_size = ep->wMaxPacketSize;
                                    last_ep_is_in = 1;
                                } else {
                                    usb_bulk_out_endpoint = ep->bEndpointAddress & 0x0F;
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
                                    usb_bulk_in_burst = max_burst;
                                } else {
                                    usb_bulk_out_burst = max_burst;
                                }
                                
                                last_ep = NULL;
                            }
                        }

                        offset += desc_len;
                    }
                            
                }
                else if (event_trb_phys == (bulk_in_ring_phys + 16)) {
            
                    volatile uint8_t* cache_ptr = (volatile uint8_t*)disk_buffer_virt;
                    for (int i = 0; i < 512; i += 64) {
                        asm volatile("clflush %0" : : "m"(*(cache_ptr + i)));
                    }

                    tprintf(0, 15, 0x1F, "xHCI: DISK SECTOR READ SUCCESS! First bytes of MBR:\n");
                    tprintf(0, 16, 0x1F, "0x%X 0x%X 0x%X 0x%X\n", 
                            disk_buffer_virt[0], disk_buffer_virt[1], 
                            disk_buffer_virt[2], disk_buffer_virt[3]);
                            
                } else if(event_trb_phys == bulk_in_ring_phys) {
                    tprintf(5, 5, 0x1F, "First event!");
                }
                else {
                    tprintf(5, 5, 0x1F, "Undefined event.");
                }

                volatile uint8_t* ptr = (volatile uint8_t*)desc_buffer_virt;
                for (int i = 0; i < 18; i += 64) {
                    asm volatile("clflush %0" : : "m"(*(ptr + i)));
                }

            } else {

                tprintf(15, 15, 0x1F, "Transfer failed with code: %d\n", completion_code);

                while(1) {
                    
                }

            }
        } 
        
        
        
        else {
            tprintf(10, 10, 0x1F, "Undefined answer!");
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

        if (status & PORTSC_CCS) {
            uint8_t speed = (status >> 10) & 0x0F;

            if (speed != 4 && !(status & PORTSC_PED)) {
                uint32_t reset_cmd = (status & ~PORTSC_RWC_MASK) | PORTSC_PR;
                *(volatile uint32_t*)portsc_reg = reset_cmd;

                for (volatile int delay = 0; delay < 50000000; delay++); 
                while (*(volatile uint32_t*)portsc_reg & PORTSC_PR);
                for (volatile int delay = 0; delay < 10000000; delay++);

                uint32_t post_reset = *(volatile uint32_t*)portsc_reg;
                *(volatile uint32_t*)portsc_reg = (post_reset & ~PORTSC_RWC_MASK) | PORTSC_PRC;
            } 
            else if (speed == 4) {
                uint32_t clear_flags = (status & ~PORTSC_RWC_MASK) | (status & PORTSC_RWC_MASK);
                *(volatile uint32_t*)portsc_reg = clear_flags;
                for (volatile int delay = 0; delay < 10000000; delay++);
            }

            uint32_t final_check = *(volatile uint32_t*)portsc_reg;
            uint8_t link_state = (final_check >> 5) & 0x0F;

            if ((final_check & PORTSC_PED) || (speed == 4 && link_state == 0)) {

                
                xhci_request_slot();
                sleep_ms(100);
                xhci_check_events(xhci_base);

                if(usb_device_slot_id != 0) {
                    xhci_setup_device(usb_device_slot_id, i+1, speed);
                    sleep_ms(100);
                    xhci_check_events(xhci_base);

                    if(usb_device_addresed != 0) {
                        xhci_get_device_descriptor(xhci_addr, usb_device_slot_id);
                        sleep_ms(100);
                        xhci_check_events(xhci_base);

                        if(usb_device_config_returned != 0) {
                        
                            xhci_set_configuration(xhci_base, usb_device_slot_id, 1);
                            sleep_ms(100);
                            xhci_check_events(xhci_base);

                            if(usb_device_config_configurated != 0) {


                                xhci_get_configuration_descriptor(xhci_base, usb_device_slot_id);
                                sleep_ms(100);
                                xhci_check_events(xhci_base);

                                if(cfg != 0) {
                                    xhci_get_full_configuration_descriptor(xhci_base, usb_device_slot_id, cfg->wTotalLength);
                                    sleep_ms(100);
                                    xhci_check_events(xhci_base);

                                    if(usb_bulk_max_packet_size != 0) {
                                        xhci_configure_bulk_endpoints(usb_device_slot_id, usb_bulk_in_endpoint, usb_bulk_out_endpoint, usb_bulk_max_packet_size);
                                        sleep_ms(100);
                                        xhci_check_events(xhci_base);

                                        if(usb_device_opened != 0) {

                                            tprintf(10, 10, 0x1F, "Trying to read...");

                                            xhci_usb_read_sector(xhci_base, usb_device_slot_id, 0);
                                            sleep_ms(1000);
                                            xhci_check_events(xhci_base);


                                            while(1) {

                                            }

                                        }

                                    }

                                }

                            }
                            

                        } else {
                            tprintf(15, 15, 0x1F, "Fail to get config from USB device!");
                            while(1) {

                            }
                        }

                    } else {
                        tprintf(15, 15, 0x1F, "Fail to address USB device!");
                        while(1) {

                        }
                    }

                } else {
                    tprintf(15, 15, 0x1F, "Fail to get slot id!");
                    while(1) {

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

    cmd_enqueue_idx++;
    if (cmd_enqueue_idx >= COMMAND_RING_SIZE) {
        cmd_enqueue_idx = 0;
        cmd_cycle = !cmd_cycle;
    }

    *(volatile uint32_t*)(db_base + 0) = 0; 
    return target_trb_phys;
}

void xhci_request_slot() {
    cmd_enable_slot_phys_addr = xhci_post_command(0, 0, 0, XHCI_CMD_ENABLE_SLOT, 0);
}

void xhci_init_dcbaa(uintptr_t xhci_base) {
    uintptr_t op_base = xhci_base + caps.op_regs_offset;

    uint32_t dcbaa_size = (caps.max_slots + 1) * sizeof(uint64_t);

    dcbaa_phys = alloc_page();
    while(dcbaa_size > 4096) {
        dcbaa_size -= 4096;
        alloc_page();
    }

    dcbaa_virt = (uint64_t*)dcbaa_phys;

    for (uint32_t i = 0; i <= caps.max_slots; i++) {
        dcbaa_virt[i] = 0;
    }

    *(volatile uint64_t*)(op_base + XHCI_OP_DCBAAP) = dcbaa_phys;
}

void xhci_setup_device(uint8_t slot_id, uint8_t port_id, uint8_t speed) {
uint32_t hccparams1 = *(volatile uint32_t*)(xhci_addr + 0x0C);
    input_context_phys = alloc_page();
    xhci_input_context_t* input_ctx = (xhci_input_context_t*)input_context_phys;
    memset(input_ctx, 0, 2048);

    dev_context_phys = alloc_page();
    xhci_device_context_t* dev_ctx = (xhci_device_context_t*)dev_context_phys;
    memset(dev_ctx, 0, 1024);
    dcbaa_virt[slot_id] = dev_context_phys;

    ep0_ring_phys = alloc_page();
    ep0_ring_virt = (xhci_trb_t*)ep0_ring_phys;
    memset(ep0_ring_virt, 0, 1024);
    for (int i = 0; i < 4096 / 4; i++) ((uint32_t*)ep0_ring_virt)[i] = 0;

    input_ctx->control[0] = 0;
    input_ctx->control[1] = (1 << 0);

    input_ctx->slot[0] = (1 << 27);
    input_ctx->slot[1] = ((uint32_t)port_id << 16) | (speed & 0x0F);

    uint32_t max_packet_size = (speed == 4) ? 512 : 64;
    input_ctx->ep[1] = (3 << 1) | (4 << 3) | (max_packet_size << 16);
    input_ctx->ep[2] = (uint32_t)(ep0_ring_phys | 1);
    input_ctx->ep[3] = (uint32_t)(ep0_ring_phys >> 32);

    uint32_t cmd_status = 0;
    uint32_t extended_cmd_type = 11 | ((uint32_t)slot_id << 14);
    
    cmd_address_device_phys_addr = xhci_post_command(
        (uint32_t)input_context_phys, 
        (uint32_t)(input_context_phys >> 32), 
        0, 
        extended_cmd_type,
        0
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

    if (!desc_buffer_phys) {
        desc_buffer_phys = alloc_page();
        desc_buffer_virt = (usb_device_descriptor_t*)desc_buffer_phys;
    }
    memset(desc_buffer_virt, 0, 4096); 

    uint32_t* ep0_dw = (uint32_t*)ep0_ring_virt;
    uint8_t ep0_cycle = 1;

    ep0_dw[0] = 0x01000680;
    ep0_dw[1] = 0x00120000;
    ep0_dw[2] = 8;

    ep0_dw[3] = (2 << 10) | (2 << 16) | (1 << 6) | ep0_cycle;

    ep0_dw[4] = (uint32_t)desc_buffer_phys;         
    ep0_dw[5] = (uint32_t)(desc_buffer_phys >> 32);  
    ep0_dw[6] = 18; 
    ep0_dw[7] = (3 << 10) | (1 << 16) | ep0_cycle;

    ep0_dw[8] = 0;
    ep0_dw[9] = 0;
    ep0_dw[10] = 0;
    ep0_dw[11] = (4 << 10) | (0 << 16) | (1 << 5) | ep0_cycle;

    trb_get_descriptor_status_phys = ep0_ring_phys + (2 * sizeof(xhci_trb_t));

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
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
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    uint32_t* ep0_dw = (uint32_t*)ep0_ring_virt;

    uint8_t ep0_cycle = 1;

    uint64_t setup_packet = pack_set_config_packet(config_value);
    uint32_t* setup_dw = (uint32_t*)&setup_packet;

    uint32_t idx = 12; 

    ep0_dw[idx + 0] = setup_dw[0];
    ep0_dw[idx + 1] = setup_dw[1];
    ep0_dw[idx + 2] = 8;

    ep0_dw[idx + 3] = (2 << 10) | (0 << 16) | (1 << 6) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = 0;
    ep0_dw[idx + 1] = 0;
    ep0_dw[idx + 2] = 0;
    
    ep0_dw[idx + 3] = (4 << 10) | (1 << 16) | (1 << 5) | ep0_cycle;

    trb_set_configuration_status_phys = ep0_ring_phys + (4 * sizeof(xhci_trb_t));

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
}

void xhci_get_configuration_descriptor(uintptr_t xhci_base, uint8_t slot_id) {
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    uint32_t* ep0_dw = (uint32_t*)ep0_ring_virt;
    uint8_t ep0_cycle = 1;

    uint64_t setup_packet = 0;
    setup_packet |= (uint64_t)0x80 << 0;
    setup_packet |= (uint64_t)0x06 << 8;
    setup_packet |= (uint64_t)0x0200 << 16;
    setup_packet |= (uint64_t)0x0000 << 32;
    setup_packet |= (uint64_t)9 << 48;

    uint32_t* setup_dw = (uint32_t*)&setup_packet;

    uint32_t idx = 20; 

    trb_get_config_status_phys = ep0_ring_phys + (7 * sizeof(xhci_trb_t));

    ep0_dw[idx + 0] = setup_dw[0];
    ep0_dw[idx + 1] = setup_dw[1];
    ep0_dw[idx + 2] = 8;
    ep0_dw[idx + 3] = (2 << 10) | (2 << 16) | (1 << 6) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = (uint32_t)desc_buffer_phys;         
    ep0_dw[idx + 1] = (uint32_t)(desc_buffer_phys >> 32);  
    ep0_dw[idx + 2] = 9;
    ep0_dw[idx + 3] = (3 << 10) | (1 << 16) | ep0_cycle;

    idx += 4;
    ep0_dw[idx + 0] = 0;
    ep0_dw[idx + 1] = 0;
    ep0_dw[idx + 2] = 0;
    ep0_dw[idx + 3] = (4 << 10) | (0 << 16) | (1 << 5) | ep0_cycle;

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
}

void xhci_get_full_configuration_descriptor(uintptr_t xhci_base, uint8_t slot_id, uint16_t total_length) {
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;
    uint32_t* ep0_dw = (uint32_t*)ep0_ring_virt;
    uint8_t ep0_cycle = 1;

    uint64_t setup_packet = 0;
    setup_packet |= (uint64_t)0x80 << 0;   
    setup_packet |= (uint64_t)0x06 << 8;   
    setup_packet |= (uint64_t)0x0200 << 16; 
    setup_packet |= (uint64_t)0x0000 << 32; 
    setup_packet |= (uint64_t)total_length << 48;

    uint32_t* setup_dw = (uint32_t*)&setup_packet;

    uint32_t idx = 32; 

    trb_get_full_config_status_phys = ep0_ring_phys + (10 * sizeof(xhci_trb_t));

    ep0_dw[idx + 0] = setup_dw[0];
    ep0_dw[idx + 1] = setup_dw[1];
    ep0_dw[idx + 2] = 8;
    ep0_dw[idx + 3] = (2 << 10) | (2 << 16) | (1 << 6) | ep0_cycle;

    idx = 36;
    ep0_dw[idx + 0] = (uint32_t)desc_buffer_phys;         
    ep0_dw[idx + 1] = (uint32_t)(desc_buffer_phys >> 32);  
    ep0_dw[idx + 2] = total_length;
    
    ep0_dw[idx + 3] = (3 << 10) | (1 << 16) | ep0_cycle;

    idx = 40;
    ep0_dw[idx + 0] = 0;
    ep0_dw[idx + 1] = 0;
    ep0_dw[idx + 2] = 0;
    ep0_dw[idx + 3] = (4 << 10) | (0 << 16) | (1 << 5) | ep0_cycle;

    memset(desc_buffer_virt, 0, 4096);

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 1; 
}

void xhci_configure_bulk_endpoints(uint8_t slot_id, uint8_t bulk_in, uint8_t bulk_out, uint16_t max_packet_size) {
    uint64_t in_ctx_phys = alloc_page();
    uint32_t* in_ctx = (uint32_t*)in_ctx_phys;
    for (int i = 0; i < 4096 / 4; i++) in_ctx[i] = 0;

    bulk_in_ring_phys = alloc_page();
    bulk_in_ring_virt = (uint32_t*)bulk_in_ring_phys;
    for (int i = 0; i < 4096 / 4; i++) bulk_in_ring_virt[i] = 0;

    bulk_out_ring_phys = alloc_page();;
    bulk_out_ring_virt = (uint32_t*)bulk_out_ring_phys;
    for (int i = 0; i < 4096 / 4; i++) bulk_out_ring_virt[i] = 0;

    uint32_t ctrl_offset = 0;
    uint32_t slot_offset = 8;
    uint32_t ep_in_offset  = 8 * ((bulk_in * 2) + 1);
    uint32_t ep_out_offset = 8 * ((bulk_out * 2) + 0);

    in_ctx[ctrl_offset + 0] = 0;
    in_ctx[ctrl_offset + 1] = (1 << 0) | (1 << 3) | (1 << 4);
    in_ctx[slot_offset + 0] = (4 << 27);

    uint8_t error_count = 3;
    uint8_t ep_type     = 6;

    in_ctx[ep_in_offset + 0] = ((uint32_t)usb_bulk_in_burst) << 8;
    in_ctx[ep_in_offset + 1] = ((uint32_t)error_count << 1)  | 
                           ((uint32_t)ep_type << 3)      | 
                           ((uint32_t)max_packet_size << 16);

    // DWORD 2 и 3: Физический адрес кольца Bulk IN с битом DCS = 1
    in_ctx[ep_in_offset + 2] = (uint32_t)(bulk_in_ring_phys | 1);
    in_ctx[ep_in_offset + 3] = (uint32_t)(bulk_in_ring_phys >> 32);

    in_ctx[ep_out_offset + 0] = ((uint32_t)usb_bulk_out_burst) << 8;

    ep_type = 2;
    in_ctx[ep_out_offset + 1] = ((uint32_t)error_count << 1)  | 
                           ((uint32_t)ep_type << 3)      | 
                           ((uint32_t)max_packet_size << 16);

    in_ctx[ep_out_offset + 2] = (uint32_t)(bulk_out_ring_phys | 1);
    in_ctx[ep_out_offset + 3] = (uint32_t)(bulk_out_ring_phys >> 32);

    uint32_t extended_cmd_type = XHCI_CMD_CONFIGURE_ENDPOINT | ((uint32_t)slot_id << 14);

    cmd_configure_ep_phys_addr = xhci_post_command(
        (uint32_t)in_ctx_phys, 
        (uint32_t)(in_ctx_phys >> 32), 
        0,
        extended_cmd_type,
        0
    );
}

void xhci_usb_read_sector(uintptr_t xhci_base, uint8_t slot_id, uint32_t lba_block) {
    uintptr_t db_base = xhci_base + caps.doorbell_regs_offset;

    if (!disk_buffer_phys) {
        disk_buffer_phys = alloc_page();
        disk_buffer_virt = (uint8_t*)disk_buffer_phys;
    }
    memset(disk_buffer_virt, 0, 4096); 

    uint64_t cbw_phys = disk_buffer_phys + 1024;
    usb_ms_cbw_t* cbw = (usb_ms_cbw_t*)(disk_buffer_virt + 1024);
    uint64_t csw_phys = disk_buffer_phys + 2048;

    uint8_t cycle = 1;

    cbw->signature       = 0x43425355;
    cbw->tag             = 0xDEADBEEF; 
    cbw->transfer_length = 512;
    cbw->flags           = 0x80;
    cbw->lun             = 0;
    cbw->cmd_length      = 10;          

    cbw->scsi_cmd[0] = 0x28; // READ(10)
    cbw->scsi_cmd[1] = 0;    
    cbw->scsi_cmd[2] = (lba_block >> 24) & 0xFF;
    cbw->scsi_cmd[3] = (lba_block >> 16) & 0xFF;
    cbw->scsi_cmd[4] = (lba_block >> 8)  & 0xFF;
    cbw->scsi_cmd[5] = (lba_block >> 0)  & 0xFF;
    cbw->scsi_cmd[6] = 0;    
    cbw->scsi_cmd[7] = 0;
    cbw->scsi_cmd[8] = 1;
    cbw->scsi_cmd[9] = 0;    

    asm volatile("clflush %0" : : "m"(*cbw));

    uint32_t* out_dw = (uint32_t*)bulk_out_ring_virt;
    out_dw[0] = (uint32_t)cbw_phys;
    out_dw[1] = (uint32_t)(cbw_phys >> 32);
    out_dw[2] = 31;
    out_dw[3] = (TRB_TYPE_NORMAL << 10) | (1 << 5) | cycle;

    uint32_t* in_dw = (uint32_t*)bulk_in_ring_virt;
    in_dw[0] = (uint32_t)disk_buffer_phys;
    in_dw[1] = (uint32_t)(disk_buffer_phys >> 32);
    in_dw[2] = 512; 
    in_dw[3] = (TRB_TYPE_NORMAL << 10) | cycle;

    in_dw[4] = (uint32_t)csw_phys;
    in_dw[5] = (uint32_t)(csw_phys >> 32);
    in_dw[6] = 13; 
    in_dw[7] = (TRB_TYPE_NORMAL << 10) | (1 << 5) | cycle;

    uint32_t* dev_ctx_dw = (uint32_t*)dev_context_virt;

    uint32_t ep3_status = dev_ctx_dw[24 + 0];
    uint32_t ep4_status = dev_ctx_dw[32 + 0];

    tprintf(0, 9, 0x1F, "EP3 State: 0x%x, EP4 State: 0x%x\n", ep3_status, ep4_status);

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 4;

    for (volatile int delay = 0; delay < 500000; delay++);

    *(volatile uint32_t*)(db_base + (slot_id * 4)) = 3;
}