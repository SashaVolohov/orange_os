#include "core.h"
#include "disk.h"

static int hover_partition = -1;

void draw_file_explorer_content(const window_t* win, int bx, int by, int bw, int bh) {
    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    int partitions_count = get_partition_count();

    for (uint32_t i = 0; i < partitions_count; i++) {

        int8_t colorRed = 0x00;
        int8_t colorGreen = 0x00;
        int8_t colorBlue = 0x00;
        int8_t colorAlpha = 0x00;

        if(hover_partition == i) {
            colorRed = 0x00;
            colorGreen = 0x56;
            colorBlue = 0xb3;
            colorAlpha = 0xFF;
        }

        draw_window_rect(win, 5, 5+(15*i), win->width - 20, 15, colorRed, colorGreen, colorBlue, colorAlpha);

        Partition* partition = get_partition(i);
        const char* fs_name = get_mbr_fs_type_name(partition->fs_type);
        size_t partition_size = partition->sector_count;

        if(get_disk(partition->drive_index)->atapi == 0) {
            partition_size *= 512;
            partition_size = partition_size / 1024 / 1024;
        } else {
            partition_size *= 2048;
            partition_size = partition_size / 1024 / 1024;
        }

        char disk_letter = i + 67;
        win_printf(win, 5, 5+(15*i), colorAlpha, colorAlpha, colorAlpha, 0xFF, "%c: (%s, %d MB)", disk_letter, fs_name, partition_size);
    }
}

void mouse_file_explorer_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    
    int partitions_count = get_partition_count();

    for (size_t i = 0; i < partitions_count; i++) {

        hover_partition = -1;

        if(click_y < 5) return;
        if(click_y > 5+(15*partitions_count)) return;

        hover_partition = (click_y - 5) / 15;

        /*if(is_left_pressed) {

            uint8_t* raw_bytes = (uint8_t*)( (uintptr_t)boot_info->vbe_modes_list);
            int internal_id = 0;

            for (uint32_t i = 0; i < boot_info->vbe_modes_count; i++) {

                uint8_t* entry = raw_bytes + (i * 11);
                uint16_t mode_id               = *(uint16_t*)(entry + 0);
                uint16_t width                 = *(uint16_t*)(entry + 2);
                uint16_t height                = *(uint16_t*)(entry + 4);
                uint8_t  bpp                   = *(uint8_t*) (entry + 6);
                uint32_t physical_base_pointer = *(uint32_t*)(entry + 7);

                if (mode_id < 0x100 || bpp != 32 || mode_id >= 0x200) {
                    continue;
                }

                if(selected_settings_id == internal_id) {
                    change_screen_resolution(mode_id, width, height, physical_base_pointer);
                    break;
                }

                internal_id++;
            }

        }*/
    }
}

void openFileExplorer() {
    createWindow(200, 200, 400, 300, "File Explorer", draw_file_explorer_content, mouse_file_explorer_content);
}