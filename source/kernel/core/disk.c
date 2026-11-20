#include "ata.h"
#include "ahci.h"
#include "nvme.h"
#include "disk.h"
#include "pci.h"
#include "xhci.h"

Drive drives[64];
int total_drives = 0;
Partition partitions[256];

int total_partitions = 0;

static cache_entry_t* free_list = NULL;

static cache_entry_t* hash_table[HASH_TABLE_SIZE];
static cache_entry_t* lru_head = NULL;
static cache_entry_t* lru_tail = NULL;

static spinlock_t cache_lock;

static uint64_t last_sample_tick = 0;

int raw_disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr) {

    if(disk_id < 0 || disk_id >= 32) return -1;

    if(drives[disk_id].type == 0) { // IDE
        if(drives[disk_id].atapi == 0) {
            return ata_dma_read_sector(lba, count, target_addr, get_ata_disk_id_on_bus(drives[disk_id].port));
        } else {
            read_atapi_sector((void*)target_addr, lba, count, drives[disk_id].port);
            return 0;
        }
    } else if(drives[disk_id].type == 1) { // SATA
        if(drives[disk_id].atapi == 0) {
            return ahci_read(drives[disk_id].port, lba, count, target_addr);
        } else {
            return !ahci_atapi_read(drives[disk_id].port, lba, count, (void*)target_addr);
        }
    } else if(drives[disk_id].type == 2) { // NVMe
        return !nvme_read_sectors(lba, count, (void*)target_addr, drives[disk_id].port);
    } else if(drives[disk_id].type == 3) { // USB

        uint64_t current_addr = target_addr;

        for (uint32_t i = 0; i < count; i++) {
            int res = xhci_disk_read_sector(drives[disk_id].port, lba + i, current_addr);
            if (res != 0) {
                return res;
            }
            current_addr += 512;
        }
        return 0;
    }

    return -1;

}

int raw_disk_write(int disk_id, uint64_t lba, uint32_t count, uint64_t source_addr) {

    if(disk_id < 0 || disk_id >= 32) return -1;

    if(drives[disk_id].type == 0) { // IDE
        if(drives[disk_id].atapi == 0) {
            return ata_dma_write_sector(lba, count, source_addr, get_ata_disk_id_on_bus(drives[disk_id].port));
        } else {
            return -1;
        }
    } else if(drives[disk_id].type == 1) { // SATA
        return ahci_write(drives[disk_id].port, lba, count, source_addr);
    } else if(drives[disk_id].type == 2) { // NVMe
        return !nvme_write_sectors(lba, count, (void*)source_addr, drives[disk_id].port);
    }

    return -1;

}

int get_disk_sector_size(int disk_id) {
    if(disk_id < 0 || disk_id >= 32) return -1;
    if(drives[disk_id].atapi == 1) return 2048;
    else return 512;
}

uint8_t is_disk_read_only(int disk_id) {
    if(disk_id < 0 || disk_id >= 32) return -1;
    return drives[disk_id].atapi;
}

void disk_cache_init() {
    for (size_t i = 0; i < MAX_CACHE_BLOCKS; i++) {
        cache_entry_t* entry = (cache_entry_t*)kmalloc(sizeof(cache_entry_t));
        entry->data = (uint8_t*)alloc_page(); 
        
        entry->is_valid = 0;
        entry->is_dirty = 0;
        entry->is_busy = 0;
        entry->pin_count = 0;

        entry->next = free_list;
        free_list = entry;
    }
}

static cache_entry_t* alloc_new_entry(void) {
    if (!free_list) {
        return NULL;
    }

    cache_entry_t* entry = free_list;
    free_list = free_list->next;

    entry->prev = NULL;
    entry->next = NULL;
    entry->hash_next = NULL;
    return entry;
}

static inline uint32_t get_hash(int disk_id, uint64_t aligned_lba) {
    uint64_t key = ((uint64_t)(uint32_t)disk_id * 2654435761ULL) ^ (aligned_lba >> 1);
    key ^= (key >> 16);
    return (uint32_t)(key & (HASH_TABLE_SIZE - 1));
}

static void hash_insert(cache_entry_t* entry) {
    uint32_t hash = get_hash(entry->disk_id, entry->aligned_lba);
    
    entry->hash_next = hash_table[hash];
    hash_table[hash] = entry;
}

