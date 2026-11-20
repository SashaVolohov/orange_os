#include "smbios.h"
#include "core.h"

void parse_smbios_tables(uint64_t table_addr, uint32_t table_len) {
    uint8_t *ptr = (uint8_t *)table_addr;
    uint8_t *end = ptr + table_len;

    g_sys_info.ram_slot_count = 0;
    g_sys_info.total_ram_bytes = 0;

    while (ptr < end) {
        struct smbios_header *header = (struct smbios_header *)ptr;

        if (header->type == 17) { 
            struct smbios_type_17 *ram = (struct smbios_type_17 *)ptr;
            
            if (header->length >= 0x26 && g_sys_info.ram_slot_count < MAX_RAM_SLOTS) { 
                uint16_t current_speed = ram->configured_memory_speed;
                if (current_speed == 0 || current_speed == 0xFFFF) {
                    current_speed = ram->speed; 
                }

                if (current_speed != 0 && current_speed != 0xFFFF) {
                    uint32_t slot = g_sys_info.ram_slot_count;
                    g_sys_info.ram_slots[slot].speed_mhz = current_speed;
                    
                    g_sys_info.ram_slot_count++;
                }
            }
        }

        ptr += header->length;
        while (!(*ptr == 0 && *(ptr + 1) == 0)) { ptr++; }
        ptr += 2;
    }
}

void init_smbios() {
    uint8_t *bios_mem = (uint8_t *)0x000F0000;
    
    for (size_t i = 0; i < 0x10000; i += 16) {

        if (memcmp(&bios_mem[i], "_SM3_", 5) == 0) {
            struct smbios_entry_point_64 *ep = (struct smbios_entry_point_64 *)&bios_mem[i];
            parse_smbios_tables(ep->table_address, ep->table_max_size);
            return;
        }
        
        if (memcmp(&bios_mem[i], "_SM_", 4) == 0) {
            struct smbios_entry_point_32 *ep = (struct smbios_entry_point_32 *)&bios_mem[i];
            parse_smbios_tables(ep->table_address, ep->table_length);
            return;
        }

    }
}