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
extern int      map_page(uint64_t virt, uint64_t phys, uint64_t flags);
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

    if (acpi_rsdp_addr != 0) {
        return (ACPI_PHYSICAL_ADDRESS)acpi_rsdp_addr;
    }
    
    return 0; 
}

void *AcpiOsAllocate(ACPI_SIZE Size)
{
    uint64_t clean_size = (uint64_t)Size; 
    void* ptr = NULL;

    if (clean_size == 0) return NULL;

    ptr = kmalloc(clean_size);

    return ptr;
}

void AcpiOsFree(void* Memory) {
    kfree(Memory);
}

#define ACPI_VIRT_BASE      0xFFFFFFFF90000000
#define PAGE_SIZE           4096

#define PAGE_FLAGS          0x03

void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
    uint64_t offset = PhysicalAddress % PAGE_SIZE;
    
    uint64_t phys_page = PhysicalAddress - offset;
    
    uint64_t total_length = Length + offset;
    uint64_t pages_count = (total_length + PAGE_SIZE - 1) / PAGE_SIZE;

    static uint64_t virtual_cursor = ACPI_VIRT_BASE;
    uint64_t virt_start = virtual_cursor;
    
    virtual_cursor += pages_count * PAGE_SIZE;

    for (uint64_t i = 0; i < pages_count; i++) {
        uint64_t v_addr = virt_start + (i * PAGE_SIZE);
        uint64_t p_addr = phys_page + (i * PAGE_SIZE);
        
        map_page(v_addr, p_addr, PAGE_FLAGS);
    }

    return (void *)(virt_start + offset);
}

void AcpiOsUnmapMemory(void* LogicalAddress, ACPI_SIZE Length) {

    uint64_t virt_addr = (uint64_t)LogicalAddress;
    uint64_t offset = virt_addr % PAGE_SIZE;
    uint64_t virt_page = virt_addr - offset;
    
    uint64_t total_length = Length + offset;
    uint64_t pages_count = (total_length + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = 0; i < pages_count; i++) {
        unmap_page(virt_page + (i * PAGE_SIZE));
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

    /*const char* real_error_message = NULL;

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
    }*/
    

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