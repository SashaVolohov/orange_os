#include <stdint.h>

struct smbios_entry_point_32 {
    char signature[4];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint16_t max_structure_size;
    uint8_t entry_point_revision;
    uint8_t formatted_area[5];
    char intermediate_signature[5];
    uint8_t intermediate_checksum;
    uint16_t table_length;
    uint32_t table_address;
    uint16_t number_of_structures;
    uint8_t bcd_revision;
} __attribute__((packed));

struct smbios_entry_point_64 {
    char signature[5];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t doc_rev;
    uint8_t entry_point_revision;
    uint8_t reserved;
    uint32_t table_max_size;
    uint64_t table_address;
} __attribute__((packed));

struct smbios_header {
    uint8_t type;
    uint8_t length;
    uint16_t handle;
} __attribute__((packed));

struct smbios_type_17 {
    struct smbios_header header;
    uint16_t physical_memory_array_handle;
    uint16_t memory_error_information_handle;
    uint16_t total_width;
    uint16_t data_width;
    uint16_t size;
    uint8_t form_factor;
    uint8_t device_set;
    uint8_t device_locator_str_index;
    uint8_t bank_locator_str_index;
    uint8_t memory_type;
    uint16_t type_detail;
    uint16_t speed;
    uint8_t manufacturer_str_index;
    uint8_t serial_number_str_index;
    uint8_t asset_tag_str_index;
    uint8_t part_number_str_index;
    uint8_t attributes;
    uint32_t extended_size;
    uint16_t configured_memory_speed;
} __attribute__((packed));

void init_smbios();