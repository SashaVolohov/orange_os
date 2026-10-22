#include "core.h"
#include "gui.h"
#include "os_memory.h"

window_t Windows[100];

int is_start_pressed = 0;

int is_start_menu_open = 0;
int active_menu_item = -1;
int window_in_focus = 0;
int current_new_window_id = 0;

uint32_t* fb_back_buffer = NULL; 
extern uint8_t os_state;

extern uintptr_t background_file;
extern uint64_t background_size;

void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int curr_y = y; curr_y < y + height; curr_y++) {
        for (int curr_x = x; curr_x < x + width; curr_x++) {
            set_pixel(curr_x, curr_y, r, g, b, a);
        }
    }
}

void draw_line(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    
    int err = dx - dy;

    while (1) {
        set_pixel(x1, y1, r, g, b, a);

        if (x1 == x2 && y1 == y2) {
            break;
        }

        int e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void draw_close_icon(int x, int y) {
    static const uint8_t icon_data[7] = {
        0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41
    };

    for (int curr_y = 0; curr_y < 7; curr_y++) {
        uint8_t row = icon_data[curr_y];
        for (int curr_x = 0; curr_x < 7; curr_x++) {
            if ((row << curr_x) & 0x40) {
                set_pixel(x + curr_x, y + curr_y, 0xFF, 0xFF, 0xFF, 0xFF);
            }
        }
    }
}

void draw_taskbar_button(int x, int y, int w, int h, const char* text, int pressed) {
    draw_rect(x, y, w, h, 0x00, 0x00, 0x00, 0x32);

    if (!pressed) {
        draw_line(x, y, x + w - 1, y, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x, y, x, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0x80, 0x80, 0x80, 0xFF);

        print_unicode(text, x + (w / 2) - 20, y + (h / 2) - 6, 0xFF, 0xFF, 0xFF, 0xFF);
    } else {
        
        draw_line(x, y, x + w - 1, y, 0x80, 0x80, 0x80, 0xFF);
        draw_line(x, y, x, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
        
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);

        print_unicode(text, x + (w / 2) - 19, y + (h / 2) - 5, 0xFF, 0xFF, 0xFF, 0x00);
    }
}

uint8_t read_rtc_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void draw_clock_field(int x, int y, int w, int h) {
    draw_line(x, y, x + w - 1, y, 0x80, 0x80, 0x80, 0xFF);
    draw_line(x, y, x, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
    draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);

    uint8_t status_b = read_rtc_register(0x0B);
    uint8_t hours = read_rtc_register(0x04);
    uint8_t minutes = read_rtc_register(0x02);

    if (!(status_b & 0x04)) {
        hours = BCD_TO_BIN(hours);
        minutes = BCD_TO_BIN(minutes);
    }

    char time_str[8];
    time_str[0] = '0' + (hours / 10);
    time_str[1] = '0' + (hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (minutes / 10);
    time_str[4] = '0' + (minutes % 10);
    time_str[5] = '\0';

    print_unicode(time_str, x + 15, y + (h / 2) - 6, 0xFF, 0xFF, 0xFF, 0xFF);
}

void draw_start_menu() {
    if (!is_start_menu_open) return;

    int menu_w = 160;
    int menu_h = 220;
    int menu_x = 5;
    int menu_y = TASKBAR_Y - menu_h;

    draw_rect(menu_x + 20, menu_y, menu_w - 20, menu_h, 0xFF, 0xFF, 0xFF, 0xFF);

    draw_line(menu_x, menu_y, menu_x + menu_w - 1, menu_y, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(menu_x, menu_y, menu_x, menu_y + menu_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(menu_x, menu_y + menu_h - 1, menu_x + menu_w - 1, menu_y + menu_h - 1, 0x80, 0x80, 0x80, 0xFF);
    draw_line(menu_x + menu_w - 1, menu_y, menu_x + menu_w - 1, menu_y + menu_h - 1, 0x80, 0x80, 0x80, 0xFF);

    draw_rect(menu_x + 2, menu_y + 2, 20, menu_h - 4, 0x00, 0x00, 0x80, 0x64); 

    const char* menu_items[] = { "Calculator", "Notepad", "Device Manager", "Settings", "About OS", "Shut Down" };
    int item_count = 6;
    int item_start_x = menu_x + 26;

    for (int i = 0; i < item_count; i++) {
        int item_y = menu_y + 4 + (i * 36);
        int item_w = menu_w - 30;
        int item_h = 30;

        if (i == active_menu_item) {
            draw_rect(item_start_x, item_y, item_w, item_h, 0xB9, 0xC5, 0xD0, 0xFF);
            print_unicode(menu_items[i], item_start_x + 10, item_y + 9, 0xFF, 0xFF, 0xFF, 0xFF);
        } else {
            print_unicode(menu_items[i], item_start_x + 10, item_y + 9, 0x00, 0x00, 0x00, 0xFF);
        }
    }
}

void UpdateTaskbar(int mouse_x, int mouse_y, int mouse_click) {

    if(mouse_click == 0) return;

    int start_x = 85;
    int btn_w = 120;
    int gap = 4;
    int current_btn_x = start_x;

    for (int i = 0; i < 100; i++) {
        if (Windows[i].is_free) {
            if (mouse_x >= current_btn_x && mouse_x <= (current_btn_x + btn_w) &&
                mouse_y >= (TASKBAR_Y + 5) && mouse_y <= (TASKBAR_Y + 35)) {
                
                if (mouse_click) {
                    window_in_focus = Windows[i].id;

                    if (i == (int)100 - 1) {
                        is_start_menu_open = 0; 
                        break;
                    }

                    window_t target_window = Windows[i];

                    for (int q = i; q < (int)100 - 1; q++) {
                        Windows[q] = Windows[q + 1];
                    }

                    Windows[100 - 1] = target_window;
                    
                    is_start_menu_open = 0; 
                    break;
                }
            }
            current_btn_x += (btn_w + gap);
        }
    }

    draw_all_windows();

}

void draw_taskbar() {

    draw_gradient_rect_vertical(0, TASKBAR_Y, SCREEN_WIDTH, TASKBAR_HEIGHT, 0x4580A164, 0x1D538164);

    //draw_rect(0, TASKBAR_Y, SCREEN_WIDTH, TASKBAR_HEIGHT, 0xC0, 0xC0, 0xC0, 0xFF);

    draw_line(0, TASKBAR_Y, SCREEN_WIDTH - 1, TASKBAR_Y, 0xFF, 0xFF, 0xFF, 0xFF);

    draw_taskbar_button(5, TASKBAR_Y + 5, 70, 30, "Start", is_start_pressed);

    draw_clock_field(SCREEN_WIDTH - 80, TASKBAR_Y + 5, 75, 30);

    int start_x = 85;
    int btn_w = 120;
    int btn_h = 30;
    int btn_y = TASKBAR_Y + 5;
    int gap = 4;

    int current_btn_x = start_x;

    for (int i = 0; i < 100; i++) {
        if (Windows[i].is_free) {
            
            int is_active = (window_in_focus == Windows[i].id);
            
            draw_rect(current_btn_x, btn_y, btn_w, btn_h, 0x00, 0x00, 0x00, 0x32);
            
            if (!is_active) {
                draw_line(current_btn_x, btn_y, current_btn_x + btn_w - 1, btn_y, 0xFF, 0xFF, 0xFF, 0xFF);
                draw_line(current_btn_x, btn_y, current_btn_x, btn_y + btn_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
                draw_line(current_btn_x, btn_y + btn_h - 1, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0x80, 0x80, 0x80, 0xFF);
                draw_line(current_btn_x + btn_w - 1, btn_y, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0x80, 0x80, 0x80, 0xFF);

                print_unicode(Windows[i].title, current_btn_x + 8, btn_y + 9, 0xFF, 0xFF, 0xFF, 0xFF);
            } else {
                draw_line(current_btn_x, btn_y, current_btn_x + btn_w - 1, btn_y, 0x80, 0x80, 0x80, 0xFF);
                draw_line(current_btn_x, btn_y, current_btn_x, btn_y + btn_h - 1, 0x80, 0x80, 0x80, 0xFF);
                draw_line(current_btn_x, btn_y + btn_h - 1, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
                draw_line(current_btn_x + btn_w - 1, btn_y, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);

                print_unicode(Windows[i].title, current_btn_x + 9, btn_y + 10, 0xFF, 0xFF, 0xFF, 0xFF);
            }
            
            current_btn_x += (btn_w + gap);
        }
    }
}

uint8_t lerp_channel(uint8_t c1, uint8_t c2, uint32_t t, uint32_t max) {
    if (max == 0) return c1;
    
    int32_t delta = (int32_t)c2 - (int32_t)c1;
    
    return (uint8_t)((int32_t)c1 + (delta * (int32_t)t) / (int32_t)max);
}

uint32_t lerp_color(uint32_t color1, uint32_t color2, uint32_t t, uint32_t max) {
    if (max == 0) return color1;
    
    Color32 c1 = *(Color32*)&color1;
    Color32 c2 = *(Color32*)&color2;
    Color32 result;

    result.r = lerp_channel(c1.r, c2.r, t, max);
    result.g = lerp_channel(c1.g, c2.g, t, max);
    result.b = lerp_channel(c1.b, c2.b, t, max);
    result.a = lerp_channel(c1.a, c2.a, t, max);

    return *(uint32_t*)&result;
}

void draw_gradient_rect_horizontal(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color_left, uint32_t color_right) {

    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < w; j++) {
            uint32_t raw_color = lerp_color(color_left, color_right, j, w - 1);
            Color32 color = *(Color32*)&raw_color;
            set_pixel(x + j, y + i, color.r, color.g, color.b, color.a);
        }
    }
}

void draw_gradient_rect_vertical(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color_top, uint32_t color_bottom) {
    for (uint32_t j = 0; j < h; j++) {
        uint32_t raw_color = lerp_color(color_top, color_bottom, j, h - 1);
        Color32 color = *(Color32*)&raw_color;

        for (uint32_t i = 0; i < w; i++) {
            set_pixel(x + i, y + j, color.r, color.g, color.b, color.a);
        }
    }
}

void draw_window(const window_t* win) {
    draw_line(win->x, win->y, win->x + win->width - 1, win->y, 0x40, 0x40, 0x40, 0xFF);
    draw_line(win->x, win->y + 1, win->x + win->width - 1, win->y + 1, 0x40, 0x40, 0x40, 0xFF);

    draw_line(win->x, win->y + win->height - 2, win->x + win->width - 1, win->y + win->height - 2, 0x40, 0x40, 0x40, 0xFF);
    draw_line(win->x, win->y + win->height - 1, win->x + win->width - 1, win->y + win->height - 1, 0x40, 0x40, 0x40, 0xFF);

    draw_line(win->x, win->y, win->x, win->y + win->height - 1, 0x40, 0x40, 0x40, 0xFF);
    draw_line(win->x + 1, win->y, win->x + 1, win->y + win->height - 1, 0x40, 0x40, 0x40, 0xFF);

    draw_line(win->x + win->width - 2, win->y, win->x + win->width - 2, win->y + win->height - 1, 0x40, 0x40, 0x40, 0xFF);
    draw_line(win->x + win->width - 2 + 1, win->y, win->x + win->width - 2 + 1, win->y + win->height - 1, 0x40, 0x40, 0x40, 0xFF);

    draw_rect(win->x + 2, win->y + 2 + 22, win->width - 4, win->height - 4 - 22, 0xC0, 0xC0, 0xC0, 0xFF);

    draw_gradient_rect_horizontal(win->x + 2, win->y + 2, win->width - 4, 22, 0x0050A064, 0x00285064); // Header

    if (win->title) {
        print_unicode(win->title, win->x + 4, win->y + 7, 0xFF, 0xFF, 0xFF, 0xFF);
    }

    int btn_x = win->x + win->width - 22; 
    int btn_y = win->y + 5;
    
    draw_rect(btn_x, btn_y, 16, 16, 0xE0, 0x30, 0x30, 0xFF);

    draw_close_icon(btn_x + 5, btn_y + 4);

    int body_x = win->x + 2;
    int body_y = win->y + 2 + 22;
    int body_w = win->width - 4;
    int body_h = win->height - 4 - 22;

    if (win->on_draw != NULL) {
        win->on_draw(win, body_x, body_y, body_w, body_h);
    }
}

void draw_all_windows()
{

    if (fb_back_buffer == NULL) {
        return;
    }

    draw_bmp((uint8_t*)background_file, background_size, 0, 0);

    for (size_t i = 0; i < 100; i++) {
        if (Windows[i].is_free) {
            draw_window(&Windows[i]);
        }
    }

    if(os_state == 1) {

        draw_taskbar();
        draw_start_menu();

    }

    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH * SCREEN_HEIGHT);
}

void flip_gui() {
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH * SCREEN_HEIGHT);
}

void draw_button(int body_x, int body_y, const button_t* btn) {
    int abs_x = body_x + btn->rel_x;
    int abs_y = body_y + btn->rel_y;

    uint8_t face_color = btn->is_pressed ? 0xAA : 0xDD;
    uint8_t top_light  = btn->is_pressed ? 0x40 : 0xFF;
    uint8_t bottom_dark = btn->is_pressed ? 0xFF : 0x40; 

    draw_rect(abs_x, abs_y, btn->width, btn->height, face_color, face_color, face_color, 0xFF);

    draw_rect(abs_x, abs_y, btn->width, 1, top_light, top_light, top_light, 0xFF);
    draw_rect(abs_x, abs_y, 1, btn->height, top_light, top_light, top_light, 0xFF);
    draw_rect(abs_x, abs_y + btn->height - 1, btn->width, 1, bottom_dark, bottom_dark, bottom_dark, 0xFF);
    draw_rect(abs_x + btn->width - 1, abs_y, 1, btn->height, bottom_dark, bottom_dark, bottom_dark, 0xFF);

    if (btn->text) {
        int text_len = 0;
        while (btn->text[text_len]) text_len++;
        
        int text_w = text_len * 8; 
        int text_x = abs_x + (btn->width - text_w) / 2;
        int text_y = abs_y + (btn->height - 12) / 2;

        if (btn->is_pressed) {
            text_x += 1;
            text_y += 1;
        }

        print_unicode(btn->text, text_x, text_y, 0x00, 0x00, 0x00, 0xFF);
    }
}

int is_button_clicked(button_t* btn, int click_x, int click_y, int is_left_pressed) {
    if (click_x >= btn->rel_x && click_x < (btn->rel_x + btn->width) &&
        click_y >= btn->rel_y && click_y < (btn->rel_y + btn->height)) {
        
        btn->is_pressed = is_left_pressed;

        draw_all_windows();

        return is_left_pressed;
    }

    btn->is_pressed = 0;
    return 0;
}

int createWindow(int x, int y, int width, int height, const char* title, void (*on_draw)(const struct window* win, int body_x, int body_y, int body_w, int body_h), void (*on_mouse)(const struct window* win, int click_x, int click_y, int is_left_pressed)) {
    for(int i = 0; i < 100; i++) {
        if(Windows[i].is_free == 0) {
            Windows[i].is_free = 1;
            Windows[i].x = x;
            Windows[i].y = y;
            Windows[i].width = width;
            Windows[i].height = height;
            Windows[i].id = current_new_window_id;
            
            current_new_window_id++;

            Windows[i].title = kmalloc(strlen(title)+1);
            memcpy(Windows[i].title, title, strlen(title)+1);

            if(on_draw != NULL) {
                Windows[i].on_draw = on_draw;
            } else {
                Windows[i].on_draw = NULL;
            }

            if(on_mouse != NULL) {
                Windows[i].on_mouse = on_mouse;
            } else {
                Windows[i].on_mouse = NULL;
            }

            window_in_focus = Windows[i].id;

            return i;

        }
    }

    return -1;
}

int is_mouse_inside_window(const window_t* win, int mx, int my) {
    if (win->is_free == 0) return 0;
    
    return (mx >= win->x && mx < (win->x + win->width) &&
            my >= win->y && my < (win->y + win->height));
}

window_t* window_focus_handle(int mouse_x, int mouse_y) {
    int found_index = -1;

    for (int i = (int)100 - 1; i >= 0; i--) {
        if (Windows[i].is_free == 1 && is_mouse_inside_window(&Windows[i], mouse_x, mouse_y)) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        return NULL; 
    }

    window_in_focus = Windows[found_index].id;

    if (found_index == (int)100 - 1) {
        return &Windows[found_index];
    }

    window_t target_window = Windows[found_index];

    for (int i = found_index; i < (int)100 - 1; i++) {
        Windows[i] = Windows[i + 1];
    }

    Windows[100 - 1] = target_window;

    return &Windows[100 - 1];
}

window_t* get_window_focus(int mouse_x, int mouse_y) {
    int found_index = -1;

    for (int i = (int)100 - 1; i >= 0; i--) {
        if (Windows[i].is_free == 1 && Windows[i].id == window_in_focus && is_mouse_inside_window(&Windows[i], mouse_x, mouse_y)) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        return NULL; 
    }

    return &Windows[found_index];
}

void init_double_buffering() {
    fb_back_buffer = (uint32_t*)kmalloc(SCREEN_SIZE);
    
    if(fb_back_buffer == 0) {
        KeBugCheck(0x20);
    }

    lfb_buffer_addr = (uint32_t)virtual_to_physical(pt_begin, (uintptr_t)fb_back_buffer);
}

void set_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    if (fb_back_buffer == NULL) return;

    size_t index = ((size_t)y * (size_t)SCREEN_WIDTH) + (size_t)x;

    if (a == 0) return;

    if (a == 255) {
        fb_back_buffer[index] = (255U << 24) | (r << 16) | (g << 8) | b;
        return;
    }

    uint32_t bg_color = fb_back_buffer[index];

    uint8_t bg_r = (bg_color >> 16) & 0xFF;
    uint8_t bg_g = (bg_color >> 8)  & 0xFF;
    uint8_t bg_b = bg_color         & 0xFF;

    uint8_t inv_a = 255 - a; 

    uint8_t out_r = (uint8_t)(((r * a) + (bg_r * inv_a)) >> 8);
    uint8_t out_g = (uint8_t)(((g * a) + (bg_g * inv_a)) >> 8);
    uint8_t out_b = (uint8_t)(((b * a) + (bg_b * inv_a)) >> 8);

    fb_back_buffer[index] = (255U << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

volatile uint8_t gcc_killer = 0;

__attribute__((noinline, target("no-sse", "no-mmx", "no-avx")))
void draw_bmp(uint8_t* file_data, size_t file_size, uint32_t start_x, uint32_t start_y) {

    if (file_size < 54) return;
    if (file_data[0] != 0x42 || file_data[1] != 0x4D) return; 

    // Максимально тупой и прямолинейный сбор данных без арифметики
    uint32_t pixel_offset = file_data[10] | (file_data[11] << 8) | (file_data[12] << 16) | (file_data[13] << 24);
    
    uint32_t w_val = file_data[18] | (file_data[19] << 8) | (file_data[20] << 16) | (file_data[21] << 24);
    int32_t raw_w = *(int32_t*)&w_val;

    uint32_t h_val = file_data[22] | (file_data[23] << 8) | (file_data[24] << 16) | (file_data[25] << 24);
    int32_t raw_h = *(int32_t*)&h_val;

    uint32_t bpp = file_data[28] | (file_data[29] << 8);

    if (raw_w <= 0 || raw_w > 4096 || raw_h == 0 || raw_h > 4096 || raw_h < -4096) return;
    if (bpp != 24 && bpp != 32) return;
    if (pixel_offset >= file_size) return;

    uint32_t img_w = (uint32_t)raw_w;
    uint32_t img_h = (raw_h < 0) ? (uint32_t)(-raw_h) : (uint32_t)raw_h;
    uint32_t bytes_per_pixel = bpp / 8;
    
    uint32_t row_stride = img_w * bytes_per_pixel;
    if (row_stride % 4 != 0) row_stride += 4 - (row_stride % 4);

    uint8_t* pixel_data = file_data + pixel_offset;
    int is_inverted = (raw_h > 0);

    for (uint32_t y = 0; y < img_h; y++) {
        uint32_t file_y = is_inverted ? (img_h - 1 - y) : y;
        uint8_t* row = pixel_data + (file_y * row_stride);

        // Проверка, что вся строка лежит внутри файла
        if (row < file_data || (row + row_stride) > (file_data + file_size)) {
            return;
        }

        for (uint32_t x = 0; x < img_w; x++) {
            uint8_t* pixel = row + (x * bytes_per_pixel);

            uint8_t b = pixel[0];
            uint8_t g = pixel[1];
            uint8_t r = pixel[2];
            uint8_t a = (bpp == 32) ? pixel[3] : 255;

            // Теперь это честный, изолированный вызов функции!
            set_pixel((int32_t)(start_x + x), (int32_t)(start_y + y), r, g, b, a);
        }
    }
}