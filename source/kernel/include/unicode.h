#include <stdint.h>

#include "gui.h"

#define GLYPH_CACHE_SIZE 0x0500

typedef struct {
    uint8_t* bitmap;
    int w;
    int h;
    int xoff;
    int yoff;
    int advance;
    uint8_t is_cached;
} CachedGlyph;

void draw_window_text(const window_t* win, const char *string, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void init_font_metrics();
void draw_string_utf8(int32_t x, int32_t y, const char* text, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
int get_string_width(const char* str);
void truncate_utf8_to_width(const char* src, char* dst, size_t dst_size, int max_width);