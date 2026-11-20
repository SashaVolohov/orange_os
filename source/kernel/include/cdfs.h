#include <stdint.h>

struct cdfs_datetime {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t  gmt_offset;
} __attribute__((packed));

struct cdfs_dir_record {
    uint8_t  length;
    uint8_t  ext_attr_length;
    uint32_t extent_lba;
    uint32_t extent_lba_be;
    uint32_t data_length;
    uint32_t data_length_be;
    uint8_t  recording_time[7];
    uint8_t  flags;
    uint8_t  file_unit_size;
    uint8_t  interleave_gap;
    uint16_t vol_seq_num;
    uint16_t vol_seq_num_be;
    uint8_t  name_length;
    char     name[];
} __attribute__((packed));

struct cdfs_pvd {
    uint8_t  type;
    char     id[5];
    uint8_t  version;
    uint8_t  padding1[33];
    char     volume_id[32];
    uint8_t  padding2[8];
    uint32_t volume_space_size;
    uint32_t volume_space_size_be;
    uint8_t  padding3[32];
    uint16_t volume_set_size;
    uint16_t volume_set_size_be;
    uint16_t volume_seq_num;
    uint16_t volume_seq_num_be;
    uint16_t logical_block_size;
    uint16_t logical_block_size_be;
    uint32_t path_table_size;
    uint32_t path_table_size_be;
    uint32_t path_table_lba;
    uint32_t path_table_optional_lba;
    uint8_t  padding4[8];
    
    struct cdfs_dir_record root_dir_record;
} __attribute__((packed));

int cdfs_get_file_info(int disk_id, const char* filename, uint32_t* out_lba, uint32_t* out_size);
int64_t load_file_from_cdrom(int disk_id, const char* filename, uint64_t target_addr);
int cdfs_read_file_data(int disk_id, uint32_t file_lba, uint32_t file_size, uint64_t target_addr);