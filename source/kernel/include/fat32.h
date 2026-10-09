#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)

typedef struct {
    uint8_t  jmp[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    // Расширение FAT32 (EBPB)
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
    uint8_t  boot_code[420];
    uint16_t boot_magic;
} __attribute__((packed)) fat32_vbr_t;

typedef struct {
    uint32_t lead_signature;
    uint8_t  reserved1[480];
    uint32_t struct_signature;
    uint32_t free_cluster_count;
    uint32_t next_free_cluster;
    uint8_t  reserved2[12];
    uint32_t trail_signature;
} fat32_fsinfo_t;

typedef struct {
    uint8_t  name[11];
    uint8_t  attr;
    uint8_t  nt_res;
    uint8_t  crt_time_tenth;
    uint16_t crt_time;
    uint16_t crt_date;
    uint16_t lst_acc_date;
    uint16_t first_cluster_hi;
    uint16_t wrt_time;
    uint16_t wrt_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} FAT32_DirectoryEntry;

#pragma pack(pop)

uint8_t get_sectors_per_cluster(uint32_t total_sectors);
int fat32_write_file(uint32_t disk_id, uint32_t start_lba, const char* filename, const uint64_t* data, uint32_t size);