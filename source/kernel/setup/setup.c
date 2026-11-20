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
#include "setup.h"

radiobutton_t* radios_partitions;

char size_str[7]; 
int str_len = 0;
int max_size_mb = 0;

int global_flag = 0;

int setup_drive_id = 0;
int partitions_count = 0;
int hover_partition = -1;
int selected_partition = -1;

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

static button_t btn_error_ata_ok   = { .rel_x = 5,  .rel_y = 40, .width = 80, .height = 24, .text = "OK",   .is_pressed = 0 };

void draw_error_ata_window(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "The drive controller returned an error during a read/write attempt.\nThe installation program cannot proceed.");

    draw_button(0, 0, &btn_error_ata_ok);
}

void mouse_error_ata_window(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_error_ata_ok, click_x, click_y, is_left_pressed)) {
        btn_error_ata_ok.is_pressed = 0;
        destroyWindow(win);
    }
}

void SetupATAError() {
    createWindow(200, 200, 300, 150, "OrangeOS Setup - Error", draw_error_ata_window, mouse_error_ata_window);
}

void tprintf(uint8_t x, uint8_t y, uint8_t color, const char *fmt, ...) {

    (void)color;

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
                    int_to_str(d, temp_buf, 10, 1, 0);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'x': {
                    unsigned int val_x = va_arg(args, unsigned int);
                    int_to_str(val_x, temp_buf, 16, 0, 0);
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
                //t_print(buffer, x, y, color);
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
        //t_print(buffer, x, y, color);
    }
}

void ShowExit() {
    
    //clear_setup_screen();
    //t_print("You declined the installation of OrangeOS.", 3, 4, 0x1F);
    //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
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
        items[0].mbr_index = 5;
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

            if((unpart_sectors * 512) / 1024 / 1024 > 0) {
            
                items[object_id].is_partition = 0;
                strcpy(items[object_id].name, "Unpartitioned space"); 
                items[object_id].sector_count = unpart_sectors;
                items[object_id].size_mb = (unpart_sectors * 512) / 1024 / 1024;
                items[object_id].start_lba = current_lba;
                items[object_id].mbr_index = 5;
                object_id++;

            }
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
        items[object_id].mbr_index = 5;
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
    
    if (selected->is_partition == 0) {
        return 0; 
    }

    //clear_setup_screen();

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

    //clear_setup_screen();

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

    int_to_str(max_size_mb, size_str, 10, 0, 0);
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
    //clear_setup_screen();

    //t_print("Please wait while Setup formats the partition", 3, 6, 0x1F);

    //t_print("[", 18, 9, 0x1F);
    //t_print("]", 59, 9, 0x1F);

    int blocks_to_fill = (percentage * 40) / 100;

    for (int i = 0; i < 40; i++) {
        if (i < blocks_to_fill) {
            //t_print(block_char, 19 + i, 9, 0x1E);
        } else {
            //t_print(empty_char, 19 + i, 9, 0x17);
        }
    }

    tprintf(61, 9, 0x1F, "%3d%%", percentage);
    tprintf(3, 12, 0x1F, "%-50s", current_action);

    update_status_bar(" ");
}

void draw_copying_progress_screen(int percentage, const char *current_file) {
    int blocks_to_fill = (percentage * 40) / 100;

    for (int i = 0; i < 40; i++) {
        if (i < blocks_to_fill) {
            //t_print(block_char, 19 + i, 9, 0x1E);
        } else {
            //t_print(empty_char, 19 + i, 9, 0x17);
        }
    }

    tprintf(61, 9, 0x1F, "%3d%%", percentage);
    tprintf(57, 24, 0x70, "|COPYING: %s", current_file);
}

