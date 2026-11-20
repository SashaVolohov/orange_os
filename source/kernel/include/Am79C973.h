#include <stdint.h>

#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "lwip/etharp.h"
#include "lwip/dns.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"

typedef struct {
    uint32_t io_base;
    uint8_t mac[6];
} am79c973_device_t;

struct Am79C973_TxDescriptor {
    uint32_t buffer_pointer;
    
    uint32_t status_and_length; 
    
    uint32_t status2;
    uint32_t reserved;       
} __attribute__((packed));

struct Am79C973_RxDescriptor {
    uint32_t buffer_pointer;
    
    uint32_t status_and_length; 
    
    uint32_t status2;
    uint32_t reserved;       
} __attribute__((packed));

struct EthernetHeader {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6]; 
    uint16_t ethertype;
} __attribute__((packed));

struct ArpPacket {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t  hardware_len;
    uint8_t  protocol_len;
    uint16_t operation;
    
    uint8_t  src_mac[6];
    uint8_t  src_ip[4];
    uint8_t  dst_mac[6];
    uint8_t  dst_ip[4];
} __attribute__((packed));

struct Am79C973_InitBlock {
    uint16_t mode;
    uint8_t  rlen_tlen;
    uint8_t  reserved1;
    
    uint16_t mac_low;
    uint16_t mac_mid;
    uint16_t mac_high;
    uint16_t reserved2;
    
    uint32_t ladd_low;
    uint32_t ladd_high;
    
    uint32_t recv_ring_pointer;
    uint32_t trans_ring_pointer;
} __attribute__((packed));

struct FullArpPacket {
    struct EthernetHeader eth;
    struct ArpPacket      arp;
} __attribute__((packed));

struct IpHeader {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint8_t  src_ip[4];
    uint8_t  dst_ip[4];
} __attribute__((packed));

struct IcmpHeader {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

struct UdpHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

int init_am79c973(am79c973_device_t *dev);
void send_test_arp_packet(uint8_t* my_mac);
void check_incoming_packets();

void am79c973_send_packet(const uint8_t* packet_data, uint32_t length);

extern struct netif g_am79c973_netif;
err_t am79c973_netif_init(struct netif *netif);

#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IPV4 0x0800

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

#define IP_PROTO_ICMP 1