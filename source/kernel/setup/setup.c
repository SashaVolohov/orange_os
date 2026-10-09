#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "ata.h"
#include "ahci.h"
#include "disk.h"
#include "core.h"
#include "os_memory.h"
#include "fat32.h"
#include "pci.h"
#include "nvme.h"
#include "xhci.h"

typedef struct {
    char name[40];
    uint64_t size_mb;
    uint64_t sector_count;
    uint64_t start_lba;
    uint8_t is_partition;
    uint8_t mbr_index;
} InstallItem;

typedef struct {
    MBR_PartitionEntry entry;
    int original_index;
} SortedPartition;

const char* fs_options[] = {
    "Format the partition using the FAT32 file system (Quick)",
    "Format the partition using the FAT32 file system"
};

static unsigned char mbr_buffer[512];
InstallItem items[9];

int total_items = 1;
int current_selection = 0;

int compare_partitions(const void* a, const void* b) {
    uint32_t lba_a = ((SortedPartition*)a)->entry.start_lba;
    uint32_t lba_b = ((SortedPartition*)b)->entry.start_lba;
    if (lba_a < lba_b) return -1;
    if (lba_a > lba_b) return 1;
    return 0;
}

void Hold() {
    while (1) {
        asm volatile("hlt");
    }
}

void SetupATAError() {

    clear_setup_screen();

    t_print("The IDE controller returned an error during a read/write attempt.", 3, 4, 0x1F);
    t_print("The installation program cannot proceed.", 3, 5, 0x1F);
    update_status_bar("F3=Exit");
    Hold();
}

void tprintf(uint8_t x, uint8_t y, uint8_t color, const char *fmt, ...) {
    char buffer[1024];
    char *out = buffer;
    
    va_list args;
    va_start(args, fmt);

    int original_x = x;
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == '\0') break;
            
            int left_justify = 0;
            if (*fmt == '-') {
                left_justify = 1;
                fmt++;
            }
            
            int width = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }
            
            char temp_buf[256]; 
            char *src = temp_buf;
            int len = 0;

            switch (*fmt) {
                case 'c': {
                    temp_buf[0] = (char)va_arg(args, int);
                    temp_buf[1] = '\0';
                    len = 1;
                    break;
                }
                case 's': {
                    char *s = va_arg(args, char*);
                    if (!s) s = "(null)";
                    src = s;
                    while (src[len]) len++;
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    int_to_str(d, temp_buf, 10, 1);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'x': {
                    unsigned int val_x = va_arg(args, unsigned int);
                    int_to_str(val_x, temp_buf, 16, 0);
                    while (temp_buf[len]) len++;
                    break;
                }
                case '%': {
                    temp_buf[0] = '%';
                    temp_buf[1] = '\0';
                    len = 1;
                    break;
                }
                default: {
                    *out++ = '%';
                    if (left_justify) *out++ = '-';
                    *out++ = *fmt;
                    fmt++;
                    continue; 
                }
            }

            int padding = width - len;
            
            if (!left_justify && padding > 0) {
                for (int i = 0; i < padding; i++) {
                    *out++ = ' ';
                }
            }

            for (int i = 0; i < len; i++) {
                *out++ = src[i];
            }

            if (left_justify && padding > 0) {
                for (int i = 0; i < padding; i++) {
                    *out++ = ' ';
                }
            }

        } else if (*fmt == '\n') {
            *out = '\0';
            if (out != buffer) {
                t_print(buffer, x, y, color);
            }
            out = buffer;
            
            x = original_x;
            y += 1;
        } else {
            *out++ = *fmt;
        }
        fmt++;
    }
    
    va_end(args);
    
    *out = '\0';
    if (out != buffer) {
        t_print(buffer, x, y, color);
    }
}

int DetectATADevice(int type) {
    const int drive_ports[] = {0xA0, 0xB0};
    const int num_drives = sizeof(drive_ports) / sizeof(drive_ports[0]);

    for (int i = 0; i < num_drives; i++) {
        DriveInfo info = ata_detect_drive(drive_ports[i]);
        if (info.type == type) {
            return drive_ports[i];
        }
    }

    return 0x00;
}

