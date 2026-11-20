#include "core.h"

static inline double stb_fabs(double x) { return x < 0.0 ? -x : x; }

static inline int stb_ifloor(double x) {
    int i = (int)x;
    return i - (i > x);
}

static inline int stb_iceil(double x) {
    int i = (int)x;
    return i + (i < x);
}

static inline double stb_sqrt(double x) {
    double res;
    __asm__ ("sqrtsd %1, %0" : "=x"(res) : "x"(x));
    return res;
}

static inline double stb_fmod(double x, double y) {
    return x - (int)(x / y) * y;
}

static inline double stb_cos(double x) {
    x = stb_fmod(x, 6.283185307179586);
    double x2 = x * x;
    return 1.0 - x2/2.0 + (x2*x2)/24.0 - (x2*x2*x2)/720.0;
}


static inline double stb_acos(double x) { (void)x; return 0.0; }
static inline double stb_pow(double x, double y) { (void)x; (void)y; return 1.0; }

#define STBTT_malloc(x, u)  ((void)(u), kmalloc(x))
#define STBTT_free(x, u)    ((void)(u), kfree(x))
#define STBTT_assert(x)     ((void)0)

#define STBTT_memcpy        memcpy
#define STBTT_memset        memset
#define STBTT_strlen        strlen

#define STBTT_ifloor(x)     stb_ifloor(x)
#define STBTT_iceil(x)      stb_iceil(x)
#define STBTT_sqrt(x)       stb_sqrt(x)
#define STBTT_pow(x, y)     stb_pow(x, y)
#define STBTT_fmod(x, y)    stb_fmod(x, y)
#define STBTT_cos(x)        stb_cos(x)
#define STBTT_acos(x)       stb_acos(x)
#define STBTT_fabs(x)       stb_fabs(x)

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"