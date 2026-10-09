#include <stdint.h>
#include <stddef.h>

extern void cpu_get_cpuid(uint32_t code, uint32_t regs[4]);
extern void print_text(const char *str, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
extern void t_print(const char* text, uint8_t x, uint8_t y, uint8_t color);
extern void update_status_bar(const char* status_text);
extern void clear_setup_screen();
extern uint64_t getch();
extern void sleep_ms(int milliseconds);

void int_to_str(long long value, char *buf, int base, int is_signed);
uint64_t str_to_int(const char* str);
void cache_flush(void* addr, uint32_t size);
uint64_t ms_to_ticks(uint64_t milliseconds);

void tprintf(uint8_t x, uint8_t y, uint8_t color, const char *fmt, ...);

extern void     outb(uint16_t port, uint8_t value);
extern void     outw(uint16_t port, uint16_t value);
extern void     outl(uint16_t port, uint32_t value);
extern uint8_t  inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

extern uint64_t alloc_page();
extern void free_page(uint64_t addr);

extern uintptr_t pt_begin;
extern uint32_t root_lba;

extern void     map_page(uint64_t virt, uint64_t phys, uint64_t flags);
extern void     unmap_page(uint64_t virt);

extern void* memcpy(void* dest, const void* src, size_t n);
extern void *memset(void *s, int c, size_t n);

extern size_t strlen(const char *restrict s);

extern void*    kmalloc(uint64_t size);
extern void*    kcalloc(size_t num, size_t size);
extern void     kfree(void* ptr);

extern int get_used_memory_mb_no_popcnt();

extern void KeBugMemoryManagement();

extern int64_t iso9660_init();
extern void read_atapi_sector(void* buffer, int64_t lba, int64_t sectors_count);
extern uint64_t iso9660_find_file(const void* buffer, const char* filename, uint64_t length, uint64_t* out_size);

uint32_t swap_uint32(uint32_t val);

void KeBugCheck(int stopCode);
void kprintf(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...);

#define KEY_F8 0x42

#pragma pack(push, 1)
typedef struct {
    uint8_t  boot_indicator;
    uint8_t  start_chs[3];
    uint8_t  partition_type;
    uint8_t  end_chs[3];
    uint32_t start_lba;
    uint32_t sector_count;
} MBR_PartitionEntry;

typedef struct {
    uint8_t            bootstrap_code[446];
    MBR_PartitionEntry partitions[4];
    uint16_t           boot_signature;
} MBR_Sector;

extern uint64_t timer_ticks;

void openDeviceManager();
void openAbout();

extern uint64_t mb_memory;