void SetupCopyFiles(uint64_t start_lba, int disk_id) {



    //clear_setup_screen();

    //t_print("Please wait while Setup copies files to the OrangeOS installation folders.", 3, 6, 0x1F);
    //t_print("This might take several minutes to complete.", 3, 7, 0x1F);

    //t_print("[", 18, 9, 0x1F);
    //t_print("]", 59, 9, 0x1F);

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
        //clear_setup_screen();
        //t_print("The installation media's CDFS is corrupted.", 3, 4, 0x1F);
        //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    int64_t root_cd_buffer = alloc_page();
    if(root_cd_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)(uintptr_t)root_cd_buffer, root_lba, 1, 0xB0);

    for(int i = 0; i < 2; i++) {

        draw_copying_progress_screen(i * 50, list_of_files[i][0]);

        uint64_t file_size = 0;
        int64_t file_lba = iso9660_find_file((void*)(uintptr_t)root_cd_buffer, list_of_files[i][0], file_name_sizes[i], &file_size);

        if(file_size == 0 || file_lba == 0) {
            //clear_setup_screen();
            tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", list_of_files[i][0]);
            //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
            update_status_bar("Reboot your computer now.");

            Hold();
        }

        uintptr_t setup_file_buffer = (uintptr_t)kmalloc(file_size + 2048);

        if(setup_file_buffer == 0) {
            KeBugCheck(0x20);
        }

        read_atapi_sector((void*)virtual_to_physical(pt_begin, setup_file_buffer), file_lba, (file_size + 2047) / 2048, 0xB0);

        status = fat32_write_file(disk_id, start_lba, list_of_files[i][1], (void*)setup_file_buffer, file_size);

        if(status != 0) {
            //clear_setup_screen();
            tprintf(3, 4, 0x1F, "Setup fail to copy this file to your computer: %s", list_of_files[i][0]);
            //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
            update_status_bar("Reboot your computer now.");

            Hold();
        }

        kfree((void*)setup_file_buffer);

    }

    draw_copying_progress_screen(100, "MBR...      ");

    uint64_t file_size = 0;
    int64_t file_lba = iso9660_find_file((void*)(uintptr_t)root_cd_buffer, "_BOOTMBR", 8, &file_size);

    if(file_size == 0 || file_lba == 0) {
        //clear_setup_screen();
        tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", "_BOOTMBR");
        //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    uintptr_t mbr_cd_buffer = (uintptr_t)kmalloc(file_size + 2048);

    if(mbr_cd_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)virtual_to_physical(pt_begin, mbr_cd_buffer), file_lba, (file_size + 2047) / 2048, 0xB0);
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
        //clear_setup_screen();
        tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", "_VBRSEC0");
        //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    fat32_vbr_t* vbr_0_buffer = kmalloc(file_size + 2048);

    if(vbr_0_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)virtual_to_physical(pt_begin, (uint64_t)vbr_0_buffer), file_lba, (file_size + 2047) / 2048, 0xB0);

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
        //clear_setup_screen();
        tprintf(3, 4, 0x1F, "This setup file is missing or corrupt: %s", "_VBRSEC1");
        //t_print("The installation program cannot proceed.", 3, 5, 0x1F);
        update_status_bar("Reboot your computer now.");

        Hold();
    }

    uintptr_t vbr_1_buffer = (uintptr_t)kmalloc(file_size + 2048);

    if(vbr_1_buffer == 0) {
        KeBugCheck(0x20);
    }

    read_atapi_sector((void*)virtual_to_physical(pt_begin, (uint64_t)vbr_1_buffer), file_lba, (file_size + 2047) / 2048, 0xB0);
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

    SetupCopyFiles(start_lba, disk_id);
}

void show_format_screen(uint64_t start_lba, uint64_t sector_count, int disk_id) {
    int current_fs_sel = 0;
    int total_options = 2;

    while(1) {
        //clear_setup_screen();

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
        //clear_setup_screen();

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

static button_t btn_error_drive_ok   = { .rel_x = 5,  .rel_y = 40, .width = 80, .height = 24, .text = "OK",   .is_pressed = 0 };

void draw_error_drive_window(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "No disks were detected on this device.\nThe installation program cannot proceed.");

    draw_window_button(win, 0, 0, &btn_error_drive_ok);
}

void mouse_error_drive_window(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_error_drive_ok, click_x, click_y, is_left_pressed)) {
        btn_error_drive_ok.is_pressed = 0;
        destroyWindow(win);
    }
}

static button_t btn_setup_welcome_next   = { .rel_x = 410,  .rel_y = 90, .width = 80, .height = 24, .text = "Next",   .is_pressed = 0 };

