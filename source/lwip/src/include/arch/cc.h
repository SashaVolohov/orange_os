#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include <stdint.h>
#include <stddef.h>

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "zu"

extern void lwip_diag_print(const char *fmt, ...);
extern void lwip_assert_fail(const char *msg);

extern void kprintf(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...);
#define LWIP_PLATFORM_DIAG(x)   do { lwip_diag_print x; } while(0)
#define LWIP_PLATFORM_ASSERT(x) do { lwip_assert_fail(x); } while(0)

#define LWIP_NO_STDDEF_H 0
#define LWIP_NO_STDINT_H 0
#define LWIP_NO_INTTYPES_H 1
#define LWIP_NO_LIMITS_H 1
#define LWIP_NO_CTYPE_H 1

typedef uint64_t sys_prot_t;

sys_prot_t sys_arch_protect(void);
void sys_arch_unprotect(sys_prot_t pval);

#define SSIZE_MAX __LONG_MAX__

#define LWIP_NO_STDIO_H 1
#define LWIP_NO_STDLIB_H 1
#define LWIP_NO_STRING_H 0

long int k_strtol(const char *nptr, char **endptr, int base);
#define strtol(nptr, endptr, base) k_strtol(nptr, endptr, base)

uint32_t lwip_port_rand(void);
#define LWIP_RAND() lwip_port_rand()

#endif