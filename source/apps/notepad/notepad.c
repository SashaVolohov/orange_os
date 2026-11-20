#include "core.h"

char* notepad_buffer = NULL;
int64_t notepad_buffer_size = 4096;

static button_t notepad_buttons[] = {
    { .rel_x = 10,  .rel_y = 3,  .width = 90, .height = 25, .text = "Create file", .is_pressed = 0 },
    { .rel_x = 100,  .rel_y = 3,  .width = 90, .height = 25, .text = "Open", .is_pressed = 0 },
    { .rel_x = 210,  .rel_y = 3,  .width = 90, .height = 25, .text = "Save", .is_pressed = 0 },
    { .rel_x = 300, .rel_y = 3,  .width = 90, .height = 25, .text = "Save as...", .is_pressed = 0 },
};

#define NUM_NOTEPAD_BUTTONS (sizeof(notepad_buttons) / sizeof(notepad_buttons[0]))

void draw_notepad_content(const window_t* win, int bx, int by, int bw, int bh) {

    int toolbar_h = 30;
    
    draw_window_gradient_rect_vertical(win, 0, 0, win->width - 2, toolbar_h, 0xF2F2F2FF, 0xE5E5E5FF); 

    draw_window_rect(win, 0, 0 + toolbar_h - 1, win->width - 2, 1, 0xB5, 0xB5, 0xB5, 0xFF);

    int text_area_y = 0 + toolbar_h;
    int text_area_h = win->height - toolbar_h;
    
    draw_window_rect(win, 0, text_area_y, win->width - 2, text_area_h, 0xFF, 0xFF, 0xFF, 0xFF);

    draw_window_rect(win, 0, text_area_y, win->width - 2, 1, 0x90, 0x90, 0x90, 0xFF); // Верхняя внутренняя грань
    draw_window_rect(win, 0, text_area_y, 1, text_area_h, 0x90, 0x90, 0x90, 0xFF); // Левая внутренняя грань
    
    for (size_t i = 0; i < NUM_NOTEPAD_BUTTONS; i++) {
        draw_window_button(win, 0, 0, &notepad_buttons[i]);
        if(notepad_buttons[i].is_pressed != 0) notepad_buttons[i].is_pressed = 0;
    }

}

void mouse_notepad_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    for (size_t i = 0; i < NUM_NOTEPAD_BUTTONS; i++) {

        if (is_button_clicked(&notepad_buttons[i], click_x, click_y, is_left_pressed)) {

            char key = notepad_buttons[i].text[0];

            
            
            break;
        }
    }
}

static button_t btn_notepad_error   = { .rel_x = 200,  .rel_y = 80, .width = 80, .height = 24, .text = "OK",   .is_pressed = 0 };

void draw_notepad_error_content(const window_t* win, int bx, int by, int bw, int bh) {
    draw_window_button(win, 0, 0, &btn_notepad_error);
    win_printf(win, 5, 5, 0x00, 0x00, 0x00, 0xFF, "Failed to allocate memory to launch Notepad.");
}

void mouse_notepad_error_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_notepad_error, click_x, click_y, is_left_pressed)) {
        btn_notepad_error.is_pressed = 0;
        ((window_t*)win)->is_free = 0;
    }
}

void openNotepad() {

    if(notepad_buffer == NULL) {
        notepad_buffer = (char*)kmalloc(notepad_buffer_size);
        if(notepad_buffer == 0) {
            createWindow(200, 300, 300, 150, "OrangeOS Error", draw_notepad_error_content, mouse_notepad_error_content);
            return;
        }

    }

    createWindow(200, 200, 400, 300, "Notepad", draw_notepad_content, mouse_notepad_content);
}