void ShowExit() {
    
    clear_setup_screen();
    t_print("You declined the installation of OrangeOS.", 3, 4, 0x1F);
    t_print("The installation program cannot proceed.", 3, 5, 0x1F);
    update_status_bar("Reboot your computer now.");

    Hold();
}

int parse_partitions(void* sector_buffer, int64_t total_disk_sectors) {
    MBR_Sector* mbr = (MBR_Sector*)sector_buffer;

    if (mbr->boot_signature != 0xAA55) {
        items[0].is_partition = 0;
        strcpy(items[0].name, "Unpartitioned space"); 
        items[0].sector_count = total_disk_sectors;
        items[0].size_mb = ((total_disk_sectors -1 )* 512) / 1024 / 1024;
        items[0].start_lba = 1;
        items[0].mbr_index = -1;
        return 1;
    }

    SortedPartition valid_parts[4];
    int valid_count = 0;

    for (int i = 0; i < 4; i++) {
        MBR_PartitionEntry* part = &mbr->partitions[i];
        if (part->partition_type == 0x00 || part->sector_count == 0) continue;
        
        if ((uint64_t)part->start_lba + part->sector_count > (uint64_t)total_disk_sectors) continue;

        valid_parts[valid_count].entry = *part;
        valid_parts[valid_count].original_index = i;
        valid_count++;
    }

    for (int i = 0; i < valid_count - 1; i++) {
        for (int j = 0; j < valid_count - i - 1; j++) {
            if (valid_parts[j].entry.start_lba > valid_parts[j + 1].entry.start_lba) {
                SortedPartition temp = valid_parts[j];
                valid_parts[j] = valid_parts[j + 1];
                valid_parts[j + 1] = temp;
            }
        }
    }

    uint64_t current_lba = 1;
    uint8_t object_id = 0;

    for (int i = 0; i < valid_count; i++) {
        MBR_PartitionEntry* part = &valid_parts[i].entry;

        if (part->start_lba > current_lba) {
            uint64_t unpart_sectors = part->start_lba - current_lba;
            
            items[object_id].is_partition = 0;
            strcpy(items[object_id].name, "Unpartitioned space"); 
            items[object_id].sector_count = unpart_sectors;
            items[object_id].size_mb = (unpart_sectors * 512) / 1024 / 1024;
            items[object_id].start_lba = current_lba;
            items[object_id].mbr_index = -1;
            object_id++;
        }

        const char* fs_type = "Unknown Partition";
        if (part->partition_type == 0x07) fs_type = "NTFS/exFAT Partition";
        else if (part->partition_type == 0x0B || part->partition_type == 0x0C) fs_type = "FAT32 Partition";
        else if (part->partition_type == 0x83) fs_type = "Linux Ext Partition";

        items[object_id].is_partition = 1;
        strcpy(items[object_id].name, fs_type); 
        items[object_id].sector_count = part->sector_count;
        items[object_id].size_mb = ((uint64_t)part->sector_count * 512) / 1024 / 1024;
        items[object_id].start_lba = part->start_lba;
        items[object_id].mbr_index = valid_parts[i].original_index;
        object_id++;

        current_lba = part->start_lba + part->sector_count;
    }

    if (current_lba < (uint64_t)total_disk_sectors) {
        uint64_t final_sectors = total_disk_sectors - current_lba;

        items[object_id].is_partition = 0;
        strcpy(items[object_id].name, "Unpartitioned space"); 
        items[object_id].sector_count = final_sectors;
        items[object_id].size_mb = (final_sectors * 512) / 1024 / 1024;
        items[object_id].start_lba = current_lba;
        items[object_id].mbr_index = -1;
        object_id++;
    }

    return object_id;
}

void draw_partition_list(InstallItem* list, int count, int selected_index) {
    uint8_t start_row = 15;
    
    for (int i = 0; i < count; i++) {
        uint8_t color;
        
        if (i == selected_index) {
            color = 0x70;
        } else {
            color = 0x1F;
        }
        
        tprintf(6, start_row + i, color, "      %-40s %4d MB   ", list[i].name, list[i].size_mb);
    }
}

