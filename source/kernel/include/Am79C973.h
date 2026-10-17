#include <stdint.h>

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

int init_am79c973(am79c973_device_t *dev);
void send_test_arp_packet(uint8_t* my_mac);
void check_incoming_packets();

#define htons(val) ((((val) & 0xFF00) >> 8) | (((val) & 0x00FF) << 8))
#define htonl(val) ((((val) & 0xFF000000) >> 24) | (((val) & 0x00FF0000) >> 8) | \
                    (((val) & 0x0000FF00) << 8)  | (((val) & 0x000000FF) << 24))