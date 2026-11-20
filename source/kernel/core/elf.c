#include "core.h"
#include "elf.h"

uint64_t load_elf_segments(uint8_t *elf_buffer, uint64_t* user_pt) {
    Elf64_Ehdr *elf_header = (Elf64_Ehdr *)elf_buffer;

    if (elf_header->e_ident[0] != 0x7F || elf_header->e_ident[1] != 'E' ||
        elf_header->e_ident[2] != 'L'  || elf_header->e_ident[3] != 'F') {
        return 0;
    }

    Elf64_Phdr *prog_header = (Elf64_Phdr *)(elf_buffer + elf_header->e_phoff);

    for (int i = 0; i < elf_header->e_phnum; i++) {
        if (prog_header[i].p_type != PT_LOAD) {
            continue;
        }

        uint64_t virt_start  = prog_header[i].p_vaddr;
        uint64_t mem_size    = prog_header[i].p_memsz;
        uint64_t file_size   = prog_header[i].p_filesz;
        uint64_t file_offset = prog_header[i].p_offset;
        uint32_t p_flags     = prog_header[i].p_flags;

        if (mem_size == 0) {
            continue;
        }

        uint64_t page_flags = PAGE_PRESENT | PAGE_USER;
        if (p_flags & PF_W) {
            page_flags |= PAGE_WRITABLE;
        }

        if (!(p_flags & PF_X && nxe_supported == 1)) {
            page_flags |= PAGE_NX;
        }

        uint64_t page_start = virt_start & ~0xFFFULL;
        uint64_t page_end   = (virt_start + mem_size + PAGE_SIZE - 1) & ~0xFFFULL;

        for (uint64_t v_addr = page_start; v_addr < page_end; v_addr += PAGE_SIZE) {
            uint64_t phys_addr = get_phys_from_pml4(user_pt, v_addr);
            uint64_t existing_flags = 0;

            if (phys_addr == 0) {
                void *new_page = (void*)alloc_page();
                if (!new_page) {
                    free_user_pml4(user_pt);
                    return 0;
                }
                phys_addr = (uint64_t)new_page;

                for (int b = 0; b < PAGE_SIZE; b++) {
                    ((uint8_t *)phys_addr)[b] = 0;
                }
            } else {
                existing_flags = PAGE_WRITABLE; 
            }

            if (!map_page_in_pml4(v_addr, phys_addr, page_flags | existing_flags, user_pt)) {
                free_user_pml4(user_pt);
                return 0;
            }

            uint64_t seg_file_end = virt_start + file_size;
            uint64_t page_next    = v_addr + PAGE_SIZE;

            uint64_t copy_start = (v_addr > virt_start) ? v_addr : virt_start;
            uint64_t copy_end   = (page_next < seg_file_end) ? page_next : seg_file_end;

            if (copy_start < copy_end) {
                uint64_t bytes_to_copy   = copy_end - copy_start;
                uint64_t offset_in_page  = copy_start - v_addr;
                uint64_t offset_in_file  = file_offset + (copy_start - virt_start);

                uint8_t *dest = (uint8_t *)(phys_addr + offset_in_page);
                uint8_t *src  = (uint8_t *)(elf_buffer + offset_in_file);

                for (uint64_t b = 0; b < bytes_to_copy; b++) {
                    dest[b] = src[b];
                }
            }
        }
    }

    return 1;
}