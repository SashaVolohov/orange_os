#ifndef ACPI_DEBUGGER_OFF
#define ACPI_DEBUGGER_OFF
#endif

#ifndef ACPI_DISASSEMBLER_OFF
#define ACPI_DISASSEMBLER_OFF
#endif

#define _COMPONENT ACPI_OS_SERVICES

#define ACPI_STRCPY(d,s)        strcpy(d,s)
#define ACPI_STRNCPY(d,s,n)     strncpy(d,s,n)
#define ACPI_STRCAT(d,s)        strcat(d,s)
#define ACPI_STRNCAT(d,s,n)     strncat(d,s,n)
#define ACPI_STRCMP(d,s)        strcmp(d,s)
#define ACPI_STRNCMP(d,s,n)     strncmp(d,s,n)
#define ACPI_STRLEN(s)          strlen(s)
#define ACPI_MEMCPY(d,s,n)      memcpy(d,s,n)
#define ACPI_MEMSET(d,v,n)      memset(d,v,n)
#define ACPI_MEMCMP(d,s,n)      memcmp(d,s,n)

#include "acpi.h"

extern void*    kmalloc(uint64_t size);
extern void     kfree(void* ptr);
extern void     map_page(uint64_t virt, uint64_t phys, uint64_t flags);
extern void     unmap_page(uint64_t virt);
extern void     outb(uint16_t port, uint8_t value);
extern void     outw(uint16_t port, uint16_t value);
extern void     outl(uint16_t port, uint32_t value);
extern uint8_t  inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

void Hold();
void kprintf(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha, const char *fmt, ...);

typedef struct {
    UINT32 max_units;
    UINT32 initial_units;
    UINT32 current_units;
} acpi_sem_t;

typedef struct {
    int locked;
} acpi_lock_t;

extern uint64_t acpi_rsdp_addr;

void print_text(const char *str, int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char alpha);

static uint8_t acpi_test_buffer[65536];
static size_t acpi_buffer_offset = 0;

uint64_t acpi_virt_cursor = 0xFFFF800010000000;

ACPI_STATUS AcpiOsInitialize(void) {
    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void) {
    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void) {

    asm volatile("ud2");

    if (acpi_rsdp_addr != 0) {
        return (ACPI_PHYSICAL_ADDRESS)acpi_rsdp_addr;
    }
    
    return 0; 
}

void *AcpiOsAllocate (ACPI_SIZE Size) {
    void *raw = kmalloc(Size + 16); 
    if (!raw) return NULL;

    void *aligned = (void *)(((uintptr_t)raw + 15) & ~15);
    
    if (aligned == raw) {
        aligned = (void *)((uintptr_t)aligned + 16);
    }

    ((void **)aligned)[-1] = raw;

    memset(aligned, 0, Size);
    return aligned;
}

void AcpiOsFree (void *Memory) {
    if (!Memory) return;
    void *raw = ((void **)Memory)[-1];
    kfree(raw);
}

/*void *AcpiOsAllocate(ACPI_SIZE Size)
{
    uint64_t clean_size = (uint64_t)Size; 
    void* ptr = NULL;

    if (clean_size == 0) return NULL;

    ptr = kmalloc(clean_size);

    if(ptr == 0) {
        asm volatile("ud2");
    }

    return ptr;
}

void AcpiOsFree(void* Memory) {
    kfree(Memory);
}*/

#define ACPI_MAX_MAP_SLOTS 32

typedef struct {
    uint64_t phys_page;
    void* virt_addr;
    uint64_t num_pages;
} ACPI_MAP_SLOT;

