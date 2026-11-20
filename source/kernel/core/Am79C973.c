#include "Am79C973.h"
#include "core.h"
#include "os_memory.h"
#include "pci.h"

struct netif g_am79c973_netif;
static uint8_t lwip_tx_scratch[1536];

int am79c973_base = 0;
volatile struct Am79C973_TxDescriptor* tx_desc; 
volatile struct Am79C973_RxDescriptor* rx_desc;
uint8_t* tx_buffer;
uint8_t* rx_buffer;

extern uint32_t* fb_back_buffer;

struct Am79C973_InitBlock init_block __attribute__((aligned(4)));

static err_t am79c973_low_level_output(struct netif *netif, struct pbuf *p) {
    (void)netif;

    if (p->tot_len > sizeof(lwip_tx_scratch)) {
        return ERR_BUF;
    }

    uint16_t len = pbuf_copy_partial(p, lwip_tx_scratch, p->tot_len, 0);

    am79c973_send_packet(lwip_tx_scratch, len);

    return ERR_OK;
}

err_t am79c973_netif_init(struct netif *netif) {
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = am79c973_low_level_output;
    netif->mtu = 1500;
    netif->hwaddr_len = 6;

    for (int i = 0; i < 6; i++) {
        netif->hwaddr[i] = device_mac[i];
    }

    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    return ERR_OK;
}

void init_rx_descriptor() {

    rx_desc = kmalloc(sizeof(struct Am79C973_RxDescriptor));
    uintptr_t rx_buf = (uintptr_t)kmalloc(1518);
    if(rx_buf == 0 || rx_desc == 0) {
        KeBugCheck(0x20);
    }

    rx_buffer = (uint8_t*)rx_buf;

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
    
    inw(am79c973_base + 0x10);
    outw(am79c973_base + 0x10, 0x000A);

    internet_packages_sended++;
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
    packet.arp.src_ip[2] = 0;   packet.arp.src_ip[3] = 200;

    for(int i = 0; i < 6; i++) packet.arp.dst_mac[i] = 0x00;
    packet.arp.dst_ip[0] = 192; packet.arp.dst_ip[1] = 168; 
    packet.arp.dst_ip[2] = 0;   packet.arp.dst_ip[3] = 1;

    am79c973_send_packet((uint8_t*)&packet, sizeof(struct FullArpPacket));

}

void net_handle_arp(uint8_t* payload, uint32_t len) {
    if (len < sizeof(struct ArpPacket)) return;

    struct ArpPacket* arp = (struct ArpPacket*)payload;

    if (ntohs(arp->hardware_type) != 1 || ntohs(arp->protocol_type) != 0x0800) return;
    if (arp->hardware_len != 6 || arp->protocol_len != 4) return;

    if (ntohs(arp->operation) != ARP_OP_REQUEST) return;

    int ip_matches = 1;
    for (int i = 0; i < 4; i++) {
        if (arp->dst_ip[i] != device_ip[i]) {
            ip_matches = 0;
            break;
        }
    }

    if (!ip_matches) return;

    struct FullArpPacket reply;

    for (int i = 0; i < 6; i++) reply.eth.dest_mac[i] = arp->src_mac[i];
    for (int i = 0; i < 6; i++) reply.eth.src_mac[i] = device_mac[i];
    reply.eth.ethertype = htons(0x0806);

    reply.arp.hardware_type = htons(1);
    reply.arp.protocol_type = htons(0x0800);
    reply.arp.hardware_len  = 6;
    reply.arp.protocol_len  = 4;
    reply.arp.operation     = htons(ARP_OP_REPLY);

    for (int i = 0; i < 6; i++) reply.arp.src_mac[i] = device_mac[i];
    for (int i = 0; i < 4; i++) reply.arp.src_ip[i] = device_ip[i];

    for (int i = 0; i < 6; i++) reply.arp.dst_mac[i] = arp->src_mac[i];
    for (int i = 0; i < 4; i++) reply.arp.dst_ip[i] = arp->src_ip[i];

    am79c973_send_packet((uint8_t*)&reply, sizeof(struct FullArpPacket));
}

