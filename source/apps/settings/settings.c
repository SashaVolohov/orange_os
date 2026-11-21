#include "core.h"
#include "settings.h"
#include "os_memory.h"

checkbox_t transparency_checkbox = { .rel_x = 5,  .rel_y = 5, .box_size = 10, .text = "Enable transparency", .is_pressed = 0, .is_checked = 1 };

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

        fb_front_buffer = (uint32_t*)(uintptr_t)physical_base_pointer;
        
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

    } else {
        kfree(new_back_buffer);
    }

    __asm__ volatile("sti");
}

void mouse_settings_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {

    int transparency_status = update_checkbox(&transparency_checkbox, click_x, click_y, is_left_pressed);
    if(transparency_status == 1) {
        transparency_gui = !transparency_gui;
    }

    selected_settings_id = -1;

    if(click_y < 20) return;
    if(click_y > 20+(15*count_of_video)) return;

    selected_settings_id = (click_y - 20) / 15;

    if(is_left_pressed) {

        uint8_t* raw_bytes = (uint8_t*)( (uintptr_t)boot_info->vbe_modes_list);
        int internal_id = 0;

        for (uint32_t i = 0; i < boot_info->vbe_modes_count; i++) {

            uint8_t* entry = raw_bytes + (i * 13);
            uint16_t mode_id               = *(uint16_t*)(entry + 0);
            uint16_t width                 = *(uint16_t*)(entry + 2);
            uint16_t height                = *(uint16_t*)(entry + 4);
            uint8_t  bpp                   = *(uint8_t*) (entry + 6);
            uint32_t physical_base_pointer = *(uint32_t*)(entry + 7);
            uint16_t pixels_per_line = *(uint16_t*)(entry + 11);

            if (mode_id < 0x100 || bpp != 32 || mode_id >= 0x200) {
                continue;
            }

            if(selected_settings_id == internal_id) {
                change_screen_resolution(mode_id, width, height, physical_base_pointer);
                boot_info->vbe_pitch = pixels_per_line;
                break;
            }

            internal_id++;
        }

    }

}

void draw_settings_content(const window_t* win, int bx, int by, int bw, int bh) {

    if (!boot_info) {
        draw_window_text(win, "ERROR: Boot info is null!", 0, 5, 0x00, 0x00, 0x00, 0xFF);
        return;
    }

    uint8_t* raw_bytes = (uint8_t*)( (uintptr_t)boot_info->vbe_modes_list);
    int internal_id = 0;

    count_of_video = 0;

    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    for (uint32_t i = 0; i < boot_info->vbe_modes_count; i++) {

        uint8_t* entry = raw_bytes + (i * 13);
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

        draw_window_rect(win, 0, 20+(15*internal_id), win->width - 20, 15, colorRed, colorGreen, colorBlue, colorAlpha);

        win_printf(win, 0, 20+(15*internal_id), colorAlpha, colorAlpha, colorAlpha, 0xFF, "[%d] ID: 0x%x | %dx%d %dbpp | LFB: 0x%x\n", 
                internal_id, 
                mode_id, 
                width, 
                height, 
                bpp, 
                physical_base_pointer);

        internal_id++;
        count_of_video++;
    }

    draw_window_checkbox(win, 0, 0, &transparency_checkbox);

}

void openSettings() {
    createWindow(50, 50, 400, 300, "Settings", draw_settings_content, mouse_settings_content);
}