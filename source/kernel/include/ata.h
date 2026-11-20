#include <stdint.h>

typedef struct {
    uint64_t type;
    uint64_t sector_count;
} DriveInfo;

typedef struct {
    uint32_t buffer_phys_addr;
    uint16_t byte_count;
    uint8_t  reserved;
    uint8_t  last_prd;
} __attribute__((packed)) prd_t;

extern DriveInfo ata_detect_drive(uint8_t drive_select);
extern void read_ata(uint32_t lba, uint64_t buffer, uint32_t sector_count, uint8_t drive);
extern void write_ata(uint64_t lba, const void* buffer, uint16_t sector_count, uint8_t drive_select);

#define NO_DEVICE 0
#define ATA_DEVICE 1
#define ATAPI_DEVICE 2

#define NO_ATA_CONTROLLER_FOUND 0
#define ATA_CONTROLLER_FOUND 1

#define ATA_SUCCESS 0

#define ATA_SR_ERR  0x01
#define ATA_SR_DF   0x20
#define ATA_SR_DRQ  0x08
#define ATA_SR_BSY  0x80

#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35

int init_ata_dma();

int ata_dma_read_sector(uint64_t lba, uint32_t count, uint64_t target_addr, uint8_t slave);
int ata_dma_write_sector(uint64_t lba, uint32_t count, uint64_t target_addr, uint8_t slave);

int atapi_dma_read_sector(uint64_t lba, uint32_t count, uint64_t target_addr, uint8_t slave);

int get_ata_disk_id_on_bus(int drive_port);
void ata_set_udma3(uint8_t drive);

extern uint8_t ata_irq14_fired;
extern uint8_t ata_irq15_fired;

#define ATA_REG_DATA       0x1F0
#define ATA_REG_FEATURES   0x1F1
#define ATA_REG_LBA_MID    0x1F4
#define ATA_REG_LBA_HIGH   0x1F5
#define ATA_REG_DRV_SEL    0x1F6
#define ATA_REG_COMMAND    0x1F7
#define ATA_REG_STATUS     0x1F7
#define ATA_REG_ALT_STATUS 0x3F6