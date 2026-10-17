#include "Am79C973.h"
#include "core.h"
#include "os_memory.h"
#include "pci.h"

int am79c973_base = 0;
volatile struct Am79C973_TxDescriptor* tx_desc; 
volatile struct Am79C973_RxDescriptor* rx_desc;
uint8_t* tx_buffer;
uint8_t* rx_buffer;

struct Am79C973_InitBlock init_block __attribute__((aligned(4)));

void init_rx_descriptor() {

    rx_desc = kmalloc(sizeof(struct Am79C973_RxDescriptor));
    uintptr_t rx_buf = (uintptr_t)kmalloc(1518);
    if(rx_buf == 0 || rx_desc == 0) {
        KeBugCheck(0x20);
    }

    uint32_t len_field = (uint32_t)(-1518) & 0x0FFF;
    len_field |= 0xF000;

    uint32_t flags_field = 0x83000000;
    rx_desc->status_and_length = flags_field | len_field;

    rx_desc->buffer_pointer = (uint32_t)virtual_to_physical(pt_begin, (uintptr_t)rx_buf);
    rx_desc->status2 = 0;
}

void init_tx_descriptor() {

    tx_desc = kmalloc(sizeof(struct Am79C973_TxDescriptor));
    uintptr_t tx_buf = (uintptr_t)kmalloc(1518);
    if(tx_buf == 0 || tx_desc == 0) {
        KeBugCheck(0x20);
    }

    tx_buffer = (uint8_t*)tx_buf;

    uint32_t len_field = (uint32_t)(-1518) & 0x0FFF;
    len_field |= 0xF000;

    uint32_t flags_field = 0x83000000;
    tx_desc->status_and_length = flags_field | len_field;

    tx_desc->buffer_pointer = (uint32_t)virtual_to_physical(pt_begin, tx_buf);
    tx_desc->status2 = 0;
}

void am79c973_write_csr(uint16_t base, uint32_t csr_num, uint32_t value) {
    outw(base + 0x12, csr_num);
    outw(base + 0x10, value);
}

uint32_t am79c973_read_csr(uint16_t base, uint32_t csr_num) {
    outw(base + 0x12, csr_num);
    return inw(base + 0x10);
}

void am79c973_write_bcr(uint16_t base, uint32_t bcr_num, uint32_t value) {
    outw(base + 0x12, bcr_num);
    outw(base + 0x16, value);
}

int init_am79c973(am79c973_device_t *device) {

    for (uint64_t i = 0; i < total_devices; i++) {

        if (pci_devices[i].vendor_id == 0x1022 && pci_devices[i].device_id == 0x2000) {

            uint8_t bus = pci_devices[i].bus;
            uint8_t dev = pci_devices[i].device;
            uint8_t func = pci_devices[i].function;

            uint32_t cmd_status = pci_read_config_32(bus, dev, func, 0x04);
            
            cmd_status |= (1 << 2);
            cmd_status |= (1 << 1); 
            pci_write_config_32(bus, dev, func, 0x04, cmd_status);

            uint32_t bar0 = pci_read_config_32(bus, dev, func, 0x10);
            
            if ((bar0 & 1) == 1) {
                am79c973_base = (uint16_t)(bar0 & 0xFFFFFFFC);

                inw(am79c973_base + 0x14);

                outw(am79c973_base + 0x14, 0x0000);

                uint16_t mac0 = inw(am79c973_base + 0x00);
                uint16_t mac1 = inw(am79c973_base + 0x02);
                uint16_t mac2 = inw(am79c973_base + 0x04);

                device->mac[0] = mac0 & 0xFF;
                device->mac[1] = (mac0 >> 8) & 0xFF;
                device->mac[2] = mac1 & 0xFF;
                device->mac[3] = (mac1 >> 8) & 0xFF;
                device->mac[4] = mac2 & 0xFF;
                device->mac[5] = (mac2 >> 8) & 0xFF;

                init_block.mode = 0;
                init_block.rlen_tlen = 0x00;

                init_block.mac_low  = *(uint16_t*)(&device->mac[0]);
                init_block.mac_mid  = *(uint16_t*)(&device->mac[2]);
                init_block.mac_high = *(uint16_t*)(&device->mac[4]);
                init_block.reserved2 = 0;

                init_block.ladd_low = 0;
                init_block.ladd_high = 0;

                init_block.recv_ring_pointer  = (uint32_t)((uintptr_t)rx_desc);
                init_block.trans_ring_pointer = (uint32_t)((uintptr_t)tx_desc);

                init_block.mode = 0x0000;
                init_block.rlen_tlen = 0x00;
                init_block.reserved1 = 0;
                init_block.reserved2 = 0;

                init_rx_descriptor();
                init_tx_descriptor();

                init_block.trans_ring_pointer = (uint32_t)virtual_to_physical(pt_begin, (uintptr_t)tx_desc);
                init_block.recv_ring_pointer  = (uint32_t)virtual_to_physical(pt_begin, (uintptr_t)rx_desc);

                uint32_t init_block_phys = (uint32_t)virtual_to_physical(pt_begin, (uintptr_t)&init_block);


                am79c973_write_csr(am79c973_base, 58, 0x0002);
                am79c973_write_csr(am79c973_base, 1, init_block_phys & 0xFFFF);
                am79c973_write_csr(am79c973_base, 2, (init_block_phys >> 16) & 0xFFFF);

                am79c973_write_csr(am79c973_base, 0, 0x0001);

                while (!(am79c973_read_csr(am79c973_base, 0) & 0x0100)) {
                    asm volatile("pause");
                }

                am79c973_write_csr(am79c973_base, 0, 0x0100);

                am79c973_write_csr(am79c973_base, 0, 0x0002);

                return 1;
            }
        }
    }

    return 0;
}

