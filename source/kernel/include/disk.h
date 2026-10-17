#include <stdint.h>

#define MAX_PARTITIONS_PER_DRIVE 4

typedef struct {
    uint64_t drive_index;
    uint64_t start_lba;
    uint64_t sector_count;
    uint8_t  fs_type;
    uint8_t  is_active;
} Partition;

typedef struct {
    uint64_t type; // 0 - IDE, 1 - SATA
    uint8_t port; // IDE - Master/Slave, SATA - port number
    uint8_t port2; // IDE - Primary/Secondary, SATA - reserved
    uint64_t sector_count;
    uint8_t atapi; // 0 - No ATAPI, 1 - ATAPI
} Drive;

#pragma pack(push, 1)
typedef struct {
    uint8_t  boot_indicator;
    uint8_t  starting_chs[3];
    uint8_t  os_type;
    uint8_t  ending_chs[3];
    uint32_t starting_lba;
    uint32_t size_in_sectors;
} MbrPartitionEntry;
#pragma pack(pop)

Drive* get_disk(int drive);
int get_drives_count();
void registerATADevices();
void registerSATADevices();
int disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr);
int disk_write(int disk_id, uint64_t lba, uint32_t count, uint64_t source_addr);

void partition_scan_all_drives();
Partition* get_partition(int partition);
int get_partition_count();