int handle_delete_partition(int drive_id, Drive* drive) {
    InstallItem* selected = &items[current_selection];
    
    if (selected->is_partition == 0 || selected->mbr_index == -1) {
        return 0; 
    }

    clear_setup_screen();

    int drive_size_in_mb = (drive->sector_count * 512) / 1024 / 1024;

    tprintf(3, 4, 0x1F, "You asked the Setup to delete the partition\n\n"
    "-: %-40s %4d MB\n\n",
    selected->name, selected->size_mb);

    if(drive->type == 0) {
        tprintf(3, 8, 0x1F, "on %d MB Disk %d at ID %d on bus 0 on atapi [MBR].", drive_size_in_mb, drive_id, get_ata_disk_id_on_bus(drive->port));
    } else {
        tprintf(3, 8, 0x1F, "on %d MB Disk %d at port %d on AHCI(SATA) [MBR].", drive_size_in_mb, drive_id, drive->port);
    }

    tprintf(3, 10, 0x1F, "   - To delete this partition, press L.\n"
    "     CAUTION: All data on this partition will be lost.\n\n"
    "   - To return to the previous screen without\n"
    "     deleting the partition, press Esc.");

    update_status_bar("L=Delete ESC=Cancel");

    while(1) {

        unsigned char key = getch();
        if(key == 'l') {
            break;
        } else if(key == 27) {
            return 1;
        }
    }

    MBR_Sector* mbr = (MBR_Sector*)mbr_buffer;

    uint8_t* part_ptr = (uint8_t*)&mbr->partitions[selected->mbr_index];
    for (int i = 0; i < 16; i++) {
        part_ptr[i] = 0x00;
    }

    int status = disk_write(drive_id, 0, 1, (uint64_t)mbr_buffer);
    if(status != ATA_SUCCESS) SetupATAError();

    int partitions_count = parse_partitions(mbr_buffer, drive->sector_count);

    if(current_selection >= partitions_count) {
        current_selection = partitions_count - 1;
    }

    return 1;
}

void commit_new_partition_to_mbr(uint64_t size_mb, uint64_t start_lba, int drive_id) {
    MBR_Sector* mbr = (MBR_Sector*)mbr_buffer;
    
    uint64_t sector_count = (size_mb * 1024 * 1024) / 512;

    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].partition_type == 0x00) {
            mbr->partitions[i].boot_indicator = 0x00;
            mbr->partitions[i].partition_type = 0x06;
            mbr->partitions[i].start_lba = start_lba;
            mbr->partitions[i].sector_count = sector_count;

            mbr->boot_signature = 0xAA55;

            int status = disk_write(drive_id, 0, 1, (uint64_t)mbr_buffer);
            if(status != ATA_SUCCESS) SetupATAError();
            break;
        }
    }
}

void show_create_partition_screen(uint64_t max_size_mb, uint64_t start_lba, int drive_id, Drive* drive) {

    clear_setup_screen();

    int drive_size_in_mb = (drive->sector_count * 512) / 1024 / 1024;

    tprintf(3, 4, 0x1F, "You have chosen to create a new partition on the unpartitioned space of");

    if(drive->type == 0) {
        tprintf(3, 5, 0x1F, "%d MB Disk %d at ID %d on bus 0 on atapi [MBR].", drive_size_in_mb, drive_id, get_ata_disk_id_on_bus(drive->port));
    } else {
        tprintf(3, 5, 0x1F, "%d MB Disk %d at port %d on AHCI(SATA) [MBR].", drive_size_in_mb, drive_id, drive->port);
    }
    
    tprintf(3, 7, 0x1F, "- To create the new partition, enter a size below and press ENTER.\n\n"
    "- To go back to the previous screen without creating, press ESC.\n\n"
    "Create partition of size (in MB):"
    );

    update_status_bar("ENTER=Create ESC=Cancel");

    char size_str[7]; 
    int str_len = 0;

    int_to_str(max_size_mb, size_str, 10, 0);
    str_len = strlen(size_str);

    while(1) {
        tprintf(37, 11, 0x1F, "[ %-6s ]", size_str);

        unsigned char key = getch();
        if (key == 0) continue;

        if (key == 27) {
            return;
        }

        if (key == 13) {
            uint64_t entered_size = str_to_int(size_str);
            
            if (entered_size > 0 && entered_size <= max_size_mb) {
                commit_new_partition_to_mbr(entered_size, start_lba, drive_id);
                return;
            } else {
                continue; 
            }
        }

        if (key == 8) {
            if (str_len > 0) {
                size_str[--str_len] = '\0';
            }
            continue;
        }

        if (str_len < 6 && (key >= 48 && key <= 57)) {
            size_str[str_len++] = key;
            size_str[str_len] = '\0';
            continue;
        }
    }
}

