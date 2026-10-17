#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT * 4)

#define HEADER_HEIGHT 22

#define TASKBAR_HEIGHT     40
#define TASKBAR_Y          (SCREEN_HEIGHT - TASKBAR_HEIGHT)

typedef struct window {
    int id;
    int x, y;
    int width, height;
    const char* title;
    int is_dragging;
    int drag_offset_x;
    int drag_offset_y;
    int8_t is_free;
    
    void (*on_draw)(const struct window* win, int body_x, int body_y, int body_w, int body_h);
    void (*on_mouse)(const struct window* win, int click_x, int click_y, int is_left_pressed);
} window_t;

typedef struct {
    int rel_x, rel_y;
    int width, height;
    const char* text;
    int is_pressed;
} button_t;

#define BCD_TO_BIN(bcd) ((((bcd) & 0xF0) >> 4) * 10 + ((bcd) & 0x0F))

extern uint32_t* fb_front_buffer;
extern uint32_t lfb_buffer_addr;

void set_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
extern void print_unicode(const char* str, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
extern void flip_buffers(uint32_t* dest_front, uint32_t* src_back, uint64_t dwords_count);

void init_double_buffering();

void draw_all_windows();
int createWindow(int x, int y, int width, int height, const char* title, void (*on_draw)(const struct window* win, int body_x, int body_y, int body_w, int body_h), void (*on_mouse)(const struct window* win, int click_x, int click_y, int is_left_pressed));
window_t* window_focus_handle(int mouse_x, int mouse_y);
void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void draw_button(int body_x, int body_y, const button_t* btn);
int is_button_clicked(button_t* btn, int click_x, int click_y, int is_left_pressed);
void UpdateTaskbar(int mouse_x, int mouse_y, int mouse_click);