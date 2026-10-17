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

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

extern void spin_lock(spinlock_t *sl);
extern void spin_unlock(spinlock_t *sl);

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
extern void KeBugACPIError();
extern void KeBugInaccesibleBootDevice();

extern int64_t iso9660_init();
extern void read_atapi_sector(void* buffer, int64_t lba, int64_t sectors_count);
extern uint64_t iso9660_find_file(const void* buffer, const char* filename, uint64_t length, uint64_t* out_size);

uint32_t swap_uint32(uint32_t val);

extern int64_t GDTR_64;

void KeBugCheck(int stopCode);
void kprintf(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...);

#define KEY_F8 0x42

#define LAPIC_ID_OFFSET 0x020

#pragma pack(push, 1)
typedef struct {
    uint8_t  boot_indicator;
    uint8_t  start_chs[3];
    uint8_t  partition_type;
    uint8_t  end_chs[3];
    uint32_t start_lba;
    uint32_t sector_count;
} MBR_PartitionEntry;

#pragma pack(push, 1)

typedef struct {
    uint16_t size;
    uint16_t flags;
    uint32_t cylinders;
    uint32_t heads;
    uint32_t sectors_per_track;
    uint64_t total_sectors;
    uint16_t bytes_per_sector;
    
    uint32_t edd_cfg_ptr;
    uint16_t host_bus_type;
    uint64_t interface_type;
    uint64_t interface_path;
    uint64_t device_path;
    uint8_t  reserved;
    uint8_t  checksum;
} edd_drive_params_t;

typedef struct {
    uint16_t vbe_hor_size;
    uint16_t vbe_ver_size;
    uint8_t  vbe_color_depth;
    uint16_t vbe_pitch;
    uint8_t  loading_drive;
    uint32_t lfb_buffer_addr;
    edd_drive_params_t drive_params;
    uint8_t reserved[14];
    uint32_t boot_pvd_hash;
    uint32_t cpu_count;
    uint8_t cpus_ids[256];
    uint64_t bsp_factory_ist1;
} boot_info_t;

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} tss64_t;

typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid1;
    uint8_t  flags1;
    uint8_t  flags2;
    uint8_t  base_mid2;
    uint32_t base_high;
    uint32_t reserved;
} tss_descriptor_t;

extern tss_descriptor_t gdt_tss_slots[];

#pragma pack(pop)

typedef struct {
    uint8_t            bootstrap_code[446];
    MBR_PartitionEntry partitions[4];
    uint16_t           boot_signature;
} MBR_Sector;

extern uint64_t timer_ticks;
extern boot_info_t* boot_info;

void openDeviceManager();
void openAbout();
void Hold();

#define LAPIC_ICR_LOW   ((volatile uint32_t*)(lapic_base_addr + 0x300))
#define LAPIC_ICR_HIGH  ((volatile uint32_t*)(lapic_base_addr + 0x310))

extern uint64_t mb_memory;
extern uintptr_t lapic_base_addr;
extern uintptr_t idt_pointer;

extern uint64_t ap_target_cr3;
extern uint64_t ap_stack_top;

extern uintptr_t trampoline_start;
extern uintptr_t trampoline_end;

extern uint32_t AcpiDbgLayer;
extern uint32_t AcpiDbgLevel;

typedef uint32_t ACPI_STATUS;
extern ACPI_STATUS AcpiInitializeSubsystem();