void draw_progress_screen(int percentage, const char* current_action) {
    clear_setup_screen();

    t_print("Please wait while Setup formats the partition", 3, 6, 0x1F);

    t_print("[", 18, 9, 0x1F);
    t_print("]", 59, 9, 0x1F);

    int blocks_to_fill = (percentage * 40) / 100;
    
    char block_char[2] = { 0xDB, 0 };
    char empty_char[2] = { 0xB0, 0 };

    for (int i = 0; i < 40; i++) {
        if (i < blocks_to_fill) {
            t_print(block_char, 19 + i, 9, 0x1E);
        } else {
            t_print(empty_char, 19 + i, 9, 0x17);
        }
    }

    tprintf(61, 9, 0x1F, "%3d%%", percentage);
    tprintf(3, 12, 0x1F, "%-50s", current_action);

    update_status_bar(" ");
}

void draw_copying_progress_screen(int percentage, const char *current_file) {
    int blocks_to_fill = (percentage * 40) / 100;
    
    char block_char[2] = { 0xDB, 0 };
    char empty_char[2] = { 0xB0, 0 };

    for (int i = 0; i < 40; i++) {
        if (i < blocks_to_fill) {
            t_print(block_char, 19 + i, 9, 0x1E);
        } else {
            t_print(empty_char, 19 + i, 9, 0x17);
        }
    }

    tprintf(61, 9, 0x1F, "%3d%%", percentage);
    tprintf(57, 24, 0x70, "|COPYING: %s", current_file);
}

