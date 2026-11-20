#ifndef CORE_H

#define CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "unicode.h"

#define STACK_PAGES       4
#define STACK_SIZE        (STACK_PAGES * 4096)
#define SLOT_WINDOW_SHIFT 14

#define PAGE_PRESENT   (1ULL << 0)
#define PAGE_WRITABLE  (1ULL << 1)
#define PAGE_USER      (1ULL << 2)
#define PAGE_PWT       (1ULL << 3)
#define PAGE_PCD       (1ULL << 4)
#define PAGE_NX        (1ULL << 63)

extern void cpu_get_cpuid(uint32_t code, uint32_t regs[4]);
extern void print_text(const char *str, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);
extern void t_print(const char* text, uint8_t x, uint8_t y, uint8_t color);
extern void update_status_bar(const char* status_text);
extern void clear_setup_screen();
extern uint64_t getch();
extern void sleep_ms(int milliseconds);

typedef void (*task_callback_t)(void* data);
typedef struct {
    task_callback_t callback;
    void* data;
} deferred_task_t;

#define TASK_QUEUE_SIZE 256
extern deferred_task_t task_queue[TASK_QUEUE_SIZE];
extern volatile int task_queue_head;
extern volatile int task_queue_tail;

typedef struct {
    window_t* window;
    int mouse_x;
    int mouse_y;
} task_event_mouse_click_t;

void int_to_str(long long value, char *buf, int base, int is_signed, int uppercase);
uint64_t str_to_int(const char* str);
void cache_flush(void* addr, uint32_t size);
uint64_t ms_to_ticks(uint64_t milliseconds);

const char* get_mbr_fs_type_name(uint8_t fs_type_id);
int64_t push_deferred_task(task_callback_t callback, void* data);

void tprintf(uint8_t x, uint8_t y, uint8_t color, const char *fmt, ...);

extern void     outb(uint16_t port, uint8_t value);
extern void     outw(uint16_t port, uint16_t value);
extern void     outl(uint16_t port, uint32_t value);
extern uint8_t  inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

extern int strcmp(const char *s1, const char *s2);
extern int memcmp(const void *s1, const void *s2, size_t n);

#define USER_CS (0x20 | 3)
#define USER_SS (0x18 | 3)

#define USER_STACK_TOP   0x00007FFFFFF00000ULL
#define USER_STACK_PAGES 4

typedef struct {
    volatile uint32_t lock;
} spinlock_t;

extern void spin_lock(spinlock_t *sl);
extern void spin_unlock(spinlock_t *sl);

extern uint64_t alloc_page();
extern void free_page(uint64_t addr);

extern uintptr_t pt_begin;
extern uint32_t root_lba;

extern int     map_page(uint64_t virt, uint64_t phys, uint64_t flags);
extern void     unmap_page(uint64_t virt);

extern uint8_t map_page_in_pml4(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags, uint64_t* pml4);
uint64_t get_phys_from_pml4(uint64_t *pml4, uint64_t virt_addr);

uint64_t* create_user_pml4();
void free_user_pml4(uint64_t* user_pml4);

extern uint8_t nxe_supported;

extern void* memcpy(void* dest, const void* src, size_t n);
extern void *memset(void *s, int c, size_t n);

extern size_t strlen(const char *restrict s);

extern void*    kmalloc(uint64_t size);
extern void*    kcalloc(size_t num, size_t size);
extern void     kfree(void* ptr);

int ksnprintf(char *buffer, size_t size, const char *fmt, ...);

extern int get_used_memory_mb_no_popcnt();

extern void KeBugMemoryManagement();
extern void KeBugACPIError();
extern void KeBugInaccesibleBootDevice();
extern void KeBugCriticalProcessDied();

extern volatile int64_t cmos_ticks;

extern void delay_loop(uint64_t loops);

extern int64_t iso9660_init();
extern int64_t read_atapi_sector(void* buffer, int64_t lba, int64_t sectors_count, uint8_t drive_number);
extern uint64_t iso9660_find_file(const void* buffer, const char* filename, uint64_t length, uint64_t* out_size);

uint32_t swap_uint32(uint32_t val);

extern int64_t GDTR_64;

extern int64_t create_task(void* addr);

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