static ACPI_MAP_SLOT acpi_map_slots[ACPI_MAX_MAP_SLOTS];
static int acpi_map_slots_count = 0;

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    asm volatile("ud2");
    uint64_t phys_page = (uint64_t)PhysicalAddress & ~0xFFFU;
    uint64_t offset    = (uint64_t)PhysicalAddress & 0xFFFU;
    uint64_t num_pages = ((uint64_t)Length + offset + 4095) / 4096;

    for (int i = 0; i < acpi_map_slots_count; i++) {
        if (acpi_map_slots[i].phys_page == phys_page) {
            if (acpi_map_slots[i].num_pages >= num_pages) {
                return (void*)((uint64_t)acpi_map_slots[i].virt_addr + offset);
            }
        }
    }

    uint64_t virt_start = acpi_virt_cursor;

    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t current_virt = virt_start + (i * 4096);
        uint64_t current_phys = phys_page + (i * 4096);

        asm volatile (
            "push %%rbx\n\t push %%r12\n\t push %%r13\n\t push %%r14\n\t push %%r15\n\t"
            "mov %[v_addr], %%rdi\n\t"
            "mov %[p_addr], %%rsi\n\t"
            "mov $0x3, %%rdx\n\t"
            "call map_page\n\t"
            "pop %%r15\n\t; pop %%r14\n\t; pop %%r13\n\t; pop %%r12\n\t; pop %%rbx"
            : : [v_addr] "r" (current_virt), [p_addr] "r" (current_phys)
            : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11", "memory"
        );
        asm volatile("invlpg (%0)" : : "r"(current_virt) : "memory");
    }

    void* result_ptr = (void*)(virt_start + offset);
    acpi_virt_cursor += (num_pages * 4096);

    if (acpi_map_slots_count < ACPI_MAX_MAP_SLOTS) {
        acpi_map_slots[acpi_map_slots_count].phys_page = phys_page;
        acpi_map_slots[acpi_map_slots_count].virt_addr = (void*)virt_start;
        acpi_map_slots[acpi_map_slots_count].num_pages = num_pages;
        acpi_map_slots_count++;
    }

    return result_ptr;
}

void AcpiOsUnmapMemory(void* LogicalAddress, ACPI_SIZE Length) {

    uint64_t virt_start = (uint64_t)LogicalAddress;
    uint64_t offset     = virt_start & 0xFFF;
    
    uint64_t num_pages = ((uint64_t)Length + offset + 4095) / 4096;
    uint64_t virt_page = virt_start & ~0xFFF;

    for (uint64_t i = 0; i < num_pages; i++) {
        uint64_t v = virt_page + (i * 4096);
        unmap_page(v);
    }
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width) {
    if (Width == 8)       *Value = (UINT32)inb((uint16_t)Address);
    else if (Width == 16) *Value = (UINT32)inw((uint16_t)Address);
    else if (Width == 32) *Value = (UINT32)inl((uint16_t)Address);
    else return AE_BAD_PARAMETER;
    
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width) {
    if (Width == 8)       outb((uint16_t)Address, (uint8_t)Value);
    else if (Width == 16) outw((uint16_t)Address, (uint16_t)Value);
    else if (Width == 32) outl((uint16_t)Address, (uint32_t)Value);
    else return AE_BAD_PARAMETER;
    
    return AE_OK;
}

ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutLockHandle)
{
    if (!OutLockHandle) return AE_BAD_PARAMETER;
    
    acpi_lock_t *lock = kmalloc(sizeof(acpi_lock_t));
    if (!lock) return AE_NO_MEMORY;
    
    lock->locked = 0;
    *OutLockHandle = (ACPI_SPINLOCK)lock;
    return AE_OK;
}

