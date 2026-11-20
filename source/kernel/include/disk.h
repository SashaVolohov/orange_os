#include <stdint.h>

#include "core.h"

#define MAX_PARTITIONS_PER_DRIVE 4

typedef struct {
    uint64_t drive_index;
    uint64_t start_lba;
    uint64_t sector_count;
    uint8_t  fs_type;
    uint8_t  is_active;
} Partition;

typedef struct {
    uint64_t type; // 0 - IDE, 1 - SATA, 2 - NVMe, 3 - USB
    uint8_t port; // IDE - Master/Slave, SATA - port number, NVMe - virtual NVMe id, USB - slot id
    uint8_t port2; // IDE - Primary/Secondary, SATA - reserved, NVMe - reserved, USB - reserved
    uint64_t sector_count;
    uint8_t atapi; // 0 - No ATAPI, 1 - ATAPI
} __attribute__((packed)) Drive;

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

#define SECTOR_SIZE      512
#define CACHE_BLOCK_SIZE 4096
#define SECTORS_PER_BLOCK (CACHE_BLOCK_SIZE / SECTOR_SIZE)
#define HASH_TABLE_SIZE  1024
#define MAX_CACHE_BLOCKS 1024

typedef struct cache_entry {
    int disk_id;
    uint64_t aligned_lba;
    uint32_t sector_size;
    uint8_t* data;
    uint8_t is_dirty;
    uint8_t is_valid;
    uint8_t is_busy;
    spinlock_t block_lock;
    uint32_t pin_count;

    struct cache_entry* prev;
    struct cache_entry* next;
    struct cache_entry* hash_next;
} cache_entry_t;

Drive* get_disk(int drive);
int get_drives_count();
void registerATADevices();
void registerSATADevices();
void registerSATAPIDevices();
int disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr);
int disk_write(int disk_id, uint64_t lba, uint32_t count, uint64_t source_addr);

void partition_scan_all_drives();
Partition* get_partition(int partition);
int get_partition_count();
void registerDisk(uint64_t type, uint8_t port, uint8_t port2, uint64_t sector_count, uint8_t atapi);
void disk_cache_init();
void kernel_cache_sync_thread();
int disk_sync(int disk_id);