void SetupCopyFiles(uint64_t start_lba, uint64_t sector_count, int disk_id) {

    clear_setup_screen();

    t_print("Please wait while Setup copies files to the OrangeOS installation folders.", 3, 6, 0x1F);
    t_print("This might take several minutes to complete.", 3, 7, 0x1F);

    t_print("[", 18, 9, 0x1F);
    t_print("]", 59, 9, 0x1F);

    update_status_bar(" ");

    char list_of_files[2][2][20];
    int file_name_sizes[2];

    strcpy(list_of_files[0][0], "_BOOTMGR");
    file_name_sizes[0] = 7;
    strcpy(list_of_files[0][1], "BOOTMGR");

    strcpy(list_of_files[1][0], "OROSKRNL.OEX");
    file_name_sizes[1] = 12;
    strcpy(list_of_files[1][1], "OROSKRNL.OEX");

    int status = iso9660_init();
    if(status != 0) {
        clear_setup_screen();
        t_print("The installation media's CDFS is corrupted.", 3, 4, 0x1F);
        t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    int64_t root_cd_buffer = alloc_page();
    if(root_cd_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)(uintptr_t)root_cd_buffer, root_lba, 1);

    for(int i = 0; i < 2; i++) {

        draw_copying_progress_screen(i * 50, list_of_files[i][0]);

        uint64_t file_size = 0;
        int64_t file_lba = iso9660_find_file((void*)(uintptr_t)root_cd_buffer, list_of_files[i][0], file_name_sizes[i], &file_size);

        if(file_size == 0 || file_lba == 0) {
            clear_setup_screen();
            tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", list_of_files[i][0]);
            t_print("The installation program cannot proceed.", 3, 5, 0x1F);
            update_status_bar("Reboot your computer now.");

            Hold();
        }

        uintptr_t setup_file_buffer = (uintptr_t)kmalloc(file_size + 2048);

        if(setup_file_buffer == 0) {
            KeBugCheck(0x20);
        }

        read_atapi_sector((void*)virtual_to_physical(pt_begin, setup_file_buffer), file_lba, (file_size + 2047) / 2048);

        status = fat32_write_file(disk_id, start_lba, list_of_files[i][1], (void*)setup_file_buffer, file_size);

        if(status != 0) {
            clear_setup_screen();
            tprintf(3, 4, 0x1F, "Setup fail to copy this file to your computer: %s", list_of_files[i][0]);
            t_print("The installation program cannot proceed.", 3, 5, 0x1F);
            update_status_bar("Reboot your computer now.");

            Hold();
        }

        kfree((void*)setup_file_buffer);

    }

    draw_copying_progress_screen(100, "MBR...      ");

    uint64_t file_size = 0;
    int64_t file_lba = iso9660_find_file((void*)(uintptr_t)root_cd_buffer, "_BOOTMBR", 8, &file_size);

    if(file_size == 0 || file_lba == 0) {
        clear_setup_screen();
        tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", "_BOOTMBR");
        t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    uintptr_t mbr_cd_buffer = (uintptr_t)kmalloc(file_size + 2048);

    if(mbr_cd_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)virtual_to_physical(pt_begin, mbr_cd_buffer), file_lba, (file_size + 2047) / 2048);
    memcpy(mbr_buffer, (void*)mbr_cd_buffer, 446);

    MBR_Sector* mbr = (MBR_Sector*)mbr_buffer;

    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].start_lba == start_lba) {
            mbr->partitions[i].boot_indicator = 0x80;
        } else {
            mbr->partitions[i].boot_indicator = 0x00;
        }
    }
    
    disk_write(disk_id, 0, 1, (uint64_t)&mbr_buffer);

    // _VBRSEC0 _VBRSEC1

    draw_copying_progress_screen(100, "_VBRSEC0    ");

    file_size = 0;
    file_lba = iso9660_find_file((void*)(uintptr_t)root_cd_buffer, "_VBRSEC0", 8, &file_size);

    if(file_size == 0 || file_lba == 0) {
        clear_setup_screen();
        tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", "_VBRSEC0");
        t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    fat32_vbr_t* vbr_0_buffer = kmalloc(file_size + 2048);

    if(vbr_0_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)virtual_to_physical(pt_begin, (uint64_t)vbr_0_buffer), file_lba, (file_size + 2047) / 2048);

    fat32_vbr_t real_vbr;
    status = disk_read(disk_id, start_lba + 0, 1, (uint64_t)&real_vbr);
    if(status != ATA_SUCCESS) SetupATAError();
    
    memcpy(real_vbr.boot_code, vbr_0_buffer->boot_code, 420);
    
    disk_write(disk_id, start_lba, 1, (uint64_t)&real_vbr);
    disk_write(disk_id, start_lba+6, 1, (uint64_t)&real_vbr);



    // _VBRSEC1

    draw_copying_progress_screen(100, "_VBRSEC1    ");

    file_size = 0;
    file_lba = iso9660_find_file((void*)(uintptr_t)root_cd_buffer, "_VBRSEC1", 8, &file_size);

    if(file_size == 0 || file_lba == 0) {
        clear_setup_screen();
        tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", "_VBRSEC1");
        t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    uintptr_t vbr_1_buffer = (uintptr_t)kmalloc(file_size + 2048);

    if(vbr_1_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)virtual_to_physical(pt_begin, (uint64_t)vbr_1_buffer), file_lba, (file_size + 2047) / 2048);
    disk_write(disk_id, start_lba+2, 4, vbr_1_buffer);

    Hold();

}