void AcpiOsDeleteLock (ACPI_SPINLOCK Handle) {
    if (Handle) kfree(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock (ACPI_SPINLOCK Handle) { return 0; }
void AcpiOsReleaseLock (ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {}

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle)
{
    if (!OutHandle) return AE_BAD_PARAMETER;

    acpi_sem_t *sem = kmalloc(sizeof(acpi_sem_t));
    if (!sem) return AE_NO_MEMORY;

    sem->max_units = MaxUnits;
    sem->initial_units = InitialUnits;
    sem->current_units = InitialUnits;

    *OutHandle = (ACPI_SEMAPHORE)sem;
    return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle)
{
    if (!Handle) return AE_BAD_PARAMETER;
    kfree(Handle);
    return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
    if (!Handle) return AE_BAD_PARAMETER;
    
    acpi_sem_t *sem = (acpi_sem_t *)Handle;
    
    if (sem->current_units >= Units) {
        sem->current_units -= Units;
        return AE_OK;
    }
    
    return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
    if (!Handle) return AE_BAD_PARAMETER;
    
    acpi_sem_t *sem = (acpi_sem_t *)Handle;
    sem->current_units += Units;
    
    return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void) {
    return 1;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context) { return AE_NOT_IMPLEMENTED; }
void AcpiOsSleep(UINT64 Milliseconds) {
    for(volatile uint64_t i = 0; i < Milliseconds * 100000; i++) { asm volatile("nop"); }
}
void AcpiOsStall(UINT32 Microseconds) {
    for(volatile uint32_t i = 0; i < Microseconds * 100; i++) { asm volatile("nop"); }
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewLength) {
    *NewAddress = 0;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
    *NewTable = NULL;
    return AE_OK;
}

int int_y = 0;

void AcpiOsPrintf(const char *Format, ...) {

    const char* real_error_message = NULL;

    asm volatile(
        "mov %%rsi, %[msg]"
        : [msg] "=r" (real_error_message)
        :
        :
    );

    if (real_error_message != NULL && ((uint64_t)real_error_message >= 0x700000000000U)) {
        print_text(real_error_message, 0, int_y, 255, 255, 255, 255);
        int_y += 15;
    } else {
        print_text(Format, 0, int_y, 255, 255, 255, 255);
        int_y += 15;
    }
    

}

void AcpiOsVprintf(const char *Format, va_list Args) {}

UINT64 AcpiOsGetTimer(void) { return 0; }

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) { return AE_OK; }

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine, void *Context) { return AE_OK; }
ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER ServiceRoutine) { return AE_OK; }

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) { return TRUE; }
BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) { return TRUE; }

void AcpiOsWaitEventsComplete(void) {
    // Тут чё-то будет потом
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width) {
    *Value = 0; return AE_OK;
}
ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width) {
    return AE_OK;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width) {
    uint64_t phys_page = (uint64_t)Address & ~0xFFF;
    uint64_t offset    = (uint64_t)Address & 0xFFF;
    
    uint64_t temp_virt = 0xFFFFFFFFD0000000; 
    map_page(temp_virt, phys_page, 0x3);
    
    volatile void* virt = (void*)(temp_virt + offset);

    if (Width == 8)  *Value = *(volatile uint8_t*)virt;
    if (Width == 16) *Value = *(volatile uint16_t*)virt;
    if (Width == 32) *Value = *(volatile uint32_t*)virt;
    if (Width == 64) *Value = *(volatile uint64_t*)virt;
    
    return AE_OK;
}
ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width) {
    uint64_t phys_page = (uint64_t)Address & ~0xFFF;
    uint64_t offset    = (uint64_t)Address & 0xFFF;
    
    uint64_t temp_virt = 0xFFFFFFFFD0000000;
    map_page(temp_virt, phys_page, 0x3);
    
    volatile void* virt = (void*)(temp_virt + offset);

    if (Width == 8)  *(volatile uint8_t*)virt = (uint8_t)Value;
    if (Width == 16) *(volatile uint16_t*)virt = (uint16_t)Value;
    if (Width == 32) *(volatile uint32_t*)virt = (uint32_t)Value;
    if (Width == 64) *(volatile uint64_t*)virt = Value;
    
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue) { return AE_OK; }
ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal, ACPI_STRING *NewVal) {
    *NewVal = NULL; 
    return AE_OK;
}

const unsigned short *fake_ctype_b = NULL;
const int *fake_ctype_tolower = NULL;
const int *fake_ctype_toupper = NULL;

const unsigned short **__ctype_b_loc(void) { return &fake_ctype_b; }
const int **__ctype_toupper_loc(void) { return &fake_ctype_toupper; }
const int **__ctype_tolower_loc(void) { return &fake_ctype_tolower; }

void AcpiDbSignalBreakPoint(void *WalkState) {
    return;
}

void AcpiDmDisassemble(void *WalkState, void *Op, UINT32 NumSubOps) {
    return;
}

void AcpiDbDumpMethodInfo(ACPI_STATUS Status, void *WalkState) {
    return;
}

void AcpiDbDisplayArgumentObject(void *Op, void *WalkState) {
    return;
}

void AcpiDbDisplayResultObject(void *ResultObj, void *WalkState) {
    return;
}

void AcpiDbSingleStep(void *WalkState, void *Op, UINT32 OpcodeClass) {
    return;
}