uint16_t net_checksum(uint16_t* addr, int count) {
    uint32_t sum = 0;

    while (count > 1) {
        sum += *addr++;
        count -= 2;
    }

    if (count > 0) {
        sum += *(uint8_t*)addr;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}

void net_handle_icmp(uint8_t* payload, uint32_t len, struct IpHeader* ip) {
    if (len < sizeof(struct IcmpHeader)) return;
    struct IcmpHeader* icmp = (struct IcmpHeader*)payload;

    if (icmp->type != 8) return;


    struct EthernetHeader* eth = (struct EthernetHeader*)(payload - sizeof(struct IpHeader) - sizeof(struct EthernetHeader));
    for (int i = 0; i < 6; i++) {
        uint8_t tmp = eth->dest_mac[i];
        eth->dest_mac[i] = eth->src_mac[i];
        eth->src_mac[i] = tmp;
    }

    for (int i = 0; i < 4; i++) {
        uint8_t tmp = ip->dst_ip[i];
        ip->dst_ip[i] = ip->src_ip[i];
        ip->src_ip[i] = tmp;
    }
    ip->total_length = htons(len + sizeof(struct IpHeader)); 

    icmp->type = 0;
    
    icmp->checksum = 0;
    icmp->checksum = net_checksum((uint16_t*)icmp, len);

    uint32_t total_packet_len = len + sizeof(struct IpHeader) + sizeof(struct EthernetHeader);
    am79c973_send_packet((uint8_t*)eth, total_packet_len);
}

void net_handle_ipv4(uint8_t* payload, uint32_t len) {
    if (len < sizeof(struct IpHeader)) return;
    struct IpHeader* ip = (struct IpHeader*)payload;

    for (int i = 0; i < 4; i++) {
        if (ip->dst_ip[i] != device_ip[i]) return;
    }

    if (ip->protocol == IP_PROTO_ICMP) {
        uint32_t ip_header_len = (ip->version_ihl & 0x0F) * 4;
        uint8_t* icmp_payload = payload + ip_header_len;
        uint32_t icmp_len = ntohs(ip->total_length) - ip_header_len;

        net_handle_icmp(icmp_payload, icmp_len, ip);
    }
}

void net_handle_ethernet(uint8_t* buffer, uint32_t len) {
    struct EthernetHeader* eth = (struct EthernetHeader*)buffer;

    int is_to_me = 1;
    int is_broadcast = 1;
    
    for (int i = 0; i < 6; i++) {
        if (eth->dest_mac[i] != device_mac[i]) is_to_me = 0;
        if (eth->dest_mac[i] != 0xFF) is_broadcast = 0;
    }

    if (!is_to_me && !is_broadcast) {
        return;
    }

    uint8_t* payload = buffer + sizeof(struct EthernetHeader);
    uint32_t payload_len = len - sizeof(struct EthernetHeader);

    uint16_t type = ntohs(eth->ethertype);

    switch (type) {
        case ETHERTYPE_ARP:
            net_handle_arp(payload, payload_len);
            break;
            
        case ETHERTYPE_IPV4:
            net_handle_ipv4(payload, payload_len);
            break;
            
        default:
            break;
    }
}

void check_incoming_packets() {
    asm volatile("wbinvd" ::: "memory");

    for (uintptr_t addr = (uintptr_t)rx_buffer; addr < (uintptr_t)rx_buffer + 1518; addr += 64) {
        asm volatile("clflush (%0)" :: "r"(addr) : "memory");
    }
    
    asm volatile("clflush (%0)" :: "r"(rx_desc) : "memory");
    asm volatile("mfence" ::: "memory");
    
    if (rx_desc->status_and_length & 0x80000000) {
        return; 
    }

    internet_packages_received++;

    uint32_t packet_len = rx_desc->status2 & 0x0FFF;

    if (packet_len >= 60) {
        uint16_t lwip_len = (uint16_t)(packet_len - 4);

        struct pbuf *p = pbuf_alloc(PBUF_RAW, lwip_len, PBUF_POOL);
        if (p != NULL) {
            pbuf_take(p, rx_buffer, lwip_len);

            if (g_am79c973_netif.input(p, &g_am79c973_netif) != ERR_OK) {
                pbuf_free(p);
            }
        }
    }

    rx_desc->status2 = 0;
    uint32_t len_field = (uint32_t)(-1518) & 0x0FFF;
    len_field |= 0xF000;
    rx_desc->status_and_length = 0x80000000 | len_field;
    
    asm volatile("wbinvd" ::: "memory");
}