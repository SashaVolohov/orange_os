#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "core.h"
#include "os_memory.h"

#include "stb_truetype.h"

window_t Windows[100];
spinlock_t window_lock = {0};

uint64_t dwm_time = 0;

int is_start_pressed = 0;

int is_start_menu_open = 0;
int active_menu_item = -1;
int window_in_focus = 0;
int current_new_window_id = 1;

uint32_t* fb_back_buffer = NULL; 
extern uint8_t os_state;

extern uintptr_t background_file;
extern uint64_t background_size;

extern uintptr_t true_type_font_system_file;
extern uint64_t true_type_font_system_size;

uint8_t transparency_gui = 1;

stbtt_fontinfo system_font;
float font_scale;

void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int curr_y = y; curr_y < y + height; curr_y++) {
        for (int curr_x = x; curr_x < x + width; curr_x++) {
            set_pixel(curr_x, curr_y, r, g, b, a);
        }
    }
}

void draw_window_rect(const window_t* win, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int curr_y = y; curr_y < y + height; curr_y++) {
        for (int curr_x = x; curr_x < x + width; curr_x++) {
            set_window_pixel(win, curr_x, curr_y, r, g, b, a);
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

void draw_window_line(const window_t* win, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    
    int err = dx - dy;

    while (1) {
        set_window_pixel(win, x1, y1, r, g, b, a);

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

void draw_close_icon(int x, int y, uint8_t alpha) {
    static const uint8_t icon_data[7] = {
        0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41
    };

    for (int curr_y = 0; curr_y < 7; curr_y++) {
        uint8_t row = icon_data[curr_y];
        for (int curr_x = 0; curr_x < 7; curr_x++) {
            if ((row << curr_x) & 0x40) {
                set_pixel(x + curr_x, y + curr_y, 0xFF, 0xFF, 0xFF, alpha);
            }
        }
    }
}

void draw_minimize_icon(int x, int y, uint8_t alpha) {
    static const uint8_t icon_data[7] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x3E,
        0x00
    };

    for (int curr_y = 0; curr_y < 7; curr_y++) {
        uint8_t row = icon_data[curr_y];
        for (int curr_x = 0; curr_x < 7; curr_x++) {
            if ((row << curr_x) & 0x40) {
                set_pixel(x + curr_x, y + curr_y, 0xFF, 0xFF, 0xFF, alpha);
            }
        }
    }
}

__attribute__((target("sse,sse2,mmx")))
void draw_taskbar_button(int x, int y, int w, int h, const char* text, int pressed) {

    draw_rect(x, y, w, h, 0x00, 0x00, 0x00, 0x32);

    if (!pressed) {
        draw_line(x, y, x + w - 1, y, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x, y, x, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0x80, 0x80, 0x80, 0xFF);

        draw_string_utf8(x + (w / 2) - 20, y + (h / 2) - 6, text, 0xFF, 0xFF, 0xFF, 0xFF);
    } else {
        
        draw_line(x, y, x + w - 1, y, 0x80, 0x80, 0x80, 0xFF);
        draw_line(x, y, x, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
        
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);

        draw_string_utf8(x + (w / 2) - 19, y + (h / 2) - 5, text, 0xFF, 0xFF, 0xFF, 0x00);
    }
}

uint8_t read_rtc_register(int reg) {
    outb(0x70, reg);
    return inb(0x71);
}

__attribute__((target("sse,sse2,mmx")))
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

    char time_str[16];
    time_str[0] = '0' + (hours / 10);
    time_str[1] = '0' + (hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (minutes / 10);
    time_str[4] = '0' + (minutes % 10);
    time_str[5] = '\0';

    draw_string_utf8(x + 15, y + (h / 2) - 6, time_str, 0xFF, 0xFF, 0xFF, 0xFF);
}

__attribute__((target("sse,sse2,mmx")))
void draw_start_menu() {
    if (!is_start_menu_open) return;

    int menu_w = 160;
    int menu_h = 340;
    int menu_x = 5;
    int menu_y = TASKBAR_Y - menu_h;

    draw_rect(menu_x + 20, menu_y, menu_w - 20, menu_h, 0xFF, 0xFF, 0xFF, 0xFF);

    draw_line(menu_x, menu_y, menu_x + menu_w - 1, menu_y, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(menu_x, menu_y, menu_x, menu_y + menu_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(menu_x, menu_y + menu_h - 1, menu_x + menu_w - 1, menu_y + menu_h - 1, 0x80, 0x80, 0x80, 0xFF);
    draw_line(menu_x + menu_w - 1, menu_y, menu_x + menu_w - 1, menu_y + menu_h - 1, 0x80, 0x80, 0x80, 0xFF);

    draw_rect(menu_x + 2, menu_y + 2, 20, menu_h - 4, 0x00, 0x00, 0x80, 0x64); 

    const char* menu_items[] = { "File Manager", "Browser", "Calculator", "Notepad", "Device Manager", "Terminal", "Settings", "About OS", "Shut Down" };
    (void)menu_items;
    int item_count = 9;
    int item_start_x = menu_x + 26;

    for (int i = 0; i < item_count; i++) {
        int item_y = menu_y + 4 + (i * 36);
        int item_w = menu_w - 30;
        int item_h = 30;

        if (i == active_menu_item) {
            draw_rect(item_start_x, item_y, item_w, item_h, 0xB9, 0xC5, 0xD0, 0xFF);
            draw_string_utf8(item_start_x + 10, item_y + 9, menu_items[i], 0xFF, 0xFF, 0xFF, 0xFF);
        } else {
            draw_string_utf8(item_start_x + 10, item_y + 9, menu_items[i], 0x00, 0x00, 0x00, 0xFF);
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
                    Windows[i].is_visible = 1;

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

}

__attribute__((target("sse,sse2,mmx")))
void draw_taskbar() {
    int half_h = TASKBAR_HEIGHT / 2;
    draw_gradient_rect_vertical(0, TASKBAR_Y, SCREEN_WIDTH, half_h, 0x4580A164, 0x2D608064);
    draw_gradient_rect_vertical(0, TASKBAR_Y + half_h, SCREEN_WIDTH, TASKBAR_HEIGHT - half_h, 0x25507064, 0x1D538164);

    draw_rect(0, TASKBAR_Y, SCREEN_WIDTH, 1, 0xFF, 0xFF, 0xFF, 0x90);

    draw_taskbar_button(5, TASKBAR_Y + 5, 70, 30, "Start", is_start_pressed);
    draw_clock_field(SCREEN_WIDTH - 80, TASKBAR_Y + 5, 75, 30);

    int start_x = 85;
    int btn_w = 120;
    int btn_h = 30;
    int btn_y = TASKBAR_Y + 5;
    int gap = 4;
    int current_btn_x = start_x;

    for (int i = 0; i < 100; i++) {
        if (Windows[i].is_free == 1) {
            
            int is_active = (window_in_focus == Windows[i].id);

            int mx = (int)global_mouse_x;
            int my = (int)global_mouse_y;
            
            int is_hovered = (mx >= current_btn_x && mx < (current_btn_x + btn_w) &&
                              my >= btn_y         && my < (btn_y + btn_h));

            char clipped_title[128];
            int max_text_w = btn_w - 22;
            truncate_utf8_to_width(Windows[i].title, clipped_title, sizeof(clipped_title), max_text_w);
            
            if (!is_active) {
                if (is_hovered) {
                    draw_rect(current_btn_x, btn_y, btn_w, btn_h, 0xE2, 0x93, 0x41, 0x40); 
                    
                    int inner_half = (btn_h - 2) / 2;
                    draw_gradient_rect_vertical(current_btn_x + 1, btn_y + 1, btn_w - 2, inner_half, 0xFFE0B050, 0xD2934160);
                    draw_gradient_rect_vertical(current_btn_x + 1, btn_y + 1 + inner_half, btn_w - 2, (btn_h - 2) - inner_half, 0xD2934170, 0xA2632190);

                    draw_rect(current_btn_x + 1, btn_y, btn_w - 2, 1, 0xFF, 0xFF, 0xFF, 0x70);
                    draw_rect(current_btn_x + 1, btn_y + btn_h - 1, btn_w - 2, 1, 0xD2, 0x93, 0x41, 0x90);

                    draw_string_utf8(current_btn_x + 12, btn_y + 9, clipped_title, 0xFF, 0xFF, 0xFF, 0xFF);
                } else {
                    draw_rect(current_btn_x, btn_y, btn_w, btn_h, 0xFF, 0xFF, 0xFF, 0x15);
                    draw_gradient_rect_vertical(current_btn_x + 1, btn_y + 1, btn_w - 2, btn_h - 2, 0xFFFFFF20, 0x00000030);

                    draw_rect(current_btn_x + 1, btn_y, btn_w - 2, 1, 0xFF, 0xFF, 0xFF, 0x40);
                    draw_rect(current_btn_x + 1, btn_y + btn_h - 1, btn_w - 2, 1, 0x00, 0x00, 0x00, 0x50);

                    draw_string_utf8(current_btn_x + 12, btn_y + 9, clipped_title, 0xDF, 0xE7, 0xEB, 0xFF);
                }
            } else {
                uint8_t alpha_boost = is_hovered ? 0x20 : 0x00;

                draw_rect(current_btn_x, btn_y, btn_w, btn_h, 0x3A, 0x6A, 0x8B, 0xB0 + alpha_boost); 
                
                int inner_half = (btn_h - 2) / 2;
                draw_gradient_rect_vertical(current_btn_x + 1, btn_y + 1, btn_w - 2, inner_half, 0x5D9BC6A0 + alpha_boost, 0x4A7A9B90 + alpha_boost);
                draw_gradient_rect_vertical(current_btn_x + 1, btn_y + 1 + inner_half, btn_w - 2, (btn_h - 2) - inner_half, 0x2A5A7BB0, 0x153A54DF);

                draw_rect(current_btn_x + 1, btn_y + 1, btn_w - 2, 1, 0xFF, 0xFF, 0xFF, 0x80);
                draw_rect(current_btn_x + 1, btn_y + 2, 1, btn_h - 4, 0xFF, 0xFF, 0xFF, 0x30);
                draw_rect(current_btn_x + btn_w - 2, btn_y + 2, 1, btn_h - 4, 0xFF, 0xFF, 0xFF, 0x30);

                uint8_t neon_alpha = is_hovered ? 0xFF : 0xC0;
                draw_rect(current_btn_x + 4, btn_y + btn_h - 2, btn_w - 8, 1, 0x00, 0xFF, 0xCC, neon_alpha); 

                draw_string_utf8(current_btn_x + 12, btn_y + 9, clipped_title, 0xFF, 0xFF, 0xFF, 0xFF);
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

void draw_window_gradient_rect_vertical(const window_t* win, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color_top, uint32_t color_bottom) {
    for (uint32_t j = 0; j < h; j++) {
        uint32_t raw_color = lerp_color(color_top, color_bottom, j, h - 1);
        Color32 color = *(Color32*)&raw_color;

        for (uint32_t i = 0; i < w; i++) {
            set_window_pixel(win, x + i, y + j, color.r, color.g, color.b, color.a);
        }
    }
}

inline uint32_t blend_pixel(uint32_t src, uint32_t dst, uint8_t opacity) {
    uint32_t src_a = ((src >> 24) & 0xFF) * opacity / 255;
    uint32_t dst_a = (dst >> 24) & 0xFF;

    if (src_a == 0) return dst;
    if (src_a == 255) return src;

    uint32_t src_r = (src >> 16) & 0xFF;
    uint32_t src_g = (src >> 8)  & 0xFF;
    uint32_t src_b = src         & 0xFF;

    uint32_t dst_r = (dst >> 16) & 0xFF;
    uint32_t dst_g = (dst >> 8)  & 0xFF;
    uint32_t dst_b = dst         & 0xFF;

    uint32_t res_a = src_a + (dst_a * (255 - src_a) >> 8);

    uint32_t res_r = ((src_r * src_a) + (dst_r * dst_a * (255 - src_a) >> 8)) / res_a;
    uint32_t res_g = ((src_g * src_a) + (dst_g * dst_a * (255 - src_a) >> 8)) / res_a;
    uint32_t res_b = ((src_b * src_a) + (dst_b * dst_a * (255 - src_a) >> 8)) / res_a;

    return (res_a << 24) | (res_r << 16) | (res_g << 8) | res_b;
}

void draw_window_buffer(uint32_t* back_buffer, int screen_width, int screen_height, const window_t* win) {
    if (!win->is_visible || win->current_alpha == 0) return;

    for (int wy = 0; wy < win->height - 26; wy++) {
        int screen_y = win->y + (wy+24);

        if (screen_y < 0 || screen_y >= screen_height) continue;

        for (int wx = 0; wx < win->width-2; wx++) {
            int screen_x = win->x + wx + 2;
            if (screen_x < 0 || screen_x >= screen_width) continue;

            int win_pixel_index = wy * win->width + wx + 2;
            
            int screen_pixel_index = screen_y * screen_width + screen_x;

            uint32_t src_color = win->buffer[win_pixel_index];
            uint32_t dst_color = back_buffer[screen_pixel_index];

            if (win->current_alpha == 255) {

                uint8_t a = (src_color >> 24) & 0xFF;
                uint8_t r = (src_color >> 16) & 0xFF;
                uint8_t g = (src_color >> 8)  & 0xFF;
                uint8_t b =  src_color & 0xFF;

                set_pixel(screen_x, screen_y, r, g, b, a);
            } else {

                uint32_t blend = blend_pixel(src_color, dst_color, win->current_alpha);

                uint8_t a = (blend >> 24) & 0xFF;
                uint8_t r = (blend >> 16) & 0xFF;
                uint8_t g = (blend >> 8)  & 0xFF;
                uint8_t b = blend & 0xFF;

                set_pixel(screen_x, screen_y, r, g, b, a);

            }
        }
    }
}

inline uint32_t set_alpha_rgba(uint32_t color, uint8_t max_alpha) {
    uint8_t current_alpha = color & 0xFF;

    if (current_alpha > max_alpha) {
        return (color & 0xFFFFFF00) | max_alpha;
    }

    return color;
}

__attribute__((target("sse,sse2,mmx")))
void draw_window(const window_t* win) {

    int shadow_offset_x = 4;
    int shadow_offset_y = 4;

    for (int layer = 3; layer >= 1; layer--) {
        uint8_t shadow_alpha = 0x0C * (4 - layer);
        if(win->current_alpha < shadow_alpha) {
            shadow_alpha = win->current_alpha;
        }
        
        int sx = win->x + shadow_offset_x - layer;
        int sy = win->y + shadow_offset_y - layer;
        int sw = win->width + (layer * 2);
        int sh = win->height + (layer * 2);
        
        for (int ty = 0; ty < sh; ty++) {
            for (int tx = 0; tx < sw; tx++) {
                set_pixel(sx + tx, sy + ty, 0x00, 0x00, 0x00, shadow_alpha);
            }
        }
    }

    draw_line(win->x, win->y, win->x + win->width - 1, win->y, 0x40, 0x40, 0x40, win->current_alpha);
    draw_line(win->x, win->y + 1, win->x + win->width - 1, win->y + 1, 0x40, 0x40, 0x40, win->current_alpha);

    draw_line(win->x, win->y + win->height - 2, win->x + win->width - 1, win->y + win->height - 2, 0x40, 0x40, 0x40, win->current_alpha);
    draw_line(win->x, win->y + win->height - 1, win->x + win->width - 1, win->y + win->height - 1, 0x40, 0x40, 0x40, win->current_alpha);

    draw_line(win->x, win->y, win->x, win->y + win->height - 1, 0x40, 0x40, 0x40, win->current_alpha);
    draw_line(win->x + 1, win->y, win->x + 1, win->y + win->height - 1, 0x40, 0x40, 0x40, win->current_alpha);

    draw_line(win->x + win->width - 2, win->y, win->x + win->width - 2, win->y + win->height - 1, 0x40, 0x40, 0x40, win->current_alpha);
    draw_line(win->x + win->width - 2 + 1, win->y, win->x + win->width - 2 + 1, win->y + win->height - 1, 0x40, 0x40, 0x40, win->current_alpha);

    draw_rect(win->x + 2, win->y + 2 + 22, win->width - 4, win->height - 4 - 22, 0xC0, 0xC0, 0xC0, win->current_alpha);

    int header_x = win->x + 2;
    int header_y = win->y + 2;
    int header_w = win->width - 4;

    draw_gradient_rect_vertical(header_x, header_y, header_w, 11, set_alpha_rgba(0x0064B4D2, win->current_alpha), set_alpha_rgba(0x004682A0, win->current_alpha));
    draw_gradient_rect_vertical(header_x, header_y + 11, header_w, 11, set_alpha_rgba(0x00326482, win->current_alpha), set_alpha_rgba(0x001E3C50, win->current_alpha));

    //draw_gradient_rect_horizontal(win->x + 2, win->y + 2, win->width - 4, 22, 0x0050A064, 0x00285064); // Header

    if (win->title) {
        draw_string_utf8(win->x + 4, win->y + 7, win->title, 0xFF, 0xFF, 0xFF, win->current_alpha);
    }

    int btn_x = win->x + win->width - 22; 
    int btn_y = win->y + 5;
    
    draw_rect(btn_x, btn_y, 16, 16, 0xE0, 0x30, 0x30, win->current_alpha);

    draw_close_icon(btn_x + 4, btn_y + 4, win->current_alpha);

    draw_rect(btn_x-20, btn_y, 16, 16, 0x4A, 0x7A, 0x9B, win->current_alpha);
    draw_minimize_icon(btn_x - 16, btn_y + 4, win->current_alpha);

    int body_x = win->x + 2;
    int body_y = win->y + 2 + 22;
    int body_w = win->width - 4;
    int body_h = win->height - 4 - 22;

    if (win->on_draw != NULL) {
        win->on_draw(win, body_x, body_y, body_w, body_h);
    }

    draw_window_buffer(fb_back_buffer, SCREEN_WIDTH, SCREEN_HEIGHT, win);
}

void draw_mouse_cursor() {
    static const CursorPixel aero_cursor[CURSOR_HEIGHT][CURSOR_WIDTH] = {
        {{0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {244,249,252,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {233,242,247,255}, {247,251,252,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {222,236,243,255}, {237,244,249,255}, {249,251,253,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {210,229,238,255}, {226,236,243,255}, {237,244,249,255}, {249,251,253,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {199,222,233,255}, {216,230,239,255}, {227,237,244,255}, {237,245,249,255}, {250,253,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {188,215,229,255}, {205,224,234,255}, {216,230,239,255}, {227,237,244,255}, {237,245,249,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {178,207,224,255}, {195,218,229,255}, {205,224,234,255}, {216,230,239,255}, {227,237,244,255}, {255,255,255,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {166,198,218,255}, {183,211,225,255}, {195,218,229,255}, {205,224,234,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {155,189,213,255}, {172,203,218,255}, {0,0,0,255}, {0,0,0,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {143,180,207,255}, {161,194,213,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,170}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,255}, {255,255,255,255}, {255,255,255,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
        {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}
    };

    volatile int32_t mouse_x = (int32_t)global_mouse_x;
    volatile int32_t mouse_y = (int32_t)global_mouse_y;

    for (int y = 0; y < CURSOR_HEIGHT; y++) {
        for (int x = 0; x < CURSOR_WIDTH; x++) {
            CursorPixel pixel = aero_cursor[y][x];
            
            if (pixel.a > 0) { 
                uint8_t shadow_alpha = (pixel.a == 255) ? 0x40 : (pixel.a / 4);

                set_pixel(mouse_x + x + 1, mouse_y + y + 2, 0x00, 0x00, 0x00, shadow_alpha);
                set_pixel(mouse_x + x, mouse_y + y, pixel.r, pixel.g, pixel.b, pixel.a);

                if (y <= 9) {
                    if (x + 1 < CURSOR_WIDTH && aero_cursor[y][x + 1].a == 0) {
                        set_pixel(mouse_x + x + 1, mouse_y + y, pixel.r, pixel.g, pixel.b, pixel.a);
                    }
                }
            }
        }
    }
}

__attribute__((target("sse,sse2,mmx")))
void draw_all_windows()
{

    if (fb_back_buffer == NULL) {
        return;
    }

    draw_bmp((uint8_t*)background_file, background_size, 0, 0);

    for (size_t i = 0; i < 100; i++) {
        if (Windows[i].is_free) {
            if(Windows[i].is_visible) draw_window(&Windows[i]);
        } else {
            if(Windows[i].title != NULL) {
                kfree(Windows[i].title);
                Windows[i].title = NULL;
            }
            if(Windows[i].buffer != NULL) {
                kfree(Windows[i].buffer);
                Windows[i].buffer = NULL;
            }
        }
    }

    if(os_state == 1) {

        draw_taskbar();
        draw_start_menu();

    }

    draw_mouse_cursor();

    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH * SCREEN_HEIGHT);
}

__attribute__((target("sse,sse2,mmx")))
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

        draw_string_utf8(text_x, text_y, btn->text, 0x00, 0x00, 0x00, 0xFF);
    }
}

void draw_window_button(const window_t* win, int body_x, int body_y, const button_t* btn) {
    int abs_x = body_x + btn->rel_x;
    int abs_y = body_y + btn->rel_y;

    uint8_t border_color = 0x90; 
    draw_window_rect(win, abs_x, abs_y, btn->width, btn->height, border_color, border_color, border_color, 0xFF);

    int inner_x = abs_x + 1;
    int inner_y = abs_y + 1;
    int inner_w = btn->width - 2;
    int inner_h = btn->height - 2;
    int half_h  = inner_h / 2;

    if (btn->is_pressed) {
        draw_window_gradient_rect_vertical(win, inner_x, inner_y, inner_w, half_h, 0xA0A0A0FF, 0xBCBCBCFF);
        draw_window_gradient_rect_vertical(win, inner_x, inner_y + half_h, inner_w, inner_h - half_h, 0xB0B0B0FF, 0x909090FF);
    } 
    else if (btn->is_hovered) { 
        draw_window_gradient_rect_vertical(win, inner_x, inner_y, inner_w, half_h, 0xEAF2F8FF, 0xC4D6E6FF);
        draw_window_gradient_rect_vertical(win, inner_x, inner_y + half_h, inner_w, inner_h - half_h, 0xA3C2DBFF, 0xB8D1E5FF);
    } 
    else {
        draw_window_gradient_rect_vertical(win, inner_x, inner_y, inner_w, half_h, 0xFAFAFAFF, 0xEDEDEDFF);
        draw_window_gradient_rect_vertical(win, inner_x, inner_y + half_h, inner_w, inner_h - half_h, 0xDFDFDFFF, 0xD4D4D4FF);
    }

    if (!btn->is_pressed) {
        uint8_t highlight = 0xFF;
        draw_window_rect(win, inner_x, inner_y, inner_w, 1, highlight, highlight, highlight, 0x60);
        draw_window_rect(win, inner_x, inner_y, 1, inner_h, highlight, highlight, highlight, 0x60);
    }

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

        draw_window_text(win, btn->text, text_x, text_y, 0x20, 0x20, 0x20, 0xFF);
    }
}

void draw_window_checkbox(const window_t* win, int body_x, int body_y, const checkbox_t* cb) {
    int abs_x = body_x + cb->rel_x;
    int abs_y = body_y + cb->rel_y;
    int size  = cb->box_size > 0 ? cb->box_size : 14;

    uint8_t r_brd = cb->is_hovered ? 0x3C : 0x90;
    uint8_t g_brd = cb->is_hovered ? 0x7F : 0x90;
    uint8_t b_brd = cb->is_hovered ? 0xB1 : 0x90;
    draw_window_rect(win, abs_x, abs_y, size, size, r_brd, g_brd, b_brd, 0xFF);

    int inner_x = abs_x + 1;
    int inner_y = abs_y + 1;
    int inner_s = size - 2;
    int half_h  = inner_s / 2;

    if (cb->is_pressed) {
        draw_window_gradient_rect_vertical(win, inner_x, inner_y, inner_s, half_h, 0xA0A0A0FF, 0xBCBCBCFF);
        draw_window_gradient_rect_vertical(win, inner_x, inner_y + half_h, inner_s, inner_s - half_h, 0xB0B0B0FF, 0x909090FF);
    } 
    else if (cb->is_hovered) { 
        draw_window_gradient_rect_vertical(win, inner_x, inner_y, inner_s, half_h, 0xEAF2F8FF, 0xC4D6E6FF);
        draw_window_gradient_rect_vertical(win, inner_x, inner_y + half_h, inner_s, inner_s - half_h, 0xA3C2DBFF, 0xB8D1E5FF);
    } 
    else {
        draw_window_gradient_rect_vertical(win, inner_x, inner_y, inner_s, half_h, 0xF4F4F4FF, 0xFFFFFFFC);
        draw_window_gradient_rect_vertical(win, inner_x, inner_y + half_h, inner_s, inner_s - half_h, 0xF8F8F8FF, 0xEAEAEAFF);
    }

    if (cb->is_checked) {
        static const uint8_t check_icon[8] = {
            0x01,
            0x03,
            0x06,
            0x8C,
            0xD8,
            0x70,
            0x20,
            0x00
        };

        int icon_x = abs_x + (size - 8) / 2;
        int icon_y = abs_y + (size - 8) / 2;

        for (int cy = 0; cy < 8; cy++) {
            uint8_t row = check_icon[cy];
            for (int cx = 0; cx < 8; cx++) {
                if ((row << cx) & 0x80) {
                    draw_window_rect(win, icon_x + cx, icon_y + cy, 1, 1, 0x1A, 0x3A, 0x5A, 0xFF);
                }
            }
        }
    }

    if (cb->text) {
        int text_x = abs_x + size + 6;
        int text_y = abs_y + (size - 12) / 2;

        draw_window_text(win, cb->text, text_x, text_y, 0x20, 0x20, 0x20, 0xFF);
    }
}

int is_button_clicked(button_t* btn, int click_x, int click_y, int is_left_pressed) {
    if (click_x >= btn->rel_x && click_x < (btn->rel_x + btn->width) &&
        click_y >= btn->rel_y && click_y < (btn->rel_y + btn->height)) {
        
        btn->is_pressed = is_left_pressed;

        btn->is_hovered = 1;

        return is_left_pressed;
    }

    btn->is_pressed = 0;
    btn->is_hovered = 0;
    return 0;
}

int update_checkbox(checkbox_t* cb, int mouse_x, int mouse_y, int is_left_pressed) {
    int size = cb->box_size > 0 ? cb->box_size : 14;
    
    int text_len = 0;
    if (cb->text) {
        while (cb->text[text_len]) text_len++;
    }
    int total_width = size + (text_len > 0 ? (6 + text_len * 8) : 0);
    int total_height = size > 12 ? size : 12;

    if (mouse_x >= cb->rel_x && mouse_x < (cb->rel_x + total_width) &&
        mouse_y >= cb->rel_y && mouse_y < (cb->rel_y + total_height)) {
        
        cb->is_hovered = 1;

        if (cb->is_pressed && !is_left_pressed) {
            cb->is_pressed = 0;
            cb->is_checked = !cb->is_checked;
            return 1;
        }

        cb->is_pressed = is_left_pressed;
        return 0;
    }

    cb->is_hovered = 0;
    if (!is_left_pressed) {
        cb->is_pressed = 0;
    }
    return 0;
}

void draw_window_radiobutton(const window_t* win, int body_x, int body_y, const radiobutton_t* rb) {
    int abs_x = body_x + rb->rel_x;
    int abs_y = body_y + rb->rel_y;

    static const uint8_t radio_map[][14] = {
        {0,0,0,0,1,1,1,1,1,1,0,0,0,0},
        {0,0,1,1,2,2,2,2,2,2,1,1,0,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,1,0},
        {0,1,2,2,2,2,2,2,2,2,2,2,1,0},
        {1,2,2,2,2,4,4,4,4,2,2,2,2,1},
        {1,2,2,2,4,4,4,4,4,4,2,2,2,1},
        {1,2,2,2,4,4,4,4,4,4,2,2,2,1},
        {1,3,3,3,4,4,4,4,4,4,3,3,3,1},
        {1,3,3,3,4,4,4,4,4,4,3,3,3,1},
        {1,3,3,3,3,4,4,4,4,3,3,3,3,1},
        {0,1,3,3,3,3,3,3,3,3,3,3,1,0},
        {0,1,3,3,3,3,3,3,3,3,3,3,1,0},
        {0,0,1,1,3,3,3,3,3,3,1,1,0,0},
        {0,0,0,0,1,1,1,1,1,1,0,0,0,0}
    };

    uint8_t brd_r = 0x90, brd_g = 0x90, brd_b = 0x90;
    uint8_t top_r = 0xFA, top_g = 0xFA, top_b = 0xFA;
    uint8_t bot_r = 0xDF, bot_g = 0xDF, bot_b = 0xDF;

    if (rb->is_pressed) {
        brd_r = 0x2C; brd_g = 0x62; brd_b = 0x8B;
        top_r = 0xA0; top_g = 0xA0; top_b = 0xA0;
        bot_r = 0x90; bot_g = 0x90; bot_b = 0x90;
    } else if (rb->is_hovered) {
        brd_r = 0x3C; brd_g = 0x7F; brd_b = 0xB1;
        top_r = 0xEA; top_g = 0xF2; top_b = 0xF8;
        bot_r = 0xA3; bot_g = 0xC2; bot_b = 0xDB;
    }

    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 14; x++) {
            uint8_t pixel = radio_map[y][x];
            if (pixel == 0) continue;

            if (pixel == 4 && !rb->is_selected) {
                pixel = (y < 7) ? 2 : 3;
            }

            if (pixel == 1) {
                draw_window_rect(win, abs_x + x, abs_y + y, 1, 1, brd_r, brd_g, brd_b, 0xFF);
            } else if (pixel == 2) {
                draw_window_rect(win, abs_x + x, abs_y + y, 1, 1, top_r, top_g, top_b, 0xFF);
            } else if (pixel == 3) {
                draw_window_rect(win, abs_x + x, abs_y + y, 1, 1, bot_r, bot_g, bot_b, 0xFF);
            } else if (pixel == 4) {
                draw_window_rect(win, abs_x + x, abs_y + y, 1, 1, 0x1A, 0x4A, 0x7A, 0xFF);
            }
        }
    }

    if (rb->text) {
        int text_x = abs_x + 14 + 6;
        int text_y = abs_y + (14 - 12) / 2;
        draw_window_text(win, rb->text, text_x, text_y, 0x20, 0x20, 0x20, 0xFF);
    }
}

int update_radiobutton_group(radiobutton_t* list, int count, int mouse_x, int mouse_y, int is_left_pressed) {
    int changed_index = -1;

    for (int i = 0; i < count; i++) {
        radiobutton_t* rb = &list[i];
        
        int text_len = 0;
        if (rb->text) {
            while (rb->text[text_len]) text_len++;
        }
        int total_width = 14 + (text_len > 0 ? (6 + text_len * 8) : 0);
        int total_height = 14;

        if (mouse_x >= rb->rel_x && mouse_x < (rb->rel_x + total_width) &&
            mouse_y >= rb->rel_y && mouse_y < (rb->rel_y + total_height)) {
            
            rb->is_hovered = 1;

            if (rb->is_pressed && !is_left_pressed) {
                rb->is_pressed = 0;

                if (!rb->is_selected) {
                    for (int j = 0; j < count; j++) {
                        if (list[j].group_id == rb->group_id) {
                            list[j].is_selected = 0;
                        }
                    }
                    rb->is_selected = 1;
                    changed_index = i;
                }
            } else {
                rb->is_pressed = is_left_pressed;
            }
        } else {
            rb->is_hovered = 0;
            if (!is_left_pressed) {
                rb->is_pressed = 0;
            }
        }
    }

    return changed_index;
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
            Windows[i].is_visible = 1;
            Windows[i].current_alpha = 0x00;
            
            size_t buffer_size = width * height * sizeof(uint32_t);
            Windows[i].buffer = (uint32_t*)kmalloc(buffer_size);

            if (Windows[i].buffer == NULL) {
                Windows[i].is_free = 0;
                return -1;
            }
            
            current_new_window_id++;

            Windows[i].title = kmalloc(strlen(title)+1);
            memcpy(Windows[i].title, title, strlen(title)+1);

            memset(Windows[i].buffer, 0, buffer_size);

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

            return Windows[i].id;

        }
    }


    return -1;
}

void destroyWindowByIdx(int window_idx) {
    if (window_idx < 0 || window_idx >= 100) return;
    
    if (Windows[window_idx].is_free == 0) return;

    if (Windows[window_idx].buffer != NULL) {
        kfree(Windows[window_idx].buffer);
        Windows[window_idx].buffer = NULL;
    }

    if (Windows[window_idx].title != NULL) {
        kfree(Windows[window_idx].title);
        Windows[window_idx].title = NULL;
    }

    Windows[window_idx].is_free = 0;
    Windows[window_idx].is_visible = 0;

    if (window_in_focus == Windows[window_idx].id) {
        window_in_focus = -1;
        
        for (int i = 99; i >= 0; i--) {
            if (Windows[i].is_free == 1 && Windows[i].is_visible == 1) {
                window_in_focus = Windows[i].id;
                break;
            }
        }
    }
}

void destroyWindow(const window_t* window) {
    int current_win_idx = -1;
    for (int i = 0; i < 100; i++) {
        if (&Windows[i] == window) {
            current_win_idx = i;
            break;
        }
    }

    destroyWindowByIdx(current_win_idx);

}

void set_window_title(int window_id, const char* title) {
    for (int i = 0; i < 100; i++) {
        if (Windows[i].id == window_id) {
            kfree(Windows[i].title);
            Windows[i].title = (char*)title;
            return;
        }
    }
}

int is_mouse_inside_window(const window_t* win, int mx, int my) {
    if (win->is_free == 0) return 0;
    
    return (mx >= win->x && mx < (win->x + win->width) &&
            my >= win->y && my < (win->y + win->height));
}

window_t* window_focus_handle(int mouse_x, int mouse_y) {
    int found_index = -1;

    for (int i = (int)100 - 1; i >= 0; i--) {
        if (Windows[i].is_free == 1 && Windows[i].is_visible == 1 && is_mouse_inside_window(&Windows[i], mouse_x, mouse_y)) {
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

    uint32_t* back_buffer = fb_back_buffer;

    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    if (back_buffer == NULL) {
        back_buffer = fb_front_buffer;
    }

    size_t index = ((size_t)y * (size_t)SCREEN_WIDTH) + (size_t)x;

    if (a == 0) return;

    if (a == 255 || transparency_gui == 0) {
        back_buffer[index] = (255U << 24) | (r << 16) | (g << 8) | b;
        return;
    }

    uint32_t bg_color = back_buffer[index];

    uint8_t bg_r = (bg_color >> 16) & 0xFF;
    uint8_t bg_g = (bg_color >> 8)  & 0xFF;
    uint8_t bg_b = bg_color         & 0xFF;

    uint8_t inv_a = 255 - a; 

    uint8_t out_r = (uint8_t)(((r * a) + (bg_r * inv_a)) >> 8);
    uint8_t out_g = (uint8_t)(((g * a) + (bg_g * inv_a)) >> 8);
    uint8_t out_b = (uint8_t)(((b * a) + (bg_b * inv_a)) >> 8);

    back_buffer[index] = (255U << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

void set_window_pixel(const window_t* win, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= (win->width - 2) || y < 0 || y >= win->height) return;
    if (win->buffer == NULL) return;

    size_t index = ((size_t)y * (size_t)win->width) + (size_t)x;

    if (a == 0) return;

    if (a == 255) {
        win->buffer[index] = (255U << 24) | (r << 16) | (g << 8) | b;
        return;
    }

    uint32_t bg_color = win->buffer[index];

    uint8_t bg_r = (bg_color >> 16) & 0xFF;
    uint8_t bg_g = (bg_color >> 8)  & 0xFF;
    uint8_t bg_b = bg_color         & 0xFF;

    uint8_t inv_a = 255 - a; 

    uint8_t out_r = (uint8_t)(((r * a) + (bg_r * inv_a)) >> 8);
    uint8_t out_g = (uint8_t)(((g * a) + (bg_g * inv_a)) >> 8);
    uint8_t out_b = (uint8_t)(((b * a) + (bg_b * inv_a)) >> 8);

    win->buffer[index] = (255U << 24) | (out_r << 16) | (out_g << 8) | out_b;
}

volatile uint8_t gcc_killer = 0;

void draw_bmp(uint8_t* file_data, size_t file_size, uint32_t start_x, uint32_t start_y) {
    if (file_size < 54) return;
    if (file_data[0] != 0x42 || file_data[1] != 0x4D) return; 

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

        if (row < file_data || (row + row_stride) > (file_data + file_size)) {
            return;
        }

        for (uint32_t x = 0; x < img_w; x++) {
            uint8_t* pixel = row + (x * bytes_per_pixel);

            uint8_t b = pixel[0];
            uint8_t g = pixel[1];
            uint8_t r = pixel[2];
            uint8_t a = (bpp == 32) ? pixel[3] : 255;

            set_pixel((int32_t)(start_x + x), (int32_t)(start_y + y), r, g, b, a);
        }
    }
}

void draw_window_bmp(const window_t* win, uint8_t* file_data, size_t file_size, uint32_t start_x, uint32_t start_y) {
    if (file_size < 54) return;
    if (file_data[0] != 0x42 || file_data[1] != 0x4D) return; 

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

        if (row < file_data || (row + row_stride) > (file_data + file_size)) {
            return;
        }

        for (uint32_t x = 0; x < img_w; x++) {
            uint8_t* pixel = row + (x * bytes_per_pixel);

            uint8_t b = pixel[0];
            uint8_t g = pixel[1];
            uint8_t r = pixel[2];
            uint8_t a = (bpp == 32) ? pixel[3] : 255;

            set_window_pixel(win, (int32_t)(start_x + x), (int32_t)(start_y + y), r, g, b, a);
        }
    }
}

window_t* get_window_by_id(int id) {
    if (id < 0) return NULL;

    for (int i = 0; i < 100; i++) {
        if (Windows[i].id == id && Windows[i].is_free == 1) {
            return &Windows[i];
        }
    }
    return NULL;
}

__attribute__((target("sse,sse2,mmx")))
__attribute__((force_align_arg_pointer))
void win_printf(const window_t* win, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...) {
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
            
            char pad_char = ' ';
            if (*fmt == '0') {
                pad_char = '0';
                fmt++;
            }
            
            int width = 0;
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            int has_precision = 0;
            int precision = -1;
            if (*fmt == '.') {
                has_precision = 1;
                precision = 0;
                fmt++;
                while (*fmt >= '0' && *fmt <= '9') {
                    precision = precision * 10 + (*fmt - '0');
                    fmt++;
                }
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
                    
                    while (src[len]) {
                        if (has_precision && len >= precision) {
                            break;
                        }
                        len++;
                    }
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    int_to_str(d, temp_buf, 10, 1, 0);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'u': {
                    unsigned int u = va_arg(args, unsigned int);
                    int_to_str(u, temp_buf, 10, 0, 0);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'x': {
                    unsigned int val_x = va_arg(args, unsigned int);
                    int_to_str(val_x, temp_buf, 16, 0, 0);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'X': {
                    unsigned int val_X = va_arg(args, unsigned int);
                    int_to_str(val_X, temp_buf, 16, 0, 1);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'p': {
                    unsigned long long ptr = (unsigned long long)va_arg(args, void*);
                    
                    if (ptr == 0) {
                        char *null_ptr = "(nil)";
                        src = null_ptr;
                        while (src[len]) len++;
                    } else {
                        temp_buf[0] = '0';
                        temp_buf[1] = 'x';
                        
                        int_to_str(ptr, temp_buf + 2, 16, 0, 0);
                        
                        while (temp_buf[len]) len++;
                    }
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
                    if (pad_char == '0') *out++ = '0'; 
                    if (width > 0) {
                        // Сделать да
                    }
                    *out++ = *fmt;
                    fmt++;
                    continue; 
                }
            }

            int padding = width - len;
            
            if (left_justify) {
                pad_char = ' ';
            }

            if (!left_justify && padding > 0) {
                for (int i = 0; i < padding; i++) {
                    *out++ = pad_char;
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
                draw_window_text(win, buffer, x, y, r, g, b, alpha);
            }
            out = buffer;
            
            x = original_x;
            y += 15;
        } else {
            *out++ = *fmt;
        }
        fmt++;
    }
    
    va_end(args);
    
    *out = '\0';
    if (out != buffer) {
        draw_window_text(win, buffer, x, y, r, g, b, alpha);
    }
}

void set_window_key_callback(int window_id, void (*on_key)(const struct window* win, char key)) {
    for (int i = 0; i < 100; i++) {
        if (Windows[i].id == window_id) {
            Windows[i].on_key = on_key;
            return;
        }
    }
}

__attribute__((target("sse,sse2,mmx")))
void dwm_thread() {

    if (!stbtt_InitFont(&system_font, (unsigned char*)true_type_font_system_file, 0)) {
        KeBugCheck(0x23);
    }

    font_scale = stbtt_ScaleForPixelHeight(&system_font, 15.0f);
    init_font_metrics();

    while(1) {

        asm volatile("cli");

        spin_lock(&window_lock);

            draw_all_windows();

            if(dwm_time < timer_ticks) {

                dwm_time = ms_to_ticks(1);

                for(int i = 0; i < 100; i++) {
                    if(Windows[i].is_free == 1 && Windows[i].current_alpha != 0xFF) {
                        if(Windows[i].current_alpha == 248) {
                            Windows[i].current_alpha = 0xFF;
                            continue;
                        }
                        Windows[i].current_alpha += 8;
                    }
                }
            }

        spin_unlock(&window_lock);

        asm volatile("sti"); 
        asm volatile("hlt");
    }
}