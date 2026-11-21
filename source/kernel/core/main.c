#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "version.h"

#include "ata.h"
#include "ahci.h"
#include "core.h"
#include "disk.h"
#include "pci.h"
#include "mouse.h"
#include "nvme.h"
#include "xhci.h"
#include "os_memory.h"
#include "setup.h"
#include "smbios.h"
#include "cdfs.h"
#include "https.h"
#include "elf.h"
#include "hdaudio.h"

#include "Am79C973.h"

#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "netif/ethernet.h"

int net_status = 0;

struct system_info g_sys_info = {0}; 

int8_t ap_running_flag = 0;
uint8_t bsp_apic_id;

uint64_t internet_packages_sended = 0;
uint64_t internet_packages_received = 0;

am79c973_device_t am73c973_device;
ACPI_STATUS acpi_status;

uint8_t *device_mac;
uint8_t *device_ip;

uint64_t core_tss_stacks[256];
tss64_t tss_per_cpu[256];

deferred_task_t task_queue[TASK_QUEUE_SIZE];
volatile int task_queue_head = 0;
volatile int task_queue_tail = 0;

extern uint32_t* fb_back_buffer;
extern Drive drives[];

uint8_t os_state = 0;
uintptr_t background_file;
uint64_t background_size;

uintptr_t true_type_font_system_file;
uint64_t true_type_font_system_size;

extern char _bss_start;
extern char _bss_end;

int64_t dwm_pid = -1;
int64_t cache_pid = -1;

static const char *fs_types_mbr[] = {
    "free",
    "FAT12",
    "XENIX root",
    "XENIX user",
    "FAT16",
    "Extended partition",
    "FAT16",
    "NTFS",
    "FAT16",
    "AIX data/boot",
    "OS/2 Boot Manager",
    "FAT32",
    "FAT32",
    "Undefined FS",
    "FAT16",
    "Extended partition",

    "Undefined FS",
    "FAT16",
    "Configuration partition",
    "Undefined FS",
    "FAT16",
    "Extended partition",
    "FAT16",
    "NTFS",
    "AST Zero Volt",
    "Willowtech Photon coS",
    "Undefined FS",
    "FAT32",
    "FAT32",
    "Undefined FS",
    "FAT16",
    "Extended partition",

    "Windows Mobile update XIP",
    "FSo2",
    "Oxygen Extended Partition Table",
    "Windows Mobile boot XIP",
    "FAT16",
    "Undefined FS",
    "Undefined FS",
    "Windows Recovery Environment",
    "Undefined FS",
    "Undefined FS",
    "AtheOS file system",
    "SyllableSecure",
    "Undefined FS",
    "Undefined FS",
    "Undefined FS",
    "Undefined FS"
};

