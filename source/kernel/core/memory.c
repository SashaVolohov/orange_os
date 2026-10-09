#include <stdint.h>

#include "os_memory.h"

uintptr_t virtual_to_physical(uintptr_t pml4_phys_table, uintptr_t virt_addr) {
    uintptr_t offset = virt_addr & OFFSET_MASK;

    uintptr_t pml4_idx = (virt_addr >> 39) & INDEX_MASK;
    uintptr_t pdpt_idx = (virt_addr >> 30) & INDEX_MASK;
    uintptr_t pd_idx   = (virt_addr >> 21) & INDEX_MASK;
    uintptr_t pt_idx   = (virt_addr >> 12) & INDEX_MASK;

    uint64_t *pml4 = (uint64_t *)pml4_phys_table;

    uint64_t pml4e = pml4[pml4_idx];
    if (!(pml4e & PTE_PRESENT)) return 0;

    uintptr_t pdpt_phys = pml4e & PHYS_ADDR_MASK;
    uint64_t *pdpt = (uint64_t *)pdpt_phys;
    uint64_t pdpte = pdpt[pdpt_idx];
    if (!(pdpte & PTE_PRESENT)) return 0;

    if (pdpte & PTE_LARGE_PAGE) {
        uintptr_t page_offset = virt_addr & 0x3FFFFFFF;
        return (pdpte & 0x000FFFFFC0000000ULL) + page_offset;
    }

    uintptr_t pd_phys = pdpte & PHYS_ADDR_MASK;
    uint64_t *pd = (uint64_t *)pd_phys;
    uint64_t pde = pd[pd_idx];
    if (!(pde & PTE_PRESENT)) return 0;

    if (pde & PTE_LARGE_PAGE) {
        uintptr_t page_offset = virt_addr & 0x1FFFFF;
        return (pde & 0x000FFFFFFFE00000ULL) + page_offset;
    }

    uintptr_t pt_phys = pde & PHYS_ADDR_MASK;
    uint64_t *pt = (uint64_t *)pt_phys;
    uint64_t pte = pt[pt_idx];
    if (!(pte & PTE_PRESENT)) return 0;

    uintptr_t page_phys_base = pte & PHYS_ADDR_MASK;
    return page_phys_base + offset;
}