static void hash_remove(cache_entry_t* entry) {
    uint32_t hash = get_hash(entry->disk_id, entry->aligned_lba);
    cache_entry_t* curr = hash_table[hash];
    cache_entry_t* prev = NULL;

    while (curr) {
        if (curr == entry) {
            if (prev) {
                prev->hash_next = curr->hash_next;
            } else {
                hash_table[hash] = curr->hash_next;
            }
            entry->hash_next = NULL;
            return;
        }
        prev = curr;
        curr = curr->hash_next;
    }
}

static cache_entry_t* find_eviction_candidate(void) {
    cache_entry_t* curr = lru_tail;
    while (curr) {
        if (!curr->is_busy && curr->pin_count == 0) {
            return curr;
        }
        curr = curr->prev;
    }
    return NULL;
}

static void lru_touch(cache_entry_t* entry) {
    if (entry == lru_head) return;

    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (entry == lru_tail) lru_tail = entry->prev;

    entry->prev = NULL;
    entry->next = lru_head;
    if (lru_head) lru_head->prev = entry;
    lru_head = entry;
    if (!lru_tail) lru_tail = entry;
}

static void lru_remove(cache_entry_t* entry) {
    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (entry == lru_head) lru_head = entry->next;
    if (entry == lru_tail) lru_tail = entry->prev;

    entry->prev = NULL;
    entry->next = NULL;
}

static cache_entry_t* cache_get_block(int disk_id, uint64_t aligned_lba, uint32_t sector_size, uint8_t skip_read) {
    uint32_t sectors_per_block = CACHE_BLOCK_SIZE / sector_size;

retry:
    spin_lock(&cache_lock);

    uint32_t hash = get_hash(disk_id, aligned_lba);
    cache_entry_t* curr = hash_table[hash];
    while (curr) {
        if (curr->disk_id == disk_id && curr->aligned_lba == aligned_lba) {
            if (curr->is_busy) {
                spin_unlock(&cache_lock);
                asm volatile("pause");
                goto retry;
            }
            if (curr->is_valid) {
                curr->pin_count++;
                lru_touch(curr);
                spin_unlock(&cache_lock);
                return curr;
            }
        }
        curr = curr->hash_next;
    }

    cache_entry_t* entry = alloc_new_entry();
    if (!entry) {
        entry = find_eviction_candidate();
        if (!entry) {
            spin_unlock(&cache_lock);
            asm volatile("pause");
            goto retry;
        }

        if (entry->is_valid && entry->is_dirty) {
            entry->is_busy = 1;
            int old_disk = entry->disk_id;
            uint64_t old_lba = entry->aligned_lba;
            uint32_t old_sectors = CACHE_BLOCK_SIZE / entry->sector_size;

            spin_unlock(&cache_lock);
            int write_res = raw_disk_write(old_disk, old_lba, old_sectors, (uint64_t)entry->data);
            spin_lock(&cache_lock);

            entry->is_busy = 0;
            if (write_res != 0) {
                spin_unlock(&cache_lock);
                return NULL;
            }
            entry->is_dirty = 0;
        }

        hash_remove(entry);
    }

    entry->disk_id = disk_id;
    entry->aligned_lba = aligned_lba;
    entry->sector_size = sector_size;
    entry->is_valid = 0;
    entry->is_dirty = 0;
    entry->pin_count = 1;

    if (skip_read) {
        entry->is_busy = 0;
        entry->is_valid = 1;
        hash_insert(entry);
        lru_touch(entry);
        spin_unlock(&cache_lock);
        return entry;
    }

    entry->is_busy = 1;
    hash_insert(entry);
    lru_touch(entry);
    spin_unlock(&cache_lock);

    int read_res = raw_disk_read(disk_id, aligned_lba, sectors_per_block, (uint64_t)entry->data);

    spin_lock(&cache_lock);
    entry->is_busy = 0;

    if (read_res != 0) {
        hash_remove(entry);
        entry->pin_count = 0;
        entry->is_valid = 0;
        spin_unlock(&cache_lock);
        return NULL;
    }

    entry->is_valid = 1;
    spin_unlock(&cache_lock);

    return entry;
}

static void cache_unpin_block(cache_entry_t* entry, uint8_t mark_dirty) {
    spin_lock(&cache_lock);
    if (mark_dirty) {
        entry->is_dirty = 1;
    }
    if (entry->pin_count > 0) {
        entry->pin_count--;
    }
    spin_unlock(&cache_lock);
}

int disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr) {
    uint32_t sector_size = get_disk_sector_size(disk_id);
    if (sector_size == 0 || CACHE_BLOCK_SIZE % sector_size != 0) {
        return raw_disk_read(disk_id, lba, count, target_addr);
    }

    uint32_t sectors_per_block = CACHE_BLOCK_SIZE / sector_size;
    uint8_t* dst = (uint8_t*)target_addr;
    uint32_t sectors_left = count;
    uint64_t current_lba = lba;

    while (sectors_left > 0) {
        uint64_t aligned_lba = current_lba & ~(uint64_t)(sectors_per_block - 1);
        uint32_t offset_sectors = (uint32_t)(current_lba - aligned_lba);
        uint32_t copy_sectors = sectors_per_block - offset_sectors;

        if (copy_sectors > sectors_left) {
            copy_sectors = sectors_left;
        }

        cache_entry_t* block = cache_get_block(disk_id, aligned_lba, sector_size, 0);
        if (!block) return -1;

        memcpy(dst, block->data + (offset_sectors * sector_size), copy_sectors * sector_size);

        cache_unpin_block(block, 0);

        dst += copy_sectors * sector_size;
        current_lba += copy_sectors;
        sectors_left -= copy_sectors;
    }

    return 0;
}

int disk_write(int disk_id, uint64_t lba, uint32_t count, uint64_t source_addr) {
    if (is_disk_read_only(disk_id)) {
        return -1; 
    }

    uint32_t sector_size = get_disk_sector_size(disk_id);
    if (sector_size == 0 || CACHE_BLOCK_SIZE % sector_size != 0) {
        return raw_disk_write(disk_id, lba, count, source_addr);
    }

    uint32_t sectors_per_block = CACHE_BLOCK_SIZE / sector_size;
    const uint8_t* src = (const uint8_t*)source_addr;
    uint32_t sectors_left = count;
    uint64_t current_lba = lba;

    while (sectors_left > 0) {
        uint64_t aligned_lba = current_lba & ~(uint64_t)(sectors_per_block - 1);
        uint32_t offset_sectors = (uint32_t)(current_lba - aligned_lba);
        uint32_t copy_sectors = sectors_per_block - offset_sectors;

        if (copy_sectors > sectors_left) {
            copy_sectors = sectors_left;
        }

        uint8_t full_block_overwrite = (offset_sectors == 0 && copy_sectors == sectors_per_block);

        cache_entry_t* block = cache_get_block(disk_id, aligned_lba, sector_size, full_block_overwrite);
        if (!block) return -1;

        memcpy(block->data + (offset_sectors * sector_size), src, copy_sectors * sector_size);

        cache_unpin_block(block, 1);

        src += copy_sectors * sector_size;
        current_lba += copy_sectors;
        sectors_left -= copy_sectors;
    }

    return 0;
}

void registerDisk(uint64_t type, uint8_t port, uint8_t port2, uint64_t sector_count, uint8_t atapi) {
    drives[total_drives].type = type;
    drives[total_drives].port = port;
    drives[total_drives].port2 = port2;
    drives[total_drives].sector_count = sector_count;
    drives[total_drives].atapi = atapi;

    total_drives++;
}

Drive* get_disk(int drive) {
    if(drive >= 64) return 0;

    return &drives[drive];
}

int get_drives_count() {
    return total_drives;
}

void registerATADevices() {

    const int drive_ports[] = {0xA0, 0xB0};
    const int num_drives = sizeof(drive_ports) / sizeof(drive_ports[0]);

    for (int i = 0; i < num_drives; i++) {
        DriveInfo info = ata_detect_drive(drive_ports[i]);
        if (info.type == 1) {
            registerDisk(0, drive_ports[i], 0, info.sector_count, 0);
            ata_set_udma3(drive_ports[i]);
        } else if(info.type == 2) {
            registerDisk(0, drive_ports[i], 0, info.sector_count, 1);
        }
    }

}

void registerSATADevices() {

    for(int i = 0; i < 32; i++) {
        int sata_port = get_ahch_port(i);
        if(sata_port == 1) {
            registerDisk(1, i, 0, ahci_get_sectors(i), 0);
        }
    }

}

void registerSATAPIDevices() {

    for(int i = 0; i < 32; i++) {
        int sata_port = get_ahch_port(i);
        if(sata_port == 2) {
            registerDisk(1, i, 0, get_satapi_sectors(i), 1);
        }
    }

}

void add_logical_partition(int disk_id, int64_t starting_lba, int64_t size_in_sectors, int64_t fs_type, int8_t bootable) {
    partitions[total_partitions].drive_index = disk_id;
    partitions[total_partitions].fs_type = fs_type;
    partitions[total_partitions].is_active = bootable;
    partitions[total_partitions].sector_count = size_in_sectors;
    partitions[total_partitions].start_lba = starting_lba;
    total_partitions++;
}