void am79c973_send_packet(const uint8_t* packet_data, uint32_t length) {
    if (length > 1518) {
        length = 1518;
    }

    while (tx_desc->status_and_length & 0x80000000) {
        asm volatile("wbinvd" ::: "memory");
    }

    for (uint32_t i = 0; i < length; i++) {
        tx_buffer[i] = packet_data[i];
    }
    asm volatile("wbinvd" ::: "memory");
    asm volatile("clflush (%0)" :: "r"(tx_buffer) : "memory");

    uint32_t len_field = (uint32_t)(-length) & 0x0FFF;
    len_field |= 0xF000;

    uint32_t flags_field = 0x83000000;
    tx_desc->status_and_length = flags_field | len_field;
    tx_desc->status2 = 0;

    outw(am79c973_base + 0x12, 0); 
    
    uint16_t csr0 = inw(am79c973_base + 0x10);
    outw(am79c973_base + 0x10, 0x000A);
}

void send_test_arp_packet(uint8_t* my_mac) {

    struct FullArpPacket packet;

    for(int i = 0; i < 6; i++) packet.eth.dest_mac[i] = 0xFF;
    for(int i = 0; i < 6; i++) packet.eth.src_mac[i] = my_mac[i];
    packet.eth.ethertype = htons(0x0806); 

    packet.arp.hardware_type = htons(1);      // Ethernet
    packet.arp.protocol_type = htons(0x0800); // IPv4
    packet.arp.hardware_len  = 6;
    packet.arp.protocol_len  = 4;
    packet.arp.operation     = htons(1);      // Request

    for(int i = 0; i < 6; i++) packet.arp.src_mac[i] = my_mac[i];
    packet.arp.src_ip[0] = 192; packet.arp.src_ip[1] = 168; 
    packet.arp.src_ip[2] = 1;   packet.arp.src_ip[3] = 200;

    for(int i = 0; i < 6; i++) packet.arp.dst_mac[i] = 0x00;
    packet.arp.dst_ip[0] = 192; packet.arp.dst_ip[1] = 168; 
    packet.arp.dst_ip[2] = 1;   packet.arp.dst_ip[3] = 1;

    am79c973_send_packet((uint8_t*)&packet, sizeof(struct FullArpPacket));

}

void check_incoming_packets() {
    if (!(rx_desc->status_and_length & 0x80000000)) {
        
        uint32_t received_length = rx_desc->status2 & 0x0FFF; 
        
        rx_desc->status_and_length = 0x80000000 | ((uint16_t)(-1518) & 0x0FFF);
        asm volatile("wbinvd" ::: "memory");
    }
}