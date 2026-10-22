#include "gui.h"
#include "core.h"
#include "settings.h"
#include "os_memory.h"

static button_t settings_buttons[] = {
    { .rel_x = 10,  .rel_y = 50,  .width = 35, .height = 35, .text = "OK", .is_pressed = 0 },
};

#define NUM_SETTINGS_BUTTONS (sizeof(settings_buttons) / sizeof(settings_buttons[0]))

int8_t selected_settings_id = -1;
int8_t count_of_video = 0;

extern uint32_t* fb_back_buffer;

void change_screen_resolution(uint16_t target_mode_id, uint32_t width, uint32_t height, uint32_t physical_base_pointer) {
    __asm__ volatile("cli");

    size_t new_size = (size_t)width * (size_t)height * 4;

    uint32_t* new_back_buffer = (uint32_t*)kmalloc(new_size);
    
    if (new_back_buffer == NULL) {
        __asm__ volatile("sti");
        return;
    }

    size_t trampoline_size = (uintptr_t)&vbe_trampoline_end - (uintptr_t)&vbe_trampoline_start;
    memcpy((void*)TRAMPOLINE_PHYS_BASE, &vbe_trampoline_start, trampoline_size);

    volatile uint64_t* vbe_target_cr3 = (volatile uint64_t*)( (uintptr_t)AP_TARGET_CR3 );
    volatile uint16_t* vbe_mode_to_set = (volatile uint16_t*)( (uintptr_t)AP_MODE_TO_SET );
    volatile uint16_t* vbe_status      = (volatile uint16_t*)( (uintptr_t)AP_VBE_STATUS );

    *vbe_target_cr3  = pt_begin;
    *vbe_mode_to_set = target_mode_id | 0x4000;
    *vbe_status      = 0;

    void (*jump_to_trampoline)(void) = (void (*)(void))TRAMPOLINE_PHYS_BASE;
    jump_to_trampoline();

    if (*vbe_status == 0x004F) {
        uint32_t* old_back_buffer = fb_back_buffer;

        SCREEN_WIDTH = width;
        SCREEN_HEIGHT = height;
        fb_back_buffer = new_back_buffer;

        fb_front_buffer = (uint32_t*)physical_base_pointer;
        
        lfb_buffer_addr = (uint32_t)virtual_to_physical(pt_begin, (uintptr_t)fb_back_buffer);
        lfb_mouse_addr = physical_base_pointer;

        vbe_hor_size = width;
        vbe_ver_size = height;

        uint64_t w_local = width;
        uint64_t h_local = height;
        uint64_t total_bytes = w_local * h_local * 4ULL;

        uint64_t phys_start = (uint64_t)physical_base_pointer;
        uint64_t page_start = phys_start & ~0xFFFULL;
        uint64_t alignment_offset = phys_start & 0xFFFULL; 

        uint64_t total_bytes_aligned = total_bytes + alignment_offset;
        size_t exact_pages = (total_bytes_aligned + 4095) / 4096;

        size_t total_pages = (exact_pages + 511) & ~511;

        if (total_pages == 0) total_pages = 512;

        map_page(page_start, page_start, 0x03);

        for (size_t page = 1; page < total_pages; page++) {
            uint64_t current_phys = page_start + (page * 4096);
            map_page(current_phys, current_phys, 0x03); 
        }

        if (old_back_buffer != NULL) {
            kfree(old_back_buffer);
        }

        draw_all_windows();
    } else {
        kfree(new_back_buffer);
    }

    __asm__ volatile("sti");
}

void mouse_settings_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    for (size_t i = 0; i < NUM_SETTINGS_BUTTONS; i++) {

        selected_settings_id = -1;

        if(click_y < 5) return;
        if(click_y > 5+(15*count_of_video)) return;

        selected_settings_id = (click_y - 5) / 15;

        if(is_left_pressed) {

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

        }
    }
}

void draw_settings_content(const window_t* win, int bx, int by, int bw, int bh) {

    if (!boot_info) {
        print_unicode("ERROR: Boot info is null!", bx, by+5, 0x00, 0x00, 0x00, 0xFF);
        return;
    }

    uint8_t* raw_bytes = (uint8_t*)( (uintptr_t)boot_info->vbe_modes_list);
    int internal_id = 0;

    count_of_video = 0;

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

        int8_t colorRed = 0x00;
        int8_t colorGreen = 0x00;
        int8_t colorBlue = 0x00;
        int8_t colorAlpha = 0x00;

        if(selected_settings_id == internal_id) {
            colorRed = 0x00;
            colorGreen = 0x56;
            colorBlue = 0xb3;
            colorAlpha = 0xFF;
        }

        draw_rect(bx, by+5+(15*internal_id), win->width - 20, 15, colorRed, colorGreen, colorBlue, colorAlpha);

        kprintf(bx, by+5+(15*internal_id), colorAlpha, colorAlpha, colorAlpha, 0xFF, "[%d] ID: 0x%x | %dx%d %dbpp | LFB: 0x%x\n", 
                internal_id, 
                mode_id, 
                width, 
                height, 
                bpp, 
                physical_base_pointer);

        internal_id++;
        count_of_video++;
    }

}

void openSettings() {
    createWindow(50, 50, 400, 300, "Settings", draw_settings_content, mouse_settings_content);
}