void perform_formatting(uint64_t start_lba, uint64_t sector_count, int mode, int disk_id) {

    draw_progress_screen(0, "Creating FAT32 structures...");

    uint16_t reserved_sectors = 32; 
    uint8_t fat_count = 2;          
    uint8_t sectors_per_cluster = get_sectors_per_cluster(sector_count);

    uint64_t data_sectors = sector_count - reserved_sectors;
    uint64_t fat_size_sectors = (data_sectors + (sectors_per_cluster * 128 / fat_count)) 
                                / (sectors_per_cluster * 128 + fat_count);
    fat_size_sectors = ((data_sectors / sectors_per_cluster) * 4 + 511) / 512;

    draw_progress_screen(5, "Writing Volume Boot Record...");

    fat32_vbr_t vbr;
    memset(&vbr, 0, sizeof(fat32_vbr_t));
    
    vbr.jmp[0] = 0xEB; vbr.jmp[1] = 0x58; vbr.jmp[2] = 0x90;
    memcpy(vbr.oem_name, "MSWIN4.1", 8);
    vbr.bytes_per_sector = 512;
    vbr.sectors_per_cluster = sectors_per_cluster;
    vbr.reserved_sectors = reserved_sectors;
    vbr.fat_count = fat_count;
    vbr.media_type = 0xF8;
    vbr.hidden_sectors = start_lba;
    vbr.total_sectors_32 = sector_count;
    
    vbr.sectors_per_fat_32 = fat_size_sectors;
    vbr.root_cluster = 2;
    vbr.fs_info_sector = 1;
    vbr.backup_boot_sector = 6;
    vbr.drive_number = 0x80;
    vbr.boot_signature = 0x29;
    vbr.volume_id = 0x12345678;
    memcpy(vbr.volume_label, "ORANGEOS   ", 11);
    memcpy(vbr.fs_type, "FAT32   ", 8);
    vbr.boot_magic = 0xAA55;

    draw_progress_screen(10, "Initializing FSInfo sector...");

    fat32_fsinfo_t fsinfo;
    memset(&fsinfo, 0, sizeof(fat32_fsinfo_t));
    fsinfo.lead_signature = 0x41615252;
    fsinfo.struct_signature = 0x61417272;
    fsinfo.free_cluster_count = 0xFFFFFFFF;
    fsinfo.next_free_cluster = 0xFFFFFFFF;
    fsinfo.trail_signature = 0xAA550000;

    uint8_t sector_buffer[512];

    int status = disk_write(disk_id, start_lba + 0, 1, (uint64_t)&vbr);
    if(status != ATA_SUCCESS) SetupATAError();

    status = disk_write(disk_id, start_lba + 1, 1, (uint64_t)&fsinfo);
    if(status != ATA_SUCCESS) SetupATAError();

    status = disk_write(disk_id, start_lba + 6, 1, (uint64_t)&vbr);
    if(status != ATA_SUCCESS) SetupATAError();

    status = disk_write(disk_id, start_lba + 7, 1, (uint64_t)&fsinfo);
    if(status != ATA_SUCCESS) SetupATAError();

    memset(sector_buffer, 0, 512);
    for (uint64_t i = 2; i < reserved_sectors; i++) {
        if (i != 6 && i != 7) {

            status = disk_write(disk_id, start_lba + i, 1, (uint64_t)sector_buffer);
            if(status != ATA_SUCCESS) SetupATAError();
        }
    }

    draw_progress_screen(20, "Clearing File Allocation Tables...");
    
    for (uint8_t f = 0; f < fat_count; f++) {
        uint64_t fat_start_lba = start_lba + reserved_sectors + (f * fat_size_sectors);
        
        // Пишем первый сектор таблицы FAT
        memset(sector_buffer, 0, 512);
        uint32_t* fat_table = (uint32_t*)sector_buffer;
        fat_table[0] = 0x0FFFFFF8;
        fat_table[1] = 0xFFFFFFFF;
        fat_table[2] = 0x0FFFFFFF;

        status = disk_write(disk_id, fat_start_lba, 1, (uint64_t)sector_buffer);
        if(status != ATA_SUCCESS) SetupATAError();

        memset(sector_buffer, 0, 512);
        for (uint64_t i = 1; i < fat_size_sectors; i++) {
            status = disk_write(disk_id, fat_start_lba+i, 1, (uint64_t)sector_buffer);
            if(status != ATA_SUCCESS) SetupATAError();
        }
    }

    uint64_t root_lba = start_lba + reserved_sectors + (fat_count * fat_size_sectors);
    memset(sector_buffer, 0, 512);
    for (uint8_t i = 0; i < sectors_per_cluster; i++) {
        status = disk_write(disk_id, root_lba + i, 1, (uint64_t)sector_buffer);
        if(status != ATA_SUCCESS) SetupATAError();
    }

    draw_progress_screen(20, "Formatting drive...");

    int old_progress = 0;

    if(mode == 1) {
        uint64_t clean_lba = root_lba + sectors_per_cluster;

        for(uint64_t i = clean_lba; i < (sector_count+start_lba) - 1; i++) {

            float max_difference = (((sector_count+start_lba)) - 1 - clean_lba);
            float difference = (((sector_count+start_lba)) - 1 - i);

            float progress = 1 - (difference / max_difference);

            int int_progress = 80 * progress;
            if(old_progress != int_progress) {
                draw_progress_screen(20 + int_progress, "Formatting drive...");
                old_progress = int_progress;
            }

            status = disk_write(disk_id, i, 1, (uint64_t)sector_buffer);
            if(status != ATA_SUCCESS) SetupATAError();
        }

    }

    draw_progress_screen(100, "Formatting complete!");

    SetupCopyFiles(start_lba, sector_count, disk_id);
}