static IconPixel icon_cpu[16][16] = {
    {{0,0,0,0}},
    {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {0,0,0,0}}, // Золотые ножки (R,G,B,A)
    {{0,0,0,0}, {0,0,0,0}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {0,0,0,0}},
    {{0,0,0,0}, {211,84,0,255}, {80,80,80,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}, {211,84,0,255}},
    {{0,0,0,0}, {0,0,0,0}, {80,80,80,255}, {240,240,240,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}},
    {{0,0,0,0}, {211,84,0,255}, {80,80,80,255}, {240,240,240,255}, {180,180,180,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {180,180,180,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}, {211,84,0,255}},
    {{0,0,0,0}, {0,0,0,0}, {80,80,80,255}, {240,240,240,255}, {180,180,180,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {180,180,180,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}},
    {{0,0,0,0}, {211,84,0,255}, {80,80,80,255}, {240,240,240,255}, {180,180,180,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {180,180,180,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}, {211,84,0,255}},
    {{0,0,0,0}, {0,0,0,0}, {80,80,80,255}, {240,240,240,255}, {180,180,180,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {140,140,140,255}, {180,180,180,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}},
    {{0,0,0,0}, {211,84,0,255}, {80,80,80,255}, {240,240,240,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {180,180,180,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}, {211,84,0,255}},
    {{0,0,0,0}, {0,0,0,0}, {80,80,80,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {240,240,240,255}, {80,80,80,255}},
    {{0,0,0,0}, {211,84,0,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {211,84,0,255}},
    {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {211,84,0,255}, {0,0,0,0}, {0,0,0,0}}
};

static IconPixel icon_pci[16][16] = {
    {{120,120,120,255}, {0,0,0,0}},
    {{120,120,120,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {40,160,80,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {40,160,80,255}, {0,0,0,255}, {0,0,0,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {40,160,80,255}, {0,0,0,255}, {0,0,0,255}, {0,0,0,255}, {40,160,80,255}, {0,0,0,255}, {0,0,0,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {40,160,80,255}, {40,160,80,255}, {211,84,0,255}, {211,84,0,255}, {211,84,0,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {40,160,80,255}, {40,160,80,255}, {211,84,0,255}, {211,84,0,255}, {211,84,0,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {40,160,80,255}, {120,120,120,255}},
    {{120,120,120,255}, {0,0,0,0}, {0,0,0,0}, {241,196,15,255}, {241,196,15,255}, {241,196,15,255}, {241,196,15,255}, {241,196,15,255}, {241,196,15,255}, {241,196,15,255}, {241,196,15,255}, {0,0,0,0}},
    {{120,120,120,255}}
};

static IconPixel icon_display[16][16] = {
    {{0,0,0,0}},
    {{0,0,0,0}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {0,0,0,0}},
    {{0,0,0,0}, {40,40,40,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {255,255,255,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {100,180,220,255}, {255,255,255,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {255,255,255,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {100,180,220,255}, {255,255,255,255}, {255,255,255,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {255,255,255,255}, {100,180,220,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {100,180,220,255}, {255,255,255,255}, {255,255,255,255}, {40,40,40,255}},
    {{0,0,0,0}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {40,40,40,255}, {0,0,0,0}},
    {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {120,120,120,255}, {120,120,120,255}, {120,120,120,255}, {120,120,120,255}, {120,120,120,255}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}},
    {{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {80,80,80,255}, {0,0,0,0}, {0,0,0,0}},
    {{0,0,0,0}}
};

#define FS_TYPES_COUNT (sizeof(fs_types_mbr) / sizeof(fs_types_mbr[0]))

u32_t sys_now() {
    return (u32_t)cmos_ticks;
}

uint32_t lwip_port_rand(void) {
    static uint32_t seed = 0;

    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

    if (seed == 0) {
        seed = lo ^ sys_now() ^ 0x9E3779B9U;
    }

    seed ^= lo;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;

    return seed;
}

sys_prot_t sys_arch_protect(void) {
    uint64_t rflags;
    asm volatile(
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=r"(rflags)
        :
        : "memory"
    );
    return rflags;
}

void sys_arch_unprotect(sys_prot_t pval) {
    if (pval & (1ULL << 9)) {
        asm volatile("sti" ::: "memory");
    }
}

void lwip_diag_print(const char *fmt, ...) {
    kprintf(10, 500, 255, 255, 255, 255, "%s", fmt);
}

void lwip_assert_fail(const char *msg) {
    kprintf(10, 520, 255, 50, 50, 255, "LWIP ASSERT: %s", msg);
    for (;;) {
        asm volatile("cli; hlt");
    }
}

long int k_strtol(const char *nptr, char **endptr, int base) {
    long int res = 0;
    int sign = 1;
    while (*nptr == ' ' || *nptr == '\t') nptr++;
    if (*nptr == '-') { sign = -1; nptr++; }
    else if (*nptr == '+') { nptr++; }

    while (*nptr >= '0' && *nptr <= '9') {
        res = res * (base ? base : 10) + (*nptr - '0');
        nptr++;
    }
    if (endptr) *endptr = (char *)nptr;
    return res * sign;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    if (d == s || n == 0) {
        return dest;
    }

    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

long int __isoc23_strtol(const char *nptr, char **endptr, int base) {
    return k_strtol(nptr, endptr, base);
}

uint8_t get_lapic_id(void) {
    uint32_t raw_id = *(volatile uint32_t *)(lapic_base_addr + LAPIC_ID_OFFSET);
    return (uint8_t)(raw_id >> 24);
}

uint32_t hash_fnv1a(const uint8_t* data, size_t size) {
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 16777619U;
    }
    return hash;
}

uint64_t get_phys_from_pml4(uint64_t *pml4, uint64_t virt_addr) {
    uint64_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & PAGE_PRESENT)) return 0;
    uint64_t *pdpt = (uint64_t *)(pml4[pml4_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) return 0;
    uint64_t *pd = (uint64_t *)(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_idx] & PAGE_PRESENT)) return 0;
    uint64_t *pt = (uint64_t *)(pd[pd_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pt[pt_idx] & PAGE_PRESENT)) return 0;
    return pt[pt_idx] & 0x000FFFFFFFFFF000ULL;
}

Partition* find_boot_partition(boot_info_t* boot_info) {

    uint8_t buf[2048];

    for (int i = 0; i < get_partition_count(); i++) {
        Partition* p = get_partition(i);
        Drive* d = get_disk(p->drive_index);

        size_t sector_size = (d->atapi == 1) ? 2048 : 100;
        uint64_t target_lba = (d->atapi == 1) ? 16 : p->start_lba;

        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

        disk_read(p->drive_index, target_lba, 1, (uint64_t)buf);

        if (hash_fnv1a(buf, sector_size) == boot_info->boot_pvd_hash) {
            return p;
        }

    }

    return NULL;
}

void KeBugCheck(int stopCode) {
    if(stopCode == 0x20) {
        KeBugMemoryManagement();
    } else if(stopCode == 0x21) {
        KeBugACPIError();
    } else if(stopCode == 0x22) {
        KeBugInaccesibleBootDevice();
    } else if(stopCode == 0x23) {
        KeBugCriticalProcessDied();
    }
}

uint32_t swap_uint32(uint32_t val) {
    return ((val >> 24) & 0x000000FF) |
           ((val >> 8)  & 0x0000FF00) |
           ((val << 8)  & 0x00FF0000) |
           ((val << 24) & 0xFF000000);
}

uint64_t ms_to_ticks(uint64_t milliseconds) {
    return milliseconds / 5;
}

void cache_flush(void* addr, uint32_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = start + size;

    start = start & ~(uintptr_t)63;

    for (uintptr_t ptr = start; ptr < end; ptr += 64) {
        asm volatile("clflush (%0)" : : "r"(ptr) : "memory");
    }

    asm volatile("mfence" : : : "memory");
}

void get_cpu_brand_string(char* buffer) {
    uint32_t regs[4];
    
    cpu_get_cpuid(0x80000000, regs);
    if (regs[0] < 0x80000004) {
        strcpy(buffer, "Unknown x86_64 CPU");
        return;
    }

    char raw_buffer[49];
    char* ptr = raw_buffer;

    for (uint32_t code = 0x80000002; code <= 0x80000004; code++) {
        cpu_get_cpuid(code, regs);
        memcpy(ptr, regs, 16);
        ptr += 16;
    }
    raw_buffer[48] = '\0';

    char* src = raw_buffer;
    char* dst = buffer;
    int is_space = 0;

    while (*src && (*src == ' ')) {
        src++;
    }

    while (*src) {
        if (*src == ' ') {
            if (!is_space) {
                *dst++ = ' ';
                is_space = 1;
            }
        } else {
            *dst++ = *src;
            is_space = 0;
        }
        src++;
    }
    *dst = '\0';

    if (dst > buffer && (*dst-1 == ' ')) {
        *(dst - 1) = '\0';
    }

}

void int_to_str(long long value, char *buf, int base, int is_signed, int uppercase) {
    char temp[64];
    int i = 0;
    
    if (is_signed && base == 10 && value < 0) {
        *buf++ = '-';
        value = -value;
    }
    
    unsigned long long uvalue = (unsigned long long)value;
    
    if (uvalue == 0) {
        temp[i++] = '0';
    } else {
        const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
        while (uvalue > 0) {
            temp[i++] = digits[uvalue % base];
            uvalue /= base;
        }
    }
    
    while (i > 0) {
        *buf++ = temp[--i];
    }
    *buf = '\0';
}

uint64_t str_to_int(const char* str) {
    uint64_t result = 0;

    while (*str == ' ' || *str == '\t') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        result = (result * 10) + (*str - '0');
        str++;
    }

    return result;
}

void kprintf(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...) {
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
                    // Здесь выводим накопленную ширину, если формат не распознан
                    if (width > 0) {
                        // Для простоты выводим обратно символ (или можно разложить число, если нужно)
                        // Но в простейшем случае просто продолжаем
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
                print_text(buffer, x, y, r, g, b, alpha);
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
        print_text(buffer, x, y, r, g, b, alpha);
    }
}

static inline void put_buf_char(char *buf, size_t max_len, size_t *pos, char c) {
    if (max_len > 0 && *pos < max_len - 1) {
        buf[*pos] = c;
    }
    (*pos)++;
}

#undef strchr
char *strchr(const char *s, int c) {
    while (*s != (char)c) {
        if (!*s++) {
            return NULL;
        }
    }
    return (char *)s;
}

int kvsnprintf(char *buffer, size_t size, const char *fmt, va_list args) {
    size_t pos = 0;

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
                        if (has_precision && len >= precision) break;
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
                case 'p': {
                    unsigned long long ptr = (unsigned long long)va_arg(args, void*);
                    if (ptr == 0) {
                        src = "(nil)";
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
                    put_buf_char(buffer, size, &pos, '%');
                    if (left_justify) put_buf_char(buffer, size, &pos, '-');
                    if (pad_char == '0') put_buf_char(buffer, size, &pos, '0');
                    put_buf_char(buffer, size, &pos, *fmt);
                    fmt++;
                    continue;
                }
            }

            int padding = width - len;
            if (left_justify) pad_char = ' ';

            if (!left_justify && padding > 0) {
                for (int i = 0; i < padding; i++) {
                    put_buf_char(buffer, size, &pos, pad_char);
                }
            }

            for (int i = 0; i < len; i++) {
                put_buf_char(buffer, size, &pos, src[i]);
            }

            if (left_justify && padding > 0) {
                for (int i = 0; i < padding; i++) {
                    put_buf_char(buffer, size, &pos, ' ');
                }
            }
        } else {
            put_buf_char(buffer, size, &pos, *fmt);
        }
        fmt++;
    }

    if (size > 0) {
        if (pos < size) {
            buffer[pos] = '\0';
        } else {
            buffer[size - 1] = '\0';
        }
    }

    return (int)pos;
}

int ksnprintf(char *buffer, size_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = kvsnprintf(buffer, size, fmt, args);
    va_end(args);
    return ret;
}

int ksprintf(char *buffer, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = kvsnprintf(buffer, (size_t)-1, fmt, args);
    va_end(args);
    return ret;
}

char device_class_name[20][40] = {
    "Undefined Device",
    "Mass Storage Controller",
    "Network Controller",
    "Display Controller",
    "Multimedia Controller",
    "Memory Controller",
    "Bridge Device",
    "Simple Communication Controller",
    "Base System Peripheral",
    "Input Device Controller",
    "Docking Station",
    "Processor",
    "Serial Bus Controller",
    "Wireless Controller",
    "Intelligent I/O Controller",
    "Satellite Communication Controller",
    "Encryption Controller",
    "Signal Processing Controller",
    "Processing Accelerator",
    "Non-Essential Instrumentation"
};

void render_device_icon(const window_t* win, int screen_x, int screen_y, const IconPixel icon[16][16]) {
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            IconPixel pixel = icon[y][x];
            if (pixel.a > 0) {
                set_window_pixel(win, screen_x + x, screen_y + y, pixel.r, pixel.g, pixel.b, pixel.a);
            }
        }
    }
}

void draw_device_manager_content(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    win_printf(win, 10, 10, 0x00, 0x00, 0x00, 0xFF, "%d devices found on this computer.", total_devices);
    
    render_device_icon(win, 10, 30, icon_cpu);

    uint64_t hz = g_sys_info.cpu.frequency_hz;
    uint64_t mhz = hz / 1000000;
    uint64_t khz_remainder = (hz % 1000000) / 1000;

    win_printf(win, 32, 30, 0x20, 0x20, 0x20, 0xFF, "CPU: %s(%d.%03d MHz), %d logical processors", g_sys_info.cpu.brand_string, mhz, khz_remainder, boot_info->cpu_count);
    
    win_printf(win, 10, 50, 0x20, 0x20, 0x20, 0xFF, "- Physical memory available for OrangeOS: %d MB(%d MHz)", mb_memory, g_sys_info.ram_slots[0].speed_mhz);
    
    int current_y = 75;

    for (uint64_t i = 0; i < total_devices; i++) {
        int class_code = pci_devices[i].class_code;
        if (class_code >= 19) class_code = 0;
        IconPixel (*selected_icon)[16];

        if(class_code == 0x03) {
            selected_icon = icon_display;
        } else {
            selected_icon = icon_pci;
        }

        render_device_icon(win, 10, current_y, selected_icon);

        win_printf(win, 32, current_y + 2, 0x20, 0x20, 0x20, 0xFF, 
                   "%s - PCI: Bus %d, Slot %d, Func %d -> Vendor: 0x%x, Device: 0x%x", 
                   device_class_name[class_code], 
                   pci_devices[i].bus, 
                   pci_devices[i].device, 
                   pci_devices[i].function, 
                   pci_devices[i].vendor_id, 
                   pci_devices[i].device_id);

        current_y += 20;
    }
}

static button_t btn_ok   = { .rel_x = 20,  .rel_y = 60, .width = 80, .height = 24, .text = "OK",   .is_pressed = 0 };

void draw_about_os_content(const window_t* win, int bx, int by, int bw, int bh) {

    (void)bx;
    (void)by;
    (void)bw;
    (void)bh;

    draw_window_rect(win, 0, 0, win->width, win->height, 0xFF, 0xFF, 0xFF, 0xFF);

    win_printf(win, 20, 20, 0x80, 0x00, 0x00, 0xFF, "OrangeOS Alpha 1.4 (Build %d, %s)", ORANGEOS_BUILD, ORANGEOS_BUILD_DATE);
    draw_window_text(win, "Copyright(C) Alexander Volokhov, 2023-2026", 20, 35, 0x00, 0x00, 0x00, 0xFF);

    draw_window_button(win, 0, 0, &btn_ok);

}

void mouse_about_os_content(const window_t* win, int click_x, int click_y, int is_left_pressed) {
    if (is_button_clicked(&btn_ok, click_x, click_y, is_left_pressed)) {
        btn_ok.is_pressed = 0;
        ((window_t*)win)->is_free = 0;
    }
}

void openDeviceManager() {
    createWindow(300, 200, 550, 500, "Device Manager", draw_device_manager_content, NULL);
}

void openAbout() {
    createWindow(300, 200, 300, 150, "About OrangeOS", draw_about_os_content, mouse_about_os_content);
}

void lapic_wait_delivery() {
    while (*LAPIC_ICR_LOW & (1 << 12)) {
        asm volatile("pause");
    }
}

void wakeup_ap_core(uint8_t target_apic_id, uint8_t vector) {
    *LAPIC_ICR_HIGH = (uint32_t)target_apic_id << 24;
    *LAPIC_ICR_LOW  = 0x00004500;
    
    lapic_wait_delivery();
    
    sleep_ms(10);
    
    *LAPIC_ICR_HIGH = (uint32_t)target_apic_id << 24;
    *LAPIC_ICR_LOW  = 0x00004600 | vector;
    
    lapic_wait_delivery();
    sleep_ms(200);
    
    *LAPIC_ICR_HIGH = (uint32_t)target_apic_id << 24;
    *LAPIC_ICR_LOW  = 0x00004600 | vector;
    
    lapic_wait_delivery();
}

void idt_set_tss_descriptor(uint8_t apic_id, uint64_t tss_address) {
    uint8_t* gdt_ptr = (uint8_t*)&gdt_tss_slots[apic_id];
    
    uint32_t limit = sizeof(tss64_t) - 1;

    memset(gdt_ptr, 0, 16);
    gdt_ptr[0] = limit & 0xFF;
    gdt_ptr[1] = (limit >> 8) & 0xFF;
    
    gdt_ptr[2] = tss_address & 0xFF;
    gdt_ptr[3] = (tss_address >> 8) & 0xFF;
    gdt_ptr[4] = (tss_address >> 16) & 0xFF;

    gdt_ptr[5] = 0x89;
    gdt_ptr[6] = (limit >> 16) & 0x0F;
    gdt_ptr[7] = (tss_address >> 24) & 0xFF;

    uint32_t base_high = (tss_address >> 32) & 0xFFFFFFFF;
    *(uint32_t*)&gdt_ptr[8] = base_high;
    *(uint32_t*)&gdt_ptr[12] = 0;
}

uint16_t get_tss_selector(uint8_t apic_id) {
    return 0x28 + ((uint16_t)apic_id * 0x10);
}

void initialize_core_tss(uint8_t apic_id, uint64_t kernel_stack_top) {
    memset(&tss_per_cpu[apic_id], 0, sizeof(tss64_t));
    
    tss_per_cpu[apic_id].rsp0 = kernel_stack_top;
    tss_per_cpu[apic_id].ist[0] = kernel_stack_top;
    tss_per_cpu[apic_id].ist[1] = kernel_stack_top;
    tss_per_cpu[apic_id].io_map_base = sizeof(tss64_t);

    idt_set_tss_descriptor(apic_id, (uint64_t)&tss_per_cpu[apic_id]);

    uint16_t tss_selector = get_tss_selector(apic_id);

    if(apic_id == bsp_apic_id) {

        asm volatile(
            "pushq $0x08\n\t"
            "leaq 1f(%%rip), %%rax\n\t"
            "pushq %%rax\n\t"
            
            "lgdt %0\n\t"
            
            "lretq\n\t"
            "1:\n\t"                       
            
            "mov $0x10, %%ax\n\t"
            "mov %%ax, %%ds\n\t"
            "mov %%ax, %%es\n\t"
            "mov %%ax, %%ss\n\t"
            "xor %%ax, %%ax\n\t"
            "mov %%ax, %%fs\n\t"
            "mov %%ax, %%gs\n\t"
            : 
            : "m"(GDTR_64)
            : "rax", "memory"
        );

    }

    asm volatile("ltr %0" :: "r"(tss_selector));
}

void SystemIdleProcess() {
    while(1) {
        asm volatile("hlt");
    }
}

void init_logical_processors() {

    bsp_apic_id = get_lapic_id();
    initialize_core_tss(bsp_apic_id, boot_info->bsp_factory_ist1);

    for(uint32_t i = 0; i < boot_info->cpu_count; i++) {

        if(boot_info->cpus_ids[i] == get_lapic_id()) {
            continue;
        }

        ap_target_cr3 = pt_begin;
        ap_stack_top = alloc_page() + 4096;

        if(ap_stack_top <= 4096) {
            KeBugCheck(0x20);
        }

        core_tss_stacks[boot_info->cpus_ids[i]] = alloc_page() + 4096;
        if(core_tss_stacks[boot_info->cpus_ids[i]] <= 4096) {
            KeBugCheck(0x20);
        }

        ap_running_flag = 0;

        size_t trampoline_size = (char*)&trampoline_end - (char*)&trampoline_start;
        memcpy((void*)0x8000, &trampoline_start, trampoline_size);

        //create_task(SystemIdleProcess); // For new CPU

        wakeup_ap_core(boot_info->cpus_ids[i], 0x08);

        while(ap_running_flag == 0) {
            asm volatile("hlt");
        }

    }

}

void LoadBackground(int disk_id) {
    const char* background_file_name = "BACK_S.BMP";
    
    uint32_t file_lba = 0;
    uint32_t file_size = 0;
    
    int res = cdfs_get_file_info(disk_id, background_file_name, &file_lba, &file_size);
    background_file = (uintptr_t)kmalloc(file_size + 2048); 

    if (res <= 0 || background_file == 0) {
        asm volatile("cli");
        kprintf(0, 0, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS could not start because the following file is missing or corrupt:\n%s\nPlease re-install a copy of the above file.", background_file_name);
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }
    
    int status = cdfs_read_file_data(disk_id, file_lba, file_size, (uint64_t)background_file);

    if(status != 0) {
        asm volatile("cli");
        kprintf(0, 0, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS could not start because the following file is missing or corrupt:\n%s\nPlease re-install a copy of the above file.", background_file_name);
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }

    background_size = file_size;
}

void LoadFont(int disk_id) {
    const char* font_file_name = "INTER.TTF";
    
    uint32_t file_lba = 0;
    uint32_t file_size = 0;
    
    int res = cdfs_get_file_info(disk_id, font_file_name, &file_lba, &file_size);
    true_type_font_system_file = (uintptr_t)kmalloc(file_size + 2048); 

    if (res <= 0 || true_type_font_system_file == 0) {
        asm volatile("cli");
        kprintf(0, 0, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS could not start because the following file is missing or corrupt:\n%s\nPlease re-install a copy of the above file.", font_file_name);
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }
    
    int status = cdfs_read_file_data(disk_id, file_lba, file_size, (uint64_t)true_type_font_system_file);

    if(status != 0) {
        asm volatile("cli");
        kprintf(0, 0, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS could not start because the following file is missing or corrupt:\n%s\nPlease re-install a copy of the above file.", font_file_name);
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }

    true_type_font_system_size = file_size;

}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline uint16_t pit_read_current(void) {
    outb(0x43, 0x00);
    uint8_t low = inb(0x40);
    uint8_t high = inb(0x40);
    return (uint16_t)(low | (high << 8));
}

uint64_t calibrate_by_bogomips() {
    outb(0x43, 0x30);
    
    outb(0x40, 0xFF);
    outb(0x40, 0xFF);

    uint16_t pit_start = pit_read_current();

    uint64_t test_loops = 20000000; 
    
    asm volatile("lfence" ::: "memory");
    delay_loop(test_loops);
    asm volatile("lfence" ::: "memory");

    uint16_t pit_end = pit_read_current();

    uint32_t pit_ticks_passed = 0;
    if (pit_end <= pit_start) {
        pit_ticks_passed = pit_start - pit_end;
    } else {
        pit_ticks_passed = pit_start + (0xFFFF - pit_end);
    }

    if (pit_ticks_passed == 0) pit_ticks_passed = 1;

    uint64_t loops_per_second = (test_loops * 1193182) / pit_ticks_passed;

    uint64_t cpu_hz = loops_per_second;

    return cpu_hz;
}

uint64_t estimate_cpu_frequency(void) {
    outb(0x43, 0x30);
    
    uint16_t pit_start_val = 20000;
    outb(0x40, (uint8_t)(pit_start_val & 0xFF));
    outb(0x40, (uint8_t)((pit_start_val >> 8) & 0xFF));

    uint16_t pit_start = pit_read_current();
    uint64_t tsc_start = rdtsc();

    for (volatile int i = 0; i < 30000; i++) {
        asm volatile("pause");
        outb(0x80, 0);
    }

    asm volatile("lfence" ::: "memory");

    uint64_t tsc_end = rdtsc();
    uint16_t pit_end = pit_read_current();

    uint32_t pit_ticks_passed = 0;
    if (pit_end <= pit_start) {
        pit_ticks_passed = pit_start - pit_end;
    } else {
        pit_ticks_passed = pit_start + (0xFFFF - pit_end);
    }

    if (pit_ticks_passed == 0) pit_ticks_passed = 1;
    uint64_t tsc_ticks_passed = tsc_end - tsc_start;

    uint64_t cpu_hz = (tsc_ticks_passed * 1193182) / pit_ticks_passed;

    return cpu_hz;
}

static inline void io_delay(void) {
    outb(0x80, 0);
}

void calibrate_lapic_timer() {
    if (!lapic_base_addr) return;

    volatile uint32_t* lapic_lvt_timer   = (volatile uint32_t*)(lapic_base_addr + 0x320);
    volatile uint32_t* lapic_init_count  = (volatile uint32_t*)(lapic_base_addr + 0x380);
    volatile uint32_t* lapic_current_cnt = (volatile uint32_t*)(lapic_base_addr + 0x390);
    volatile uint32_t* lapic_divide_cfg  = (volatile uint32_t*)(lapic_base_addr + 0x3E0);

    *lapic_divide_cfg = 0x03; 
    *lapic_lvt_timer = 0x00010020;

    outb(0x43, 0x30); 
    
    uint16_t pit_start_val = 18000;
    outb(0x40, (uint8_t)(pit_start_val & 0xFF));
    outb(0x40, (uint8_t)((pit_start_val >> 8) & 0xFF));

    uint16_t pit_start = pit_read_current();
    *lapic_init_count = 0xFFFFFFFF;

    for (volatile int i = 0; i < 20000; i++) {
        asm volatile("pause");
        io_delay(); 
    }

    asm volatile("lfence" ::: "memory");

    uint32_t current_lapic = *lapic_current_cnt;
    uint16_t pit_end = pit_read_current();
    *lapic_init_count = 0;

    uint32_t pit_ticks_passed = 0;
    if (pit_end <= pit_start) {
        pit_ticks_passed = pit_start - pit_end;
    } else {
        pit_ticks_passed = pit_start + (0xFFFF - pit_end);
    }

    if (pit_ticks_passed == 0) pit_ticks_passed = 1;

    uint32_t lapic_ticks_passed = 0xFFFFFFFF - current_lapic;

    uint64_t lapic_ticks_per_ms = ((uint64_t)lapic_ticks_passed * 1193) / pit_ticks_passed;

    if (lapic_ticks_per_ms == 0) lapic_ticks_per_ms = 1000; 

    *lapic_lvt_timer = 0x00020020;
    *lapic_init_count = lapic_ticks_per_ms; 
}

uint64_t detect_cpu_frequency(void) {

    uint32_t max_leaf;
    asm volatile("cpuid" : "=a"(max_leaf) : "a"(0) : "bx", "cx", "dx");
    
    if (max_leaf >= 0x16) {
        uint32_t eax, ebx, ecx, edx;
        asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x16));
        uint64_t base_freq_mhz = eax & 0xFFFF;
        
        if (base_freq_mhz > 0) {
            return base_freq_mhz * 1000000; 
        }
    }

    uint64_t freq_by_rdtsc_hz = estimate_cpu_frequency();

    if (freq_by_rdtsc_hz < 500000000) {
        return calibrate_by_bogomips()*2;
    }

    return freq_by_rdtsc_hz;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

const char* get_mbr_fs_type_name(uint8_t fs_type_id) {
    if (fs_type_id >= FS_TYPES_COUNT) {
        return "Undefined FS"; 
    }
    
    if (fs_types_mbr[fs_type_id] == NULL) {
        return "Undefined FS";
    }

    return fs_types_mbr[fs_type_id];
}

void stop_all_other_cores()
{
    volatile uint32_t *lapic_icr_low  = (volatile uint32_t *)(lapic_base_addr + 0x300);
    volatile uint32_t *lapic_icr_high = (volatile uint32_t *)(lapic_base_addr + 0x310);

    *lapic_icr_high = 0;
    *lapic_icr_low = 0xC0500;

    for (volatile int i = 0; i < 10000; i++) {
        __asm__ __volatile__("pause");
    }
}

void acpi_os_shutdown()
{
    __asm__ __volatile__("cli");

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(15, 15, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is shutting down...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    stop_all_other_cores();
    if(net_status != 0) {
        dhcp_release_and_stop(&g_am79c973_netif);
    }

    disk_sync(-1);

    int64_t acpi_status = AcpiEnterSleepStatePrep(ACPI_STATE_S5);
    if(acpi_status != 0) {
        kprintf(15, 30, 0xFF, 0xFF, 0xFF, 0xFF, "AcpiEnterSleepStatePrep error: %d", acpi_status);
        kprintf(15, 45, 0xFF, 0xFF, 0xFF, 0xFF, "It is now safe to turn off your computer");
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }

    acpi_status = AcpiDisableAllGpes();
    if(acpi_status != 0) {
        kprintf(15, 30, 0xFF, 0xFF, 0xFF, 0xFF, "AcpiDisableAllGpes error: %d", acpi_status);
        kprintf(15, 45, 0xFF, 0xFF, 0xFF, 0xFF, "It is now safe to turn off your computer");
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }

    acpi_status = AcpiEnterSleepState(ACPI_STATE_S5);
    if(acpi_status != 0) {
        kprintf(15, 30, 0xFF, 0xFF, 0xFF, 0xFF, "AcpiEnterSleepState error: %d", acpi_status);
        kprintf(15, 45, 0xFF, 0xFF, 0xFF, 0xFF, "It is now safe to turn off your computer");
        flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
        Hold();
    }

    kprintf(15, 30, 0xFF, 0xFF, 0xFF, 0xFF, "It is now safe to turn off your computer");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
    Hold();

    while (1) {
        __asm__ __volatile__("hlt");
    }
}

int64_t push_deferred_task(task_callback_t callback, void* data) {
    int next = (task_queue_head + 1) % TASK_QUEUE_SIZE;
    if (next == task_queue_tail) {
        return 0;
    }
    task_queue[task_queue_head].callback = callback;
    task_queue[task_queue_head].data = data;
    task_queue_head = next;
    return 1;
}

void process_deferred_tasks() {
    while (task_queue_tail != task_queue_head) {
        asm volatile("cli");
        deferred_task_t task = task_queue[task_queue_tail];
        task_queue_tail = (task_queue_tail + 1) % TASK_QUEUE_SIZE;

        asm volatile("sti");

        if (task.callback) {
            task.callback(task.data); 
        }
    }
}

int64_t CreateProcess(void (*entry_point)(void), const char* name) {
    pcb_t* pcb = NULL;
    uint32_t slot_index = 0;

    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        pcb_t* current = &process_table_ptr[i];
        if (current->state == TASK_STATE_ZOMBIE || 
           (current->state == TASK_STATE_EMPTY && current->pid == 0)) {
            pcb = current;
            slot_index = i;
            break;
        }
    }

    if (!pcb) {
        return -1;
    }

    uint64_t slot_base = TASK_STACK_LIMIT + ((uint64_t)slot_index << SLOT_WINDOW_SHIFT);
    uint64_t virt_stack_base = slot_base + 4096;

    for (uint32_t p = 0; p < STACK_PAGES; p++) {
        void* phys_page = (void*)alloc_page();
        if (!phys_page) {
            for (uint32_t rollback = 0; rollback < p; rollback++) {
                uint64_t vaddr = virt_stack_base + (rollback * 4096);
                uint64_t paddr = virtual_to_physical(pt_begin, vaddr);
                unmap_page(vaddr);
                if (paddr) free_page(paddr);
            }
            return -1;
        }
        map_page(virt_stack_base + (p * 4096), (uint64_t)phys_page, 0b111);
    }

    uint64_t* raw_pcb = (uint64_t*)pcb;
    for (size_t i = 0; i < sizeof(pcb_t) / 8; i++) {
        raw_pcb[i] = 0;
    }

    pcb->name   = name ? name : "Unnamed";
    pcb->pid    = slot_index;
    pcb->state  = TASK_STATE_READY;
    pcb->cr3    = pt_begin;
    pcb->rip    = (uint64_t)entry_point;
    pcb->rflags = 0x200;
    pcb->ticks  = 0;

    pcb->PCB_fpu_state = alloc_page();
    if (pcb->PCB_fpu_state == 0) {
        for (uint32_t p = 0; p < STACK_PAGES; p++) {
            uint64_t vaddr = virt_stack_base + (p * 4096);
            uint64_t paddr = virtual_to_physical(pt_begin, vaddr);
            unmap_page(vaddr);
            if (paddr) free_page(paddr);
        }
        return -1;
    }

    asm volatile (
        "fninit\n\t"
        "fxsave (%0)"
        : 
        : "r"(pcb->PCB_fpu_state) 
        : "memory"
    );

    uint64_t* stack_top = (uint64_t*)(virt_stack_base + STACK_SIZE - 8);

    *stack_top = 0x10;
    
    uint64_t saved_rsp = (uint64_t)stack_top;
    stack_top--;
    *stack_top = saved_rsp;

    stack_top--;
    *stack_top = 0x200;

    stack_top--;
    *stack_top = 0x08;

    stack_top--;
    *stack_top = (uint64_t)entry_point;

    for (int i = 0; i < 15; i++) {
        stack_top--;
        *stack_top = 0;
    }

    pcb->rsp = (uint64_t)stack_top;

    total_processes++;
    return (int64_t)slot_index;
}

int64_t CreateUserProcess(uint8_t* elf_buffer, const char* name) {
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*)elf_buffer;
    if (*(uint32_t*)elf_header->e_ident != 0x464C457F) {
        return -1;
    }

    pcb_t* pcb = NULL;
    uint32_t slot_index = 0;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        pcb_t* current = &process_table_ptr[i];
        if (current->state == TASK_STATE_ZOMBIE || 
           (current->state == TASK_STATE_EMPTY && current->pid == 0)) {
            pcb = current;
            slot_index = i;
            break;
        }
    }
    if (!pcb) return -1;

    uint64_t* user_pml4 = create_user_pml4();
    if (!user_pml4) return -1;

    if (!load_elf_segments(elf_buffer, user_pml4)) {
        free_user_pml4(user_pml4);
        return -1;
    }

    uint64_t user_stack_bottom = USER_STACK_TOP - (USER_STACK_PAGES * PAGE_SIZE);
    for (uint32_t p = 0; p < USER_STACK_PAGES; p++) {
        void* phys_page = (void*)alloc_page();
        if (!phys_page) {
            free_user_pml4(user_pml4);
            return -1;
        }
        for (int b = 0; b < PAGE_SIZE; b++) ((uint8_t*)phys_page)[b] = 0;

        if (!map_page_in_pml4(user_stack_bottom + (p * PAGE_SIZE), 
                              (uint64_t)phys_page, 
                              PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER, 
                              user_pml4)) {
            free_page((uint64_t)phys_page);
            free_user_pml4(user_pml4);
            return -1;
        }
    }

    uint64_t slot_base = TASK_STACK_LIMIT + ((uint64_t)slot_index << SLOT_WINDOW_SHIFT);
    uint64_t kstack_base = slot_base + 4096;

    for (uint32_t p = 0; p < STACK_PAGES; p++) {
        void* phys_page = (void*)alloc_page();
        if (!phys_page) {
            for (uint32_t rb = 0; rb < p; rb++) {
                uint64_t vaddr = kstack_base + (rb * 4096);
                uint64_t paddr = virtual_to_physical(pt_begin, vaddr);
                unmap_page(vaddr);
                if (paddr) free_page(paddr);
            }
            free_user_pml4(user_pml4);
            return -1;
        }
        map_page(kstack_base + (p * 4096), (uint64_t)phys_page, 0b011);
    }

    uint64_t* raw_pcb = (uint64_t*)pcb;
    for (size_t i = 0; i < sizeof(pcb_t) / 8; i++) {
        raw_pcb[i] = 0;
    }

    pcb->name   = name ? name : "UserProcess";
    pcb->pid    = slot_index;
    pcb->state  = TASK_STATE_READY;
    pcb->cr3    = (uint64_t)user_pml4;
    pcb->rip    = elf_header->e_entry;
    pcb->rflags = 0x202;
    pcb->ticks  = 0;

    uint64_t kstack_top_addr = kstack_base + STACK_SIZE;
    // pcb->kernel_stack_top = kstack_top_addr;

    pcb->PCB_fpu_state = alloc_page();
    if (!pcb->PCB_fpu_state) {
        return -1;
    }
    asm volatile ("fninit\n\tfxsave (%0)" : : "r"(pcb->PCB_fpu_state) : "memory");

    uint64_t* stack_top = (uint64_t*)(kstack_top_addr - 8);

    *stack_top = USER_SS;
    
    stack_top--;
    *stack_top = USER_STACK_TOP - 8;

    stack_top--;
    *stack_top = 0x202;

    stack_top--;
    *stack_top = USER_CS;

    stack_top--;
    *stack_top = elf_header->e_entry;

    for (int i = 0; i < 15; i++) {
        stack_top--;
        *stack_top = 0;
    }

    pcb->rsp = (uint64_t)stack_top;

    total_processes++;
    return (int64_t)slot_index;
}

int64_t KillProcess(uint32_t pid) {
    if (pid >= MAX_PROCESSES || pid == 0 || pid == 1) {
        return -1;
    }

    uint64_t rflags;
    __asm__ volatile ("pushfq; pop %0; cli" : "=r"(rflags) :: "memory");

    pcb_t* pcb = &process_table_ptr[pid];

    if (pcb->state == TASK_STATE_EMPTY || pcb->state == TASK_STATE_ZOMBIE) {
        if (rflags & 0x200) __asm__ volatile ("sti");
        return -1;
    }

    uint64_t slot_base = TASK_STACK_LIMIT + ((uint64_t)pid << SLOT_WINDOW_SHIFT);
    uint64_t virt_stack_base = slot_base + 4096;
        
    for (uint32_t p = 0; p < STACK_PAGES; p++) {
        uint64_t vaddr = virt_stack_base + (p * 4096);
        uint64_t phys_stack = virtual_to_physical(pt_begin, vaddr);
        if (phys_stack) {
            unmap_page(vaddr);
            free_page(phys_stack);
        }
    }

    pcb->state = TASK_STATE_EMPTY;
    pcb->pid   = 0;
    pcb->name  = NULL;
    pcb->rsp   = 0;
    pcb->rip   = 0;
    pcb->ticks = 0;

    if (pcb->PCB_fpu_state != 0) {
        free_page(pcb->PCB_fpu_state);
        pcb->PCB_fpu_state = 0;
    }

    if (total_processes > 0) {
        total_processes--;
    }

    if (rflags & 0x200) {
        __asm__ volatile ("sti");
    }

    return 0;
}

int is_process_alive(const char* target_name) {
    pcb_t* tasks = process_table_ptr; 
    for (int i = 0; i < MAX_PROCESSES; i++) {

        pcb_t* task = &tasks[i];

        if (task->state == TASK_STATE_EMPTY || task->state == TASK_STATE_ZOMBIE) {
            continue;
        }

        if (strcmp(task->name, target_name) == 0) {
            return 1;
        }

    }

    return 0;
}

#undef strstr
char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

uint8_t read_cmos(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

int is_updating() {
    outb(0x70, 0x0A);
    return (inb(0x71) & 0x80);
}

void read_rtc(int *year, int *month, int *day, int *hour, int *min, int *sec) {
    uint8_t century = 0;
    uint8_t last_sec, last_min, last_hour, last_day, last_month, last_year, last_century;
    uint8_t registerB;

    while (is_updating());

    *sec   = read_cmos(0x00);
    *min   = read_cmos(0x02);
    *hour  = read_cmos(0x04);
    *day   = read_cmos(0x07);
    *month = read_cmos(0x08);
    *year  = read_cmos(0x09);
    century = read_cmos(0x32);

    do {
        last_sec     = *sec;
        last_min     = *min;
        last_hour    = *hour;
        last_day     = *day;
        last_month   = *month;
        last_year    = *year;
        last_century = century;

        while (is_updating());

        *sec   = read_cmos(0x00);
        *min   = read_cmos(0x02);
        *hour  = read_cmos(0x04);
        *day   = read_cmos(0x07);
        *month = read_cmos(0x08);
        *year  = read_cmos(0x09);
        century = read_cmos(0x32);
    } while ((last_sec != *sec) || (last_min != *min) || (last_hour != *hour) ||
             (last_day != *day) || (last_month != *month) || (last_year != *year) ||
             (last_century != century));

    registerB = read_cmos(0x0B);

    if (!(registerB & 0x04)) {
        *sec   = (*sec & 0x0F) + ((*sec / 16) * 10);
        *min   = (*min & 0x0F) + ((*min / 16) * 10);
        *hour  = ((*hour & 0x0F) + (((*hour & 0x70) / 16) * 10)) | (*hour & 0x80);
        *day   = (*day & 0x0F) + ((*day / 16) * 10);
        *month = (*month & 0x0F) + ((*month / 16) * 10);
        *year  = (*year & 0x0F) + ((*year / 16) * 10);
        if (century != 0) {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    if (!(registerB & 0x02) && (*hour & 0x80)) {
        *hour = ((*hour & 0x7F) + 12) % 24;
    }

    if (century != 0) {
        *year += century * 100;
    } else {
        *year += 2000;
    }
}

uint64_t to_unix_timestamp(int year, int month, int day, int hour, int min, int sec) {
    uint64_t days = 0;
    for (int y = 1970; y < year; y++) {
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        days_in_month[1] = 29;
    }
    for (int m = 1; m < month; m++) {
        days += days_in_month[m - 1];
    }
    days += (day - 1);
    return days * 86400 + hour * 3600 + min * 60 + sec;
}

void KiInitializeKernel() {

    char *bss = &_bss_start;
    while (bss < &_bss_end) {
        *bss++ = 0;
    }

    init_double_buffering();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "Calibrating Local APIC Timer...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    calibrate_lapic_timer();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(340, 550, 0xFF, 0xFF, 0xFF, 0xFF, "Detecting CPU...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    g_sys_info.cpu.frequency_hz = detect_cpu_frequency();
    g_sys_info.cpu.frequency_mhz = g_sys_info.cpu.frequency_hz / 1000000;
    get_cpu_brand_string(g_sys_info.cpu.brand_string);

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(330, 550, 0xFF, 0xFF, 0xFF, 0xFF, "Scanning SMBIOS...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    init_smbios();

    asm volatile("sti");

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(320, 550, 0xFF, 0xFF, 0xFF, 0xFF, "Initializing Multiprocessing (SMP)...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    init_logical_processors();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is checking for: SATA Devices...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    init_ahci();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is checking for: IDE Devices...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    init_ata_dma();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is checking for: NVMe Devices...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    int status = init_nvme();
    if(status != 0) {

        for(int i = 0; i < nvme_controller_count; i++) {

            status = nvme_controller_reset(i);
            if(status != 0) {
                nvme_identify_disk(i);
                nvme_identify_namespace(i);
                status = nvme_create_io_queues(i);

                if(status != 0) {
                    registerDisk(2, i, 0, nvme_get_sectors_count(i), 0);
                }
            }
        }
    }

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is checking for: USB Devices...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    pci_device_t* xhci_controller = find_xhci_controller();
    if(xhci_controller != 0) {
        uintptr_t xhci_addr = get_xhci_base_address(xhci_controller);

        if(xhci_addr != 0) {

            enable_xhci_pci_bus(xhci_controller);
            intel_xhci_route_ports(xhci_controller);
            xhci_read_capabilities(xhci_addr);
            xhci_bios_handoff(xhci_addr);
            xhci_reset_controller(xhci_addr);
            xhci_init_event_ring(xhci_addr);
            xhci_init_dcbaa(xhci_addr);
            xhci_init_command_ring(xhci_addr);
            xhci_finalize_controller_init(xhci_addr);
            xhci_start_controller(xhci_addr);

            sleep_ms(150);
            xhci_probe_ports(xhci_addr);

            for(int i = 0; i < MAX_USB_DEVICES; i++) {
                if(usb_devices[i].ready_for_storage == 1) {
                    xhci_usb_msc_init(xhci_addr, i);
                    registerDisk(3, i, 0, 0, 0);
                }
            }

        }

    }

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is initializes the disk subsystem...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    disk_cache_init();

    registerSATADevices();
    registerATADevices();
    registerSATAPIDevices();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(290, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is initializes the partition subsystem...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    partition_scan_all_drives();

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(290, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is initializes the audio subsystem...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    /*int hd_status = init_hdaudio();
    if(hd_status == 1) {
        hd_status = hda_reset_and_init_rings();
        if(hd_status == 1) {
            if (setup_audio_path(0)) {
                hda_play_test_tone();
            }
        }
    }*/

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is initializes the network card...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    net_status = init_am79c973(&am73c973_device);
    if(net_status != 0) {
        device_mac = am73c973_device.mac;
        device_ip = kmalloc(4);
        if(device_ip == NULL) {
            KeBugCheck(0x20);
        }
        device_ip[0] = 192;
        device_ip[1] = 168;
        device_ip[2] = 0;
        device_ip[3] = 200;
        
        lwip_init();
        dns_init();

        ip4_addr_t ipaddr, netmask, gw;
        ip4_addr_set_zero(&ipaddr);
        ip4_addr_set_zero(&netmask);
        ip4_addr_set_zero(&gw);

        netif_add(&g_am79c973_netif, &ipaddr, &netmask, &gw, NULL, am79c973_netif_init, ethernet_input);
        netif_set_default(&g_am79c973_netif);
        netif_set_up(&g_am79c973_netif);
        dhcp_start(&g_am79c973_netif);

        int year, month, day, hour, min, sec;
        read_rtc(&year, &month, &day, &hour, &min, &sec);

        uint64_t unix_timestamp = to_unix_timestamp(year, month, day, hour, min, sec);
        uint32_t days = (uint32_t)(unix_timestamp / 86400) + 719528;
        uint32_t seconds = (uint32_t)(unix_timestamp % 86400);

        https_init(days, seconds);

    }

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(310, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is initializes the ACPI...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    AcpiDbgLayer = 0x0001FFFF;
    AcpiDbgLevel = 0x0000001F | 0x00000001;

    acpi_status = AcpiInitializeSubsystem();
    if(acpi_status != 0) {
        KeBugCheck(0x21);
    }

    acpi_status = AcpiInitializeTables(NULL, 0, 1);
    if(acpi_status != 0) {
        KeBugCheck(0x21);
    }

    acpi_status = AcpiLoadTables();
    if(acpi_status != 0) {
        KeBugCheck(0x21);
    }

    acpi_status = AcpiEnableSubsystem(0);
    if(acpi_status != 0) {
        KeBugCheck(0x21);
    }

    acpi_status = AcpiInitializeObjects(0);
    if(acpi_status != 0) {
        KeBugCheck(0x21);
    }

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(290, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is trying to find a boot partition...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);
    
    Partition* bootPartition = find_boot_partition(boot_info);

    if(bootPartition == NULL) {
        KeBugCheck(0x22);
    }

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    if(boot_info->is_setup == 0) os_state = 1;
    else os_state = 1;

    asm volatile("cli");
    mouse_init();
    asm volatile("sti");

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is loading: BACK_S.BMP...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    LoadBackground(bootPartition->drive_index);

    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0x00, 0x00, 0x00, 0xFF);
    kprintf(300, 550, 0xFF, 0xFF, 0xFF, 0xFF, "OrangeOS is loading: INTER.TTF...");
    flip_buffers(fb_front_buffer, fb_back_buffer, SCREEN_WIDTH , SCREEN_HEIGHT);

    LoadFont(bootPartition->drive_index);

    if(os_state == 2) {
        KeSetupGraphic();
    }

    dwm_pid = CreateProcess(dwm_thread, "Display Window Manager");
    if(dwm_pid == -1) {
        KeBugCheck(0x20);
    }

    cache_pid = CreateProcess(kernel_cache_sync_thread, "Cache Manager");
    if(cache_pid == -1) {
        KeBugCheck(0x20);
    }

    while (1) {
        process_deferred_tasks();
        if(net_status != 0) {
            check_incoming_packets();
            sys_check_timeouts();
            https_poll(&https_client);
        }

        if (!is_process_alive("Display Window Manager")) {
            CreateProcess(dwm_thread, "Display Window Manager");
        }

        if (!is_process_alive("Cache Manager")) {
            CreateProcess(kernel_cache_sync_thread, "Cache Manager");
        }

        window_t* active_win = get_window_by_id(window_in_focus);

        while (kbhit()) {
            char c = getch();

            if (active_win != NULL && active_win->on_key != NULL) {
                active_win->on_key(active_win, c);
            }
        }

        asm volatile("hlt");
    }
}

void kp_entry_ap() {

    asm volatile(
        "lgdt %0\n\t"

        "pushq $0x08\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"

        "lretq\n\t"                    

        "1:\n\t"                       
    
        "mov $0x10, %%ax\n\t" 
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%ss\n\t"
        "xor %%ax, %%ax\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        : 
        : "m"(GDTR_64)
        : "rax", "memory"
    );

    uint8_t my_apic_id = get_lapic_id();
    uint64_t my_stack_top = core_tss_stacks[my_apic_id];

    initialize_core_tss(my_apic_id, my_stack_top);

    asm volatile("lidt %0" :: "m"(idt_pointer));

    volatile int8_t* flag = (volatile int8_t*)&ap_running_flag;
    *flag = 1;

    while(1) {
        asm volatile("hlt" ::: "memory");
    }
}