#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH  boot_info->vbe_hor_size
#define SCREEN_HEIGHT boot_info->vbe_ver_size
#define SCREEN_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT * 4)

#define HEADER_HEIGHT 22

#define TASKBAR_HEIGHT     40
#define TASKBAR_Y          (SCREEN_HEIGHT - TASKBAR_HEIGHT)

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t g;
    uint8_t r;
} Color32;

typedef struct __attribute__((packed)) window {
    int id;
    int x, y;
    int width, height;
    char* title;
    int is_dragging;
    int drag_offset_x;
    int drag_offset_y;
    int8_t is_free;
    uint32_t* buffer;
    uint8_t is_visible;
    uint8_t current_alpha;
    
    void (*on_draw)(const struct window* win, int body_x, int body_y, int body_w, int body_h);
    void (*on_mouse)(const struct window* win, int click_x, int click_y, int is_left_pressed);
    void (*on_key)(const struct window* win, char key);
} window_t;

extern window_t Windows[100];
extern int window_in_focus;
window_t* get_window_by_id(int id);

typedef struct {
    int rel_x, rel_y;
    int width, height;
    const char* text;
    int is_pressed;
    int8_t is_hovered;
} button_t;

typedef struct {
    int rel_x, rel_y;
    int box_size;
    const char* text;
    int is_checked;
    int is_pressed;
    int8_t is_hovered;
} checkbox_t;

typedef struct {
    int rel_x, rel_y;
    int size;
    int group_id;
    char* text;
    int is_selected;
    int is_pressed;
    int8_t is_hovered;
} radiobutton_t;

extern uint32_t global_mouse_x;
extern uint32_t global_mouse_y;

#define CURSOR_WIDTH  12
#define CURSOR_HEIGHT 19

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} CursorPixel;

typedef struct __attribute__((packed)) {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPFileHeader;

typedef struct __attribute__((packed)) {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPInfoHeader;

#define BCD_TO_BIN(bcd) ((((bcd) & 0xF0) >> 4) * 10 + ((bcd) & 0x0F))

extern uint32_t* fb_front_buffer;
extern uint64_t lfb_buffer_addr;
extern uint64_t lfb_mouse_addr;

extern uint16_t vbe_hor_size;
extern uint16_t vbe_ver_size;

void set_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void set_window_pixel(const window_t* win, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void draw_window_rect(const window_t* win, int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void flip_buffers(uint32_t* front, uint32_t* back, uint32_t width, uint32_t height);

extern uint8_t transparency_gui;

void init_double_buffering();

void draw_all_windows();
int createWindow(int x, int y, int width, int height, const char* title, void (*on_draw)(const struct window* win, int body_x, int body_y, int body_w, int body_h), void (*on_mouse)(const struct window* win, int click_x, int click_y, int is_left_pressed));
void destroyWindow(const window_t* window);
void destroyWindowByIdx(int window_idx);
window_t* window_focus_handle(int mouse_x, int mouse_y);
void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void draw_button(int body_x, int body_y, const button_t* btn);
int is_button_clicked(button_t* btn, int click_x, int click_y, int is_left_pressed);
void UpdateTaskbar(int mouse_x, int mouse_y, int mouse_click);
void draw_bmp(uint8_t* file_data, size_t file_size, uint32_t start_x, uint32_t start_y);
window_t* get_window_focus(int mouse_x, int mouse_y);
void draw_gradient_rect_vertical(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color_left, uint32_t color_right);
void flip_gui();
void draw_window_button(const window_t* win, int body_x, int body_y, const button_t* btn);
void win_printf(const window_t* win, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...);
void draw_window_gradient_rect_vertical(const window_t* win, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color_top, uint32_t color_bottom);
void dwm_thread();
void draw_window_checkbox(const window_t* win, int body_x, int body_y, const checkbox_t* cb);
int update_checkbox(checkbox_t* cb, int mouse_x, int mouse_y, int is_left_pressed);
void draw_window_radiobutton(const window_t* win, int body_x, int body_y, const radiobutton_t* rb);
int update_radiobutton_group(radiobutton_t* list, int count, int mouse_x, int mouse_y, int is_left_pressed);
void draw_window_line(const window_t* win, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void set_window_key_callback(int window_id, void (*on_key)(const struct window* win, char key));
void draw_window_bmp(const window_t* win, uint8_t* file_data, size_t file_size, uint32_t start_x, uint32_t start_y);
void set_window_title(int window_id, const char* title);