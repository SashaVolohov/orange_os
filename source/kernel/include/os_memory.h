#include <stdint.h>

#define PAGE_SHIFT        12
#define PAGE_SIZE         4096
#define INDEX_MASK        0x1FF
#define OFFSET_MASK       0xFFF

#define PHYS_ADDR_MASK    0x000FFFFFFFFFF000ULL

#define PTE_PRESENT       (1ULL << 0)
#define PTE_LARGE_PAGE    (1ULL << 7)

uintptr_t virtual_to_physical(uintptr_t pml4_phys_table, uintptr_t virt_addr);