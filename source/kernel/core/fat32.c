#include "core.h"
#include "disk.h"
#include "fat32.h"

uint8_t get_sectors_per_cluster(uint32_t total_sectors) {
    uint64_t size_mb = ((uint64_t)total_sectors * 512) / (1024 * 1024);
    if (size_mb < 64)       return 1;
    if (size_mb < 260)      return 2;
    if (size_mb < 512)      return 4;
    if (size_mb < 8192)     return 8;
    if (size_mb < 16384)    return 16;
    if (size_mb < 32768)    return 32;
    return 64;
}

uint32_t cluster_to_lba(fat32_vbr_t* bpb, uint32_t start_lba, uint32_t cluster) {
    uint32_t fat_size_sectors = bpb->sectors_per_fat_32;
    uint32_t first_data_sector = start_lba + bpb->reserved_sectors + (bpb->fat_count * fat_size_sectors);
    return first_data_sector + ((cluster - 2) * bpb->sectors_per_cluster);
}

int fat32_write_file(uint32_t disk_id, uint32_t start_lba, const char* filename, const uint64_t* data, uint32_t size) {
    uint64_t sector_buf[512];

    if (disk_read(disk_id, start_lba, 1, (uint64_t)sector_buf) != 0) return -1;
    fat32_vbr_t* bpb = (fat32_vbr_t*)sector_buf;
    
    uint32_t bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;
    uint32_t clusters_needed = (size + bytes_per_cluster - 1) / bytes_per_cluster;
    if (clusters_needed == 0) clusters_needed = 1;

    uint32_t fat_lba = start_lba + bpb->reserved_sectors;
    
    uint32_t* fat_table = kmalloc(bpb->bytes_per_sector);
    
    uint32_t* allocated_clusters = kmalloc(clusters_needed * sizeof(uint32_t));
    uint32_t found_clusters = 0;
    
    for (uint32_t i = 0; i < bpb->sectors_per_fat_32 && found_clusters < clusters_needed; i++) {
        disk_read(disk_id, fat_lba + i, 1, (uintptr_t)fat_table);
        for (uint32_t j = 0; j < 128 && found_clusters < clusters_needed; j++) {
            uint32_t cluster_idx = (i * 128) + j;
            if (cluster_idx < 2) continue;
            
            if (fat_table[j] == 0x00000000) {
                allocated_clusters[found_clusters++] = cluster_idx;
            }
        }
    }
    
    if (found_clusters < clusters_needed) {
        kfree(fat_table); 
        kfree(allocated_clusters);
        return -2;
    }

    for (uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t current_cluster = allocated_clusters[i];
        uint32_t next_cluster_val = (i == clusters_needed - 1) ? 0x0FFFFFFF : allocated_clusters[i+1];
        
        uint32_t target_fat_sector = fat_lba + (current_cluster * 4) / bpb->bytes_per_sector;
        uint32_t entry_offset = (current_cluster * 4) % bpb->bytes_per_sector / 4;
        
        disk_read(disk_id, target_fat_sector, 1, (uintptr_t)fat_table);
        fat_table[entry_offset] = next_cluster_val;
        disk_write(disk_id, target_fat_sector, 1, (uintptr_t)fat_table);
        if (bpb->fat_count == 2) {
            disk_write(disk_id, target_fat_sector + bpb->sectors_per_fat_32, 1, (uintptr_t)fat_table);
        }
    }

    uint32_t bytes_left = size;
    const uint8_t* data_ptr = (const uint8_t*)data;
    for (uint32_t i = 0; i < clusters_needed; i++) {
        uint32_t cl_lba = cluster_to_lba(bpb, start_lba, allocated_clusters[i]);
        uint32_t write_size = (bytes_left > bytes_per_cluster) ? bytes_per_cluster : bytes_left;
        
        uint64_t* cluster_buf = kcalloc(1, bytes_per_cluster);
        memcpy(cluster_buf, data_ptr, write_size);
        
        disk_write(disk_id, cl_lba, bpb->sectors_per_cluster, (uintptr_t)cluster_buf);
        
        kfree(cluster_buf);
        data_ptr += write_size;
        bytes_left -= write_size;
    }

    uint32_t root_lba = cluster_to_lba(bpb, start_lba, bpb->root_cluster);
    FAT32_DirectoryEntry* dir = (FAT32_DirectoryEntry*)sector_buf;
    
    disk_read(disk_id, root_lba, 1, (uintptr_t)sector_buf);
    
    char fat_name[11];
    memset(fat_name, ' ', 11);
    
    int name_len = 0;
    while (filename[name_len] != '\0' && filename[name_len] != '.' && name_len < 8) {
        fat_name[name_len] = filename[name_len];
        name_len++;
    }
    
    const char* ext = filename;
    while (*ext != '\0' && *ext != '.') ext++;
    if (*ext == '.') {
        ext++;
        int ext_len = 0;
        while (ext[ext_len] != '\0' && ext_len < 3) {
            fat_name[8 + ext_len] = ext[ext_len];
            ext_len++;
        }
    }

    int entry_found = 0;
    for (int i = 0; i < 16; i++) {
        if (dir[i].name[0] == 0x00 || dir[i].name[0] == 0xE5) {
            memset(&dir[i], 0, sizeof(FAT32_DirectoryEntry));
            
            memcpy(dir[i].name, fat_name, 11);
            dir[i].attr = 0x20;
            dir[i].first_cluster_hi = (allocated_clusters[0] >> 16) & 0xFFFF;
            dir[i].first_cluster_lo = allocated_clusters[0] & 0xFFFF;
            dir[i].file_size = size;
            
            entry_found = 1;
            break;
        }
    }
    
    if (entry_found) {
        disk_write(disk_id, root_lba, 1, (uintptr_t)sector_buf);
    }

    kfree(fat_table);
    kfree(allocated_clusters);
    return entry_found ? 0 : -3;
}