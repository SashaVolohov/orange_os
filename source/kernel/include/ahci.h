#include <stdint.h>

typedef struct {
    uint32_t clb;
    uint32_t clbu;
    uint32_t fb;
    uint32_t fbu;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} volatile ahci_port_t;

typedef struct {
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t cap2;
    uint32_t bohc;
    uint8_t  rsv[116];
    uint8_t  vendor[96];
    ahci_port_t ports[32];
} volatile ahci_abar_t;

typedef struct {
    uint8_t cfl:5;
    uint8_t a:1;
    uint8_t w:1;
    uint8_t p:1;

    uint8_t b:1;
    uint8_t c:1;
    uint8_t r:1;
    uint8_t pmp:4;
    uint8_t prdtl_low:1;

    uint16_t prdtl;
    
    volatile uint32_t prdbc;
    
    uint32_t ctba;
    uint32_t ctbau;
    
    uint32_t rsv1[4];
} __attribute__((packed)) ahci_cmd_header_t;

typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc:22;
    uint32_t rsv1:9; 
    uint32_t i:1;
} __attribute__((packed)) ahci_prdt_entry_t;

typedef struct {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    ahci_prdt_entry_t prdt[1];
} __attribute__((packed)) ahci_cmd_table_t;

typedef struct {
    uint8_t fis_type;
    
    uint8_t pmport:4;
    uint8_t rsv0:3;
    uint8_t c:1;
    
    uint8_t command;
    uint8_t featurel;
    
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;
    
    uint8_t countl;
    uint8_t counth;
    uint8_t icc;
    uint8_t control;
    
    uint32_t rsv1;
} __attribute__((packed)) fis_reg_h2d_t;

typedef struct {
    uint16_t config;
    uint16_t reserved1[82];
    uint16_t command_sets;
    uint16_t reserved2[16];
    uint64_t lba48_sectors;
    uint16_t reserved3[152];
} __attribute__((packed)) ahci_identify_t;

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t device_type      : 5;
    uint8_t peripheral_qual  : 3;

    uint8_t rmb              : 1;
    uint8_t device_type_mod  : 7;

    uint8_t version;
    
    uint8_t response_format  : 4;
    uint8_t hi_sup           : 1;
    uint8_t norm_aca         : 1;
    uint8_t reserved1        : 2;

    uint8_t additional_length;
    
    uint8_t sccs             : 1;
    uint8_t acc              : 1;
    uint8_t tpgs             : 2;
    uint8_t pc3              : 1;
    uint8_t reserved2        : 2;
    uint8_t protect          : 1;

    uint8_t bque             : 1;
    uint8_t enc_serv         : 1;
    uint8_t vs               : 1;
    uint8_t multi_p          : 1;
    uint8_t m_chnl           : 1;
    uint8_t reserved3        : 1;
    uint8_t addr16           : 1;
    uint8_t reserved4        : 1;

    uint8_t vs2              : 1;
    uint8_t cmd_que          : 1;
    uint8_t reserved5        : 6;

    char vendor_id[8];
    char product_id[16];
    char product_rev[4];
    
    uint8_t vendor_specific[20];
    uint8_t reserved6[40];
} satapi_inquiry_data_t;

typedef struct __attribute__((packed)) {
    uint32_t lba_max_big;
    uint32_t bytes_in_one_sector_big;
} satapi_capacity_data_t;

#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_PACKET              0xA0
#define ATA_CMD_IDENTIFY        0xEC

int init_ahci();

int ahci_read(int port, uint64_t lba, uint16_t count, uint64_t target_phys_buffer);
int ahci_write(int port, uint64_t lba, uint16_t count, uint64_t source_phys_buffer);
uint64_t ahci_get_sectors(int port);
int get_ahch_port(int port);
satapi_inquiry_data_t* ahch_atapi_get_info(int port);
int ahci_atapi_get_sectors(int port);
int ahci_wait_port(int port);
int ahci_atapi_test_ready(int port);
int ahci_atapi_request_sense(int port);