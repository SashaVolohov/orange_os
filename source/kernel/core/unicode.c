#include "unicode.h"
#include "stb_truetype.h"

extern stbtt_fontinfo system_font;
extern float font_scale;
int font_baseline = 0;

static CachedGlyph glyph_cache[GLYPH_CACHE_SIZE] = {0};

__attribute__((target("sse,sse2,mmx")))
static const CachedGlyph* get_cached_glyph(uint32_t codepoint) {
    if (codepoint < GLYPH_CACHE_SIZE) {
        CachedGlyph* g = &glyph_cache[codepoint];
        
        if (!g->is_cached) {
            g->bitmap = stbtt_GetCodepointBitmap(&system_font, 0, font_scale, codepoint, &g->w, &g->h, &g->xoff, &g->yoff);
            
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&system_font, codepoint, &adv, &lsb);
            g->advance = (int)(adv * font_scale);
            
            g->is_cached = 1;
        }
        return g;
    }
    return NULL;
}

__attribute__((target("sse,sse2,mmx")))
void init_font_metrics() {
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&system_font, &ascent, &descent, &lineGap);
    font_baseline = (int)(ascent * font_scale);
}

__attribute__((target("sse,sse2,mmx")))
static uint32_t utf8_next(const char** str) {
    const uint8_t* s = (const uint8_t*)(*str);
    if (*s == 0) return 0;

    uint32_t codepoint = 0;
    if (*s < 0x80) {
        codepoint = *s;
        *str += 1;
    } else if ((*s & 0xE0) == 0xC0) {
        codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        *str += 2;
    } else if ((*s & 0xF0) == 0xE0) {
        codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        *str += 3;
    } else if ((*s & 0xF8) == 0xF0) {
        codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        *str += 4;
    } else {
        *str += 1;
        return '?';
    }
    return codepoint;
}

__attribute__((target("sse,sse2,mmx")))
void draw_string_utf8(int32_t x, int32_t y, const char* text, uint8_t r, uint8_t g, uint8_t b, uint8_t max_a) {
    if (!text) return;

    int32_t cursor_x = x;
    int32_t cursor_y = y + font_baseline;
    uint32_t prev_codepoint = 0;

    while (*text) {
        uint32_t codepoint = utf8_next(&text);
        if (codepoint == 0) break;

        if (codepoint == '\n') {
            cursor_x = x;
            cursor_y += 16;
            prev_codepoint = 0;
            continue;
        }

        if (prev_codepoint != 0) {
            int kern = stbtt_GetCodepointKernAdvance(&system_font, prev_codepoint, codepoint);
            cursor_x += (int)(kern * font_scale);
        }

        const CachedGlyph* cached = get_cached_glyph(codepoint);
        if (cached) {
            if (cached->bitmap) {
                for (int row = 0; row < cached->h; row++) {
                    for (int col = 0; col < cached->w; col++) {
                        uint8_t alpha = cached->bitmap[row * cached->w + col];
                        if (alpha > 0) {
                            if (alpha > max_a) alpha = max_a;
                            set_pixel(cursor_x + col + cached->xoff, cursor_y + row + cached->yoff, r, g, b, alpha);
                        }
                    }
                }
            }
            cursor_x += cached->advance;
        } else {
            int w, h, xoff, yoff;
            uint8_t* bitmap = stbtt_GetCodepointBitmap(&system_font, 0, font_scale, codepoint, &w, &h, &xoff, &yoff);
            if (bitmap) {
                for (int row = 0; row < h; row++) {
                    for (int col = 0; col < w; col++) {
                        uint8_t alpha = bitmap[row * w + col];
                        if (alpha > 0) {
                            if (alpha > max_a) alpha = max_a;
                            set_pixel(cursor_x + col + xoff, cursor_y + row + yoff, r, g, b, alpha);
                        }
                    }
                }
                stbtt_FreeBitmap(bitmap, NULL);
            }
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&system_font, codepoint, &adv, &lsb);
            cursor_x += (int)(adv * font_scale);
        }

        prev_codepoint = codepoint;
    }
}