void show_format_screen(uint64_t start_lba, uint64_t sector_count, int disk_id) {
    int current_fs_sel = 0;
    int total_options = 2;

    while(1) {
        clear_setup_screen();

        tprintf(3, 4, 0x1F, "The partition you selected is not formatted, or it is formatted with\n"
        "a file system unknown to Setup.\n\n"
        "Setup will now format the partition. Use the UP and DOWN ARROW keys\n"
        "to select the file system you want to use, then press ENTER.");

        uint8_t start_row = 11;
        for (int i = 0; i < total_options; i++) {
            uint8_t color = (i == current_fs_sel) ? 0x70 : 0x1F;
            tprintf(6, start_row + i, color, "      %-60s", fs_options[i]);
        }

        update_status_bar("ENTER=Continue ESC=Cancel");

        unsigned char key = getch();
        if (key == 0 || key == 224) {
            int key = getch();

            if (key == 72) {
                if (current_fs_sel > 0) current_fs_sel--;
            } else if(key == 80) {
                if (current_fs_sel < total_options - 1) current_fs_sel++;
            }
        }

        if (key == 27) {
            return;
        }

        if (key == 13) {
            perform_formatting(start_lba, sector_count, current_fs_sel, disk_id);
            return;
        }
    }
}

void ShowPartitions(int disk_id, Drive* drive) {

    while(1) {
        clear_setup_screen();

        int drive_size_in_mb = (drive->sector_count * 512) / 1024 / 1024;
        
        tprintf(3, 4, 0x1F, "The following list shows the existing partitions and\n"
            "unpartitioned space on this computer.\n\n"
            "Use the UP and DOWN ARROW keys to select the item in the list.\n\n"
            "To set up OrangeOS on the selected item, press ENTER.\n"
            "To create a partition in the unpartitioned space, press C.\n"
            "To delete the selected partition, press D.\n\n"
            );

        if(drive->type == 0) {
            tprintf(3, 13, 0x1F, "%d MB Disk 0 at ID %d on bus 0 on atapi:", drive_size_in_mb, disk_id, get_ata_disk_id_on_bus(drive->port));
        } else {
            tprintf(3, 13, 0x1F, "%d MB Disk %d at port %d on AHCI(SATA):", drive_size_in_mb, disk_id, drive->port);
        }

        int partitions_count = parse_partitions(mbr_buffer, drive->sector_count);

        while(1) {
            draw_partition_list(items, partitions_count, current_selection);

            InstallItem* selected = &items[current_selection];
            if (selected->is_partition == 0) {
                update_status_bar("C=Create Partition F3=Exit");
            } else {
                update_status_bar("ENTER=Install D=Delete Partition F3=Exit");
            }

            int key = getch();
            if (key == 0 || key == 224) {
                int key = getch();

                if (key == 72) {
                    current_selection--;
                    if(current_selection < 0) current_selection = 0;
                } else if(key == 80) {
                    current_selection++;
                    if(current_selection >= partitions_count) {
                        current_selection = partitions_count - 1;
                    }
                }
            }
            if(key == 'd') {
                int result = handle_delete_partition(disk_id, drive);
                if(result == 1) {
                    break;
                }
            } else if(key == 'c') {
    
                if (selected->is_partition == 0) {
                    show_create_partition_screen(selected->size_mb, selected->start_lba, disk_id, drive);
                    break;
                }

            } else if(key == 13) {

                InstallItem* selected = &items[current_selection];
    
                if (selected->is_partition == 1) {
                    show_format_screen(selected->start_lba, selected->sector_count, disk_id);
                    break;
                }
            }

        }

    }
}