struct cpu_features {
    uint64_t frequency_hz;
    uint32_t frequency_mhz;
    char brand_string[49];
    uint8_t has_invariant_tsc;
};

struct ram_module {
    uint16_t speed_mhz;
    uint64_t size_bytes;
};

#define MAX_RAM_SLOTS 32

struct system_info {
    struct cpu_features cpu;
    
    struct ram_module ram_slots[MAX_RAM_SLOTS];
    uint32_t ram_slot_count;
    uint64_t total_ram_bytes;
};

extern struct system_info g_sys_info;

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

typedef struct __attribute__((packed)) {
    uint16_t mode_id;
    uint16_t width;
    uint16_t height;
    uint8_t  bpp;
    uint32_t physical_base_pointer;
} boot_vbe_mode_t;


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
    uint8_t is_setup;
    uint32_t vbe_modes_count;
    boot_vbe_mode_t* vbe_modes_list;
} __attribute__((packed)) boot_info_t;

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

extern int global_flag;

extern tss_descriptor_t gdt_tss_slots[];

#pragma pack(pop)

extern uint8_t *device_mac;
extern uint8_t *device_ip;

extern uint32_t total_processes;

typedef struct {
    uint8_t r; uint8_t g; uint8_t b; uint8_t a;
} IconPixel;

typedef struct {
    uint8_t            bootstrap_code[446];
    MBR_PartitionEntry partitions[4];
    uint16_t           boot_signature;
} MBR_Sector;

extern uint64_t timer_ticks;
extern boot_info_t* boot_info;

int toupper(int c);
int tolower(int c);

void openDeviceManager();
void openAbout();
void Hold();

extern uint64_t get_used_memory_mb();
extern int kbhit();

int kvsnprintf(char *buffer, size_t size, const char *fmt, va_list args);

extern uint64_t internet_packages_sended;
extern uint64_t internet_packages_received;

void acpi_os_shutdown();

#define LAPIC_ICR_LOW   ((volatile uint32_t*)(lapic_base_addr + 0x300))
#define LAPIC_ICR_HIGH  ((volatile uint32_t*)(lapic_base_addr + 0x310))

extern uint64_t mb_memory;
extern uintptr_t lapic_base_addr;
extern uintptr_t idt_pointer;

extern uint64_t ap_target_cr3;
extern uint64_t ap_stack_top;

extern uintptr_t trampoline_start;
extern uintptr_t trampoline_end;

int64_t KillProcess(uint32_t pid);

extern uint32_t AcpiDbgLayer;
extern uint32_t AcpiDbgLevel;

#define ACPI_STATE_S5    5

typedef uint32_t ACPI_STATUS;
extern ACPI_STATUS AcpiInitializeSubsystem();
extern ACPI_STATUS AcpiInitializeTables(
    void *initial_table_array,
    uint32_t initial_table_count,
    uint8_t allow_resize
);
extern ACPI_STATUS AcpiLoadTables();
extern ACPI_STATUS AcpiEnableSubsystem (uint32_t Flags);
extern ACPI_STATUS AcpiInitializeObjects (uint32_t Flags);

extern ACPI_STATUS AcpiEnterSleepStatePrep(uint8_t sleep_state);
extern ACPI_STATUS AcpiDisableAllGpes(void);
extern ACPI_STATUS AcpiEnterSleepState(uint8_t sleep_state);

#define MAX_PROCESSES 32

#define TASK_STATE_EMPTY 0
#define TASK_STATE_READY 1
#define TASK_STATE_RUNNING 2
#define TASK_STATE_SLEEP 3
#define TASK_STATE_ZOMBIE 4
#define TASK_STACK_LIMIT 0xFFFF800000000000

typedef struct {
    const char* name;
    uint64_t    PCB_fpu_state;
    uint64_t    rcx;
    uint64_t    rdx;
    uint64_t    rsi;
    uint64_t    rdi;
    uint64_t    rbp;
    uint64_t    rsp;
    uint64_t    rip;
    uint64_t    rflags;
    uint64_t    cr3;
    uint64_t    pid;
    uint64_t    state;
    uint64_t    ticks;
} __attribute__((packed)) pcb_t;

extern pcb_t* process_table_ptr;

_Static_assert(sizeof(pcb_t) == 112, "PCB size mismatch! Must be 112 bytes.");

#endif