__attribute__((target("sse,sse2,mmx")))
__attribute__((force_align_arg_pointer))
void draw_window_text(const window_t* win, const char *string, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!string) return;

    int32_t cursor_x = x;
    int32_t cursor_y = y + font_baseline;
    uint32_t prev_codepoint = 0;

    while (*string) {
        uint32_t codepoint = utf8_next(&string);
        if (codepoint == 0) break;

        if (codepoint == '\n') {
            cursor_x = x;
            cursor_y += 16;
            prev_codepoint = 0;
            continue;
        }

        if (prev_codepoint != 0) {
            int kern = stbtt_GetCodepointKernAdvance(&system_font, prev_codepoint, codepoint);
            cursor_x += (int)(kern * font_scale);
        }

        const CachedGlyph* cached = get_cached_glyph(codepoint);
        if (cached) {
            if (cached->bitmap) {
                for (int row = 0; row < cached->h; row++) {
                    for (int col = 0; col < cached->w; col++) {
                        uint8_t alpha = cached->bitmap[row * cached->w + col];
                        if (alpha > 0) {
                            if (alpha > a) alpha = a;
                            set_window_pixel(win, cursor_x + col + cached->xoff, cursor_y + row + cached->yoff, r, g, b, alpha);
                        }
                    }
                }
            }
            cursor_x += cached->advance;
        } else {
            int w, h, xoff, yoff;
            uint8_t* bitmap = stbtt_GetCodepointBitmap(&system_font, 0, font_scale, codepoint, &w, &h, &xoff, &yoff);
            if (bitmap) {
                for (int row = 0; row < h; row++) {
                    for (int col = 0; col < w; col++) {
                        uint8_t alpha = bitmap[row * w + col];
                        if (alpha > 0) {
                            if (alpha > a) alpha = a;
                            set_window_pixel(win, cursor_x + col + xoff, cursor_y + row + yoff, r, g, b, alpha);
                        }
                    }
                }
                stbtt_FreeBitmap(bitmap, NULL);
            }
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&system_font, codepoint, &adv, &lsb);
            cursor_x += (int)(adv * font_scale);
        }

        prev_codepoint = codepoint;
    }
}

int get_string_width(const char* str) {
    if (!str) return 0;

    int current_width = 0;
    int max_width = 0;
    uint32_t prev_codepoint = 0;

    while (*str) {
        uint32_t codepoint = utf8_next(&str);
        if (codepoint == 0) break;

        if (codepoint == '\n') {
            if (current_width > max_width) {
                max_width = current_width;
            }
            current_width = 0;
            prev_codepoint = 0;
            continue;
        }

        if (prev_codepoint != 0) {
            int kern = stbtt_GetCodepointKernAdvance(&system_font, prev_codepoint, codepoint);
            current_width += (int)(kern * font_scale);
        }

        const CachedGlyph* cached = get_cached_glyph(codepoint);
        if (cached) {
            current_width += cached->advance;
        } else {
            int adv, lsb;
            stbtt_GetCodepointHMetrics(&system_font, codepoint, &adv, &lsb);
            current_width += (int)(adv * font_scale);
        }

        prev_codepoint = codepoint;
    }

    return (current_width > max_width) ? current_width : max_width;
}

void truncate_utf8_to_width(const char* src, char* dst, size_t dst_size, int max_width) {
    if (!src || !dst || dst_size == 0) return;

    if (get_string_width(src) <= max_width) {
        size_t i = 0;
        while (src[i] && i < dst_size - 1) {
            dst[i] = src[i];
            i++;
        }
        dst[i] = '\0';
        return;
    }

    int dots_w = get_string_width("...");
    int target_w = max_width - dots_w;
    if (target_w < 0) target_w = 0;

    size_t out_idx = 0;
    const char* ptr = src;

    while (*ptr) {
        const char* prev_ptr = ptr;
        uint32_t cp = utf8_next(&ptr);
        if (cp == 0) break;

        size_t char_bytes = (size_t)(ptr - prev_ptr);
        if (out_idx + char_bytes + 4 >= dst_size) break;

        for (size_t b = 0; b < char_bytes; b++) {
            dst[out_idx + b] = prev_ptr[b];
        }
        dst[out_idx + char_bytes] = '\0';

        if (get_string_width(dst) > target_w) {
            dst[out_idx] = '\0';
            break;
        }

        out_idx += char_bytes;
    }

    if (out_idx + 3 < dst_size) {
        dst[out_idx++] = '.';
        dst[out_idx++] = '.';
        dst[out_idx++] = '.';
    }
    dst[out_idx] = '\0';
}