void draw_setup_welcome_window(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "Welcome to Setup.\n\nThis portion of the Setup prepares OrangeOS to run on your computer.\n\nTo set up OrangeOS now, press Next");
    draw_window_button(win, 0, 0, &btn_setup_welcome_next);
}

/*oid show_create_partition_screen(uint64_t max_size_mb, uint64_t start_lba, int drive_id, Drive* drive) {

    //clear_setup_screen();

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
}*/

static button_t btn_setup_create_next   = { .rel_x = 500,  .rel_y = 300, .width = 80, .height = 24, .text = "Create",   .is_pressed = 0 };
static button_t btn_setup_create_back   = { .rel_x = 5,  .rel_y = 300, .width = 80, .height = 24, .text = "Back",   .is_pressed = 0 };

static button_t btn_setup_partitions_next   = { .rel_x = 500,  .rel_y = 300, .width = 80, .height = 24, .text = "Setup",   .is_pressed = 0 };
static button_t btn_setup_partitions_create_or_delete   = { .rel_x = 375,  .rel_y = 300, .width = 115, .height = 24, .text = "Create/Delete",   .is_pressed = 0 };
static button_t btn_setup_partitions_back   = { .rel_x = 5,  .rel_y = 300, .width = 80, .height = 24, .text = "Back",   .is_pressed = 0 };

void draw_create_partitions_window(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    Drive* drive = get_disk(setup_drive_id);

    int drive_size_in_mb = (drive->sector_count * 512) / 1024 / 1024;

    if(drive->type == 0) {
        win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "You have chosen to create a new partition on the unpartitioned space\non %d MB Disk 0 at ID %d on bus 0 on atapi:", drive_size_in_mb, setup_drive_id, get_ata_disk_id_on_bus(drive->port));
    } else {
        win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "You have chosen to create a new partition on the unpartitioned space\non %d MB Disk %d at port %d on AHCI(SATA):", drive_size_in_mb, setup_drive_id, drive->port);
    }

    win_printf(win, 5, 45, 0x00, 0x00, 0x00, 0xFF, "Create partition of size (in MB): ");

    int_to_str(max_size_mb, size_str, 10, 0, 0);
    str_len = strlen(size_str);



    draw_window_rect(win, 180, 45, 100, 15, 0xC0, 0xC0, 0xC0, 0xFF);

    draw_window_button(win, 0, 0, &btn_setup_create_next);
    draw_window_button(win, 0, 0, &btn_setup_create_back);
}

void mouse_create_partitions_window(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_setup_create_back, click_x, click_y, is_left_pressed)) {
        btn_setup_create_back.is_pressed = 0;
        destroyWindow(win);
        showPartitions();
    }

    if (is_button_clicked(&btn_setup_create_next, click_x, click_y, is_left_pressed)) {
        /*btn_setup_partitions_next.is_pressed = 0;
        destroyWindow(win);

        createWindow(175, 220, 500, 150, "OrangeOS Setup", draw_setup_partitions_window, mouse_setup_partitions_window);*/
    }

}

void draw_setup_partitions_window(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    Drive* drive = get_disk(setup_drive_id);

    int drive_size_in_mb = (drive->sector_count * 512) / 1024 / 1024;

    if(drive->type == 0) {
        win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "Choose your installation destination on %d MB Disk 0 at ID %d on bus 0 on atapi:", drive_size_in_mb, setup_drive_id, get_ata_disk_id_on_bus(drive->port));
    } else {
        win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "Choose your installation destination on %d MB Disk %d at port %d on AHCI(SATA):", drive_size_in_mb, setup_drive_id, drive->port);
    }

    for(int i = 0; i < partitions_count; i++) {
        draw_window_radiobutton(win, 0, 0, &radios_partitions[i]);
    }

    draw_window_button(win, 0, 0, &btn_setup_partitions_next);
    draw_window_button(win, 0, 0, &btn_setup_partitions_back);
    draw_window_button(win, 0, 0, &btn_setup_partitions_create_or_delete);
}