extern uint32_t* fb_back_buffer;
int test_very_y = 0;

void partition_scan_all_drives() {
    uint8_t sector_buf[512];
    
    for (int d = 0; d < total_drives; d++) {

        kprintf(15, test_very_y, 0xFF, 0xFF, 0xFF, 0xFF, "Scanning disk %d of type %d on port %d...", d, drives[d].type, drives[d].port);
        test_very_y += 15;
        if (drives[d].atapi == 1) {
            kprintf(15, test_very_y, 0xFF, 0xFF, 0xFF, 0xFF, "Added ATAPI partition LBA 0, sectors %d, type 0x96 on drive %d", drives[d].sector_count, d);
            test_very_y += 15;
            add_logical_partition(d, 0, drives[d].sector_count, 0x96, 1);
            continue;
        }
        
        int status = disk_read(d, 0, 1, (uint64_t)sector_buf);
        if(status != 0) {
            kprintf(15, 115, 0xFF, 0xFF, 0xFF, 0xFF, "Scanning disk %d of type %d on port %d... - ERROR! %d", d, drives[d].type, drives[d].port, status);
            test_very_y += 15;
        }
        
        if (sector_buf[510] == 0x55 && sector_buf[511] == 0xAA) {
            MbrPartitionEntry* table = (MbrPartitionEntry*)&sector_buf[0x1BE];
            
            for (int i = 0; i < 4; i++) {
                if (table[i].size_in_sectors > 0) {
                    kprintf(15, test_very_y, 0xFF, 0xFF, 0xFF, 0xFF, "Added partition LBA %d, sectors %d, type %d on drive %d", table[i].starting_lba, table[i].size_in_sectors, table[i].os_type, d);
                    test_very_y += 15;
                    add_logical_partition(d, table[i].starting_lba, table[i].size_in_sectors, table[i].os_type, (table[i].boot_indicator == 0x80) ? 1 : 0);
                }
            }
        }
    }
}

Partition* get_partition(int partition) {
    if(partition >= 256) return 0;
    return &partitions[partition];
}

int get_partition_count() {
    return total_partitions;
}

int disk_sync(int disk_id) {
    int final_result = 0;

    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
    rescan_bucket:
        spin_lock(&cache_lock);

        cache_entry_t* curr = hash_table[i];
        while (curr) {
            if (curr->is_valid && curr->is_dirty && (disk_id == -1 || curr->disk_id == disk_id)) {
                
                if (curr->is_busy || curr->pin_count > 0) {
                    spin_unlock(&cache_lock);
                    asm volatile("pause");
                    goto rescan_bucket;
                }

                curr->is_busy = 1;
                curr->pin_count++;

                int target_disk = curr->disk_id;
                uint64_t target_lba = curr->aligned_lba;
                uint32_t sectors_count = CACHE_BLOCK_SIZE / curr->sector_size;
                uint64_t data_addr = (uint64_t)curr->data;

                spin_unlock(&cache_lock);

                int res = raw_disk_write(target_disk, target_lba, sectors_count, data_addr);

                spin_lock(&cache_lock);

                curr->is_busy = 0;
                if (curr->pin_count > 0) {
                    curr->pin_count--;
                }

                if (res == 0) {
                    curr->is_dirty = 0;
                } else {
                    final_result = -1;
                    curr->is_dirty = 0; 
                }

                spin_unlock(&cache_lock);

                asm volatile("pause");
                goto rescan_bucket;
            }

            curr = curr->hash_next;
        }

        spin_unlock(&cache_lock);
    }

    return final_result;
}

void disk_invalidate(int disk_id) {
retry:
    spin_lock(&cache_lock);

    for (size_t i = 0; i < HASH_TABLE_SIZE; i++) {
        cache_entry_t* curr = hash_table[i];
        while (curr) {
            cache_entry_t* next = curr->hash_next;

            if (curr->disk_id == disk_id) {
                if (curr->is_busy || curr->pin_count > 0) {
                    spin_unlock(&cache_lock);
                    asm volatile("pause");
                    goto retry;
                }

                hash_remove(curr);
                lru_remove(curr);

                curr->is_valid = 0;
                curr->is_dirty = 0;
                curr->next = free_list;
                free_list = curr;
            }

            curr = next;
        }
    }

    spin_unlock(&cache_lock);
}

void kernel_cache_sync_thread() {
    while (1) {
        if (cmos_ticks - last_sample_tick >= 3000) {
            last_sample_tick = cmos_ticks;
            disk_sync(-1);
        }
        asm volatile("hlt");
    }
}