void KiSetup() {

    clear_setup_screen();

    int finalStatus = 0;

    int status = init_ahci();
    if(status == 1) finalStatus = 1;

    status = init_ata_dma();
    if(status == 1) finalStatus = 1;

    status = init_nvme();
    if(status != 0) {
        status = nvme_controller_reset();
        if(status != 0) {
            nvme_identify_disk();
            nvme_identify_namespace();

            status = nvme_create_io_queues();
            if(status != 0) {
                // NVMe detection
            }
        }
    }

    if(finalStatus == 0) {
        t_print("OrangeOS failed to detect IDE/AHCI controllers.", 3, 4, 0x1F);
        t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("F3=Exit");
        Hold();
    }

    registerATADevices();
    registerSATADevices();

    if(get_drives_count() == 0) {
        t_print("No disks were detected on this device.", 3, 4, 0x1F);
        t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("F3=Exit");
        Hold();
    }

    pci_device_t* xhci_controller = find_xhci_controller();
    if(xhci_controller != 0) {
        uintptr_t xhci_addr = get_xhci_base_address(xhci_controller);

        if(xhci_addr == 0) {

            enable_xhci_pci_bus(xhci_controller);
            xhci_read_capabilities(xhci_addr);
            xhci_reset_controller(xhci_addr);
            xhci_init_event_ring(xhci_addr);
            xhci_init_dcbaa(xhci_addr);
            xhci_start_controller(xhci_addr);
            xhci_init_command_ring(xhci_addr);
            xhci_finalize_controller_init(xhci_addr);
            //xhci_probe_ports(xhci_addr);

        }

    }

    int drive_id = 0;

    Drive* drive = get_disk(drive_id);

    int drive_size_in_mb = (drive->sector_count * 512) / 1024 / 1024;
    if(drive->type == 0) {
        tprintf(0, 24, 0x70, "Examining %d MB Disk %d at ID %d on bus 0 on atapi...", drive_size_in_mb, drive_id, get_ata_disk_id_on_bus(drive->port));
    } else {
        tprintf(0, 24, 0x70, "Examining %d MB Disk %d at port %d on AHCI(SATA)...", drive_size_in_mb, drive_id, drive->port);
    }

    status = disk_read(drive_id, 0, 1, (uint64_t)&mbr_buffer);
    if(status != ATA_SUCCESS) SetupATAError();

    clear_setup_screen();
    update_status_bar("F8=I agree ESC=I do not agree");

    tprintf(3, 4, 0x1F, "Copyright(C) Alexander Volokhov, 2023-2026."
        "\n\n"
        "Permission is hereby granted, free of charge, to any person obtaining\n"
        "a copy of this software and associated documentation files (the \"Software\"),\n"
        "to deal in the Software without restriction, including without limitation\n"
        "the rights to use, copy, modify, merge, publish, distribute, sublicense,\n"
        "and/or sell copies of the Software, and to permit persons to whom the\n"
        "Software is furnished to do so, subject to the following conditions:\n\n"
        ""
        "The above copyright notice and this permission notice shall be included \n"
        "in all copies or substantial portions of the Software.\n\n"
        ""
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS\n"
        "IN THE SOFTWARE.");

    while(1) {
        int key = getch();
        if(key == 0) {
            key = getch();
            if(key == KEY_F8) {
                ShowPartitions(drive_id, drive);
            }
        } else if(key == 27) {
            ShowExit();
        }
    }
}

_Static_assert(sizeof(fat32_vbr_t) == 512, "Размер VBR должен быть ровно 512 байт!");