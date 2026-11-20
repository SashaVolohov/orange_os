#include "cdfs.h"
#include "core.h"
#include "disk.h"

uint32_t root_dir_lba = 0;
uint32_t root_dir_size = 0;

int cdfs_disk_read(int disk_id, uint64_t lba, uint32_t count, uint64_t target_addr) {

    Drive* disk = get_disk(disk_id);

    if(disk->atapi == 0) return disk_read(disk_id, lba*4, count*4, target_addr);
    else return disk_read(disk_id, lba, count, target_addr);
}

int cdfs_mount(int disk_id) {
    uint64_t buf_page = alloc_page();
    if (!buf_page) return -1;

    if (cdfs_disk_read(disk_id, 16, 1, buf_page) != 0) {
        free_page(buf_page);
        return -2;
    }

    struct cdfs_pvd* pvd = (struct cdfs_pvd*)buf_page;
    
    if (pvd->type != 1 || memcmp(pvd->id, "CD001", 5) != 0) {
        free_page(buf_page);
        return -3;
    }

    root_dir_lba = pvd->root_dir_record.extent_lba;
    root_dir_size = pvd->root_dir_record.data_length;

    free_page(buf_page);
    return 0;
}

int cdfs_find_file(int disk_id, uint32_t dir_lba, uint32_t dir_size, const char* filename, uint32_t* out_lba, uint32_t* out_size) {
    uint32_t sectors_to_read = (dir_size + 2047) / 2048;
    if (sectors_to_read == 0) sectors_to_read = 1;
    
    uint8_t* dir_buf = (uint8_t*)kmalloc(sectors_to_read * 2048); 
    
    if (cdfs_disk_read(disk_id, dir_lba, sectors_to_read, (uint64_t)dir_buf) != 0) {
        return -1;
    }

    uint32_t offset = 0;
    while (offset < dir_size) {
        if ((offset % 2048) == 0 && dir_buf[offset] == 0) {
            offset += 0;
            continue;
        }

        struct cdfs_dir_record* record = (struct cdfs_dir_record*)&dir_buf[offset];
        
        if (record->length == 0) {
            offset = ((offset / 2048) + 1) * 2048;
            continue;
        }

        if (record->name_length == 1 && (record->name[0] == 0 || record->name[0] == 1)) {
            offset += record->length;
            continue;
        }

        uint32_t actual_len = record->name_length;
        for (uint32_t i = 0; i < record->name_length; i++) {
            if (record->name[i] == ';') {
                actual_len = i;
                break;
            }
        }

        if (memcmp(record->name, filename, actual_len) == 0 && filename[actual_len] == '\0') {
            *out_lba = record->extent_lba;
            *out_size = record->data_length;
            return 1;
        }

        offset += record->length;
    }

    return 0;
}

int cdfs_read_file_data(int disk_id, uint32_t file_lba, uint32_t file_size, uint64_t target_addr) {
    if (file_size == 0) return 0;
    
    uint32_t total_sectors = (file_size + 2047) / 2048;
    uint32_t sectors_left = total_sectors;
    uint32_t current_lba = file_lba;
    uint64_t current_target = target_addr;

    const uint32_t MAX_SECTORS_PER_CHUNK = 512; 

    while (sectors_left > 0) {
        uint32_t chunk_sectors = (sectors_left > MAX_SECTORS_PER_CHUNK) ? MAX_SECTORS_PER_CHUNK : sectors_left;

        int res = cdfs_disk_read(disk_id, current_lba, chunk_sectors, current_target);
        if (res != 0) {
            return res;
        }

        sectors_left -= chunk_sectors;
        current_lba += chunk_sectors;
        current_target += (uint64_t)chunk_sectors * 2048;
    }

    return 0;
}

int cdfs_get_file_info(int disk_id, const char* filename, uint32_t* out_lba, uint32_t* out_size) {
    if (cdfs_mount(disk_id) != 0) return -101;

    int find_res = cdfs_find_file(disk_id, root_dir_lba, root_dir_size, filename, out_lba, out_size);
    
    if (find_res == 0)  return 0;
    if (find_res < 0)  return -103;
    
    return (int)*out_size;
}

int64_t load_file_from_cdrom(int disk_id, const char* filename, uint64_t target_addr) {

    int mount_res = cdfs_mount(disk_id);
    if (mount_res != 0) {
        return -101; 
    }

    uint32_t file_lba = 0;
    uint32_t file_size = 0;

    int find_res = cdfs_find_file(disk_id, root_dir_lba, root_dir_size, filename, &file_lba, &file_size);
    
    if (find_res == 0) {
        return -102;
    } else if (find_res < 0) {
        return -103;
    }

    uint32_t sectors_count = (file_size + 2047) / 2048;
    uint32_t buffer_size = sectors_count * 2048;
    
    uint8_t* temp_buffer = (uint8_t*)kmalloc(buffer_size);
    if (!temp_buffer) {
        return -104;
    }

    int read_res = cdfs_read_file_data(disk_id, file_lba, file_size, (uint64_t)temp_buffer);
    if (read_res != 0) {
        kfree(temp_buffer);
        return -105;
    }

    memcpy((void*)target_addr, temp_buffer, file_size);

    kfree(temp_buffer);

    return (int)file_size; 
}