#include "core.h"
#include "gui.h"

window_t Windows[100];

int is_start_pressed = 0;

int is_start_menu_open = 0;
int active_menu_item = -1;
int window_in_focus = 0;
int current_new_window_id = 0;

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
    draw_rect(x, y, w, h, 0xC0, 0xC0, 0xC0, 0xFF);

    if (!pressed) {
        draw_line(x, y, x + w - 1, y, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x, y, x, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0x80, 0x80, 0x80, 0xFF);

        print_unicode(text, x + (w / 2) - 20, y + (h / 2) - 6, 0x00, 0x00, 0x00, 0xFF);
    } else {
        
        draw_line(x, y, x + w - 1, y, 0x80, 0x80, 0x80, 0xFF);
        draw_line(x, y, x, y + h - 1, 0x80, 0x80, 0x80, 0xFF);
        
        draw_line(x, y + h - 1, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
        draw_line(x + w - 1, y, x + w - 1, y + h - 1, 0xFF, 0xFF, 0xFF, 0xFF);

        print_unicode(text, x + (w / 2) - 19, y + (h / 2) - 5, 0x00, 0x00, 0x00, 0x00);
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

    print_unicode(time_str, x + 15, y + (h / 2) - 6, 0x00, 0x00, 0x00, 0x00);
}

void draw_start_menu() {
    if (!is_start_menu_open) return;

    int menu_w = 160;
    int menu_h = 120;
    int menu_x = 5;
    int menu_y = TASKBAR_Y - menu_h;

    draw_rect(menu_x, menu_y, menu_w, menu_h, 0xC0, 0xC0, 0xC0, 0xFF);

    draw_line(menu_x, menu_y, menu_x + menu_w - 1, menu_y, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(menu_x, menu_y, menu_x, menu_y + menu_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
    draw_line(menu_x, menu_y + menu_h - 1, menu_x + menu_w - 1, menu_y + menu_h - 1, 0x80, 0x80, 0x80, 0xFF);
    draw_line(menu_x + menu_w - 1, menu_y, menu_x + menu_w - 1, menu_y + menu_h - 1, 0x80, 0x80, 0x80, 0xFF);

    draw_rect(menu_x + 2, menu_y + 2, 20, menu_h - 4, 0x00, 0x00, 0x80, 0xFF); 

    const char* menu_items[] = { "Device Manager", "About OS", "Shut Down" };
    int item_count = 3;
    int item_start_x = menu_x + 26;

    for (int i = 0; i < item_count; i++) {
        int item_y = menu_y + 4 + (i * 36);
        int item_w = menu_w - 30;
        int item_h = 30;

        if (i == active_menu_item) {
            draw_rect(item_start_x, item_y, item_w, item_h, 0x00, 0x00, 0x80, 0xFF);
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
    draw_rect(0, TASKBAR_Y, SCREEN_WIDTH, TASKBAR_HEIGHT, 0xC0, 0xC0, 0xC0, 0xFF);

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
            
            draw_rect(current_btn_x, btn_y, btn_w, btn_h, 0xC0, 0xC0, 0xC0, 0xFF);
            
            if (!is_active) {
                draw_line(current_btn_x, btn_y, current_btn_x + btn_w - 1, btn_y, 0xFF, 0xFF, 0xFF, 0xFF);
                draw_line(current_btn_x, btn_y, current_btn_x, btn_y + btn_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
                draw_line(current_btn_x, btn_y + btn_h - 1, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0x80, 0x80, 0x80, 0xFF);
                draw_line(current_btn_x + btn_w - 1, btn_y, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0x80, 0x80, 0x80, 0xFF);

                print_unicode(Windows[i].title, current_btn_x + 8, btn_y + 9, 0x00, 0x00, 0x00, 0xFF);
            } else {
                draw_line(current_btn_x, btn_y, current_btn_x + btn_w - 1, btn_y, 0x80, 0x80, 0x80, 0xFF);
                draw_line(current_btn_x, btn_y, current_btn_x, btn_y + btn_h - 1, 0x80, 0x80, 0x80, 0xFF);
                draw_line(current_btn_x, btn_y + btn_h - 1, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);
                draw_line(current_btn_x + btn_w - 1, btn_y, current_btn_x + btn_w - 1, btn_y + btn_h - 1, 0xFF, 0xFF, 0xFF, 0xFF);

                print_unicode(Windows[i].title, current_btn_x + 9, btn_y + 10, 0x00, 0x00, 0x00, 0xFF);
            }
            
            current_btn_x += (btn_w + gap);
        }
    }
}

void draw_window(const window_t* win) {
    draw_rect(win->x, win->y, win->width, win->height, 0x40, 0x40, 0x40, 0xFF);

    draw_rect(win->x + 2, win->y + 2, win->width - 4, win->height - 4, 0xC0, 0xC0, 0xC0, 0xFF);

    draw_rect(win->x + 2, win->y + 2, win->width - 4, 22, 0x00, 0x50, 0xA0, 0xFF);

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

    draw_rect(0, 0, 800, 600, 0, 148, 255, 0xFF);

    for (size_t i = 0; i < 100; i++) {
        if (Windows[i].is_free) {
            draw_window(&Windows[i]);
        }
    }

    draw_taskbar();
    draw_start_menu();
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