void mouse_setup_partitions_window(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_setup_partitions_back, click_x, click_y, is_left_pressed)) {
        btn_setup_partitions_back.is_pressed = 0;
        destroyWindow(win);
        showLicenseWindow();
    }

    if (is_button_clicked(&btn_setup_partitions_next, click_x, click_y, is_left_pressed)) {
        /*btn_setup_partitions_next.is_pressed = 0;
        destroyWindow(win);

        createWindow(175, 220, 500, 150, "OrangeOS Setup", draw_setup_partitions_window, mouse_setup_partitions_window);*/
    }

    if (is_button_clicked(&btn_setup_partitions_create_or_delete, click_x, click_y, is_left_pressed)) {
        btn_setup_partitions_create_or_delete.is_pressed = 0;
        destroyWindow(win);

        createWindow(150, 150, 600, 350, "OrangeOS Setup", draw_create_partitions_window, mouse_create_partitions_window);
    }


    update_radiobutton_group(radios_partitions, partitions_count, click_x, click_y, is_left_pressed);

}

static button_t btn_setup_license_next   = { .rel_x = 500,  .rel_y = 300, .width = 80, .height = 24, .text = "I agree",   .is_pressed = 0 };
static button_t btn_setup_license_back   = { .rel_x = 5,  .rel_y = 300, .width = 80, .height = 24, .text = "Back",   .is_pressed = 0 };

void draw_license_window(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "Copyright(C) Alexander Volokhov, 2023-2026."
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

    draw_window_button(win, 0, 0, &btn_setup_license_next);
    draw_window_button(win, 0, 0, &btn_setup_license_back);
}

void mouse_license_window(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_setup_license_back, click_x, click_y, is_left_pressed)) {
        btn_setup_license_back.is_pressed = 0;
        destroyWindow(win);

        KeSetupGraphic();
    }

    if (is_button_clicked(&btn_setup_license_next, click_x, click_y, is_left_pressed)) {
        btn_setup_license_next.is_pressed = 0;
        destroyWindow(win);

        Drive* drive = get_disk(setup_drive_id);
        partitions_count = parse_partitions(mbr_buffer, drive->sector_count);

        showPartitions();
    }

}

void showLicenseWindow() {
    global_flag = 1;
    createWindow(150, 150, 600, 350, "OrangeOS Setup", draw_license_window, mouse_license_window);
}

void showPartitions() {

    if(radios_partitions == 0) {

        radios_partitions = (radiobutton_t*)kmalloc(9 * sizeof(radiobutton_t));

    }

    for (int i = 0; i < partitions_count; i++) {

        radios_partitions[i].group_id = 0;
        radios_partitions[i].is_hovered = 0;
        radios_partitions[i].is_pressed = 0;
        radios_partitions[i].is_selected = 0;
        radios_partitions[i].rel_x = 5;
        radios_partitions[i].rel_y = 30+(15*i);
        radios_partitions[i].size = 14;

        if(radios_partitions[i].text == 0) {
            radios_partitions[i].text = (char*)kmalloc(100);
        }
        ksnprintf(radios_partitions[i].text, 100, "      %-40s %4d MB   ", items[i].name, items[i].size_mb);

    }


    createWindow(150, 150, 600, 350, "OrangeOS Setup", draw_setup_partitions_window, mouse_setup_partitions_window);
}

void mouse_setup_welcome_window(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_setup_welcome_next, click_x, click_y, is_left_pressed)) {

        int status = disk_read(setup_drive_id, 0, 1, (uint64_t)&mbr_buffer);
        if(status != ATA_SUCCESS) SetupATAError();

        btn_setup_welcome_next.is_pressed = 0;
        showLicenseWindow();
        destroyWindow(win);
    }
}

void KeSetupGraphic() {

    for(int i = 0; i < get_drives_count(); i++) {
        if(get_disk(i)->atapi == 0) {
            setup_drive_id = i;
            break;
        }

        if(i+1 == get_drives_count()) {
            createWindow(200, 200, 300, 150, "OrangeOS Setup - Error", draw_error_drive_window, mouse_error_drive_window);
            return;
        }
    }

    createWindow(175, 220, 500, 150, "Установка OrangeOS", draw_setup_welcome_window, mouse_setup_welcome_window);

}