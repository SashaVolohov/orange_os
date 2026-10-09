#include <stdint.h>

typedef struct {
    uint64_t type; // 0 - IDE, 1 - SATA
    uint8_t port; // IDE - Master/Slave, SATA - port number
    uint8_t port2; // IDE - Primary/Secondary, SATA - reserved
    uint64_t sector_count;
    uint8_t atapi; // 0 - No ATAPI, 1 - ATAPI
} Drive;

Drive* get_disk(int drive);
int get_drives_count();
void registerATADevices();
void registerSATADevices();
int disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr);
int disk_write(int disk_id, uint64_t lba, uint32_t count, uint64_t source_addr);