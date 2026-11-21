#define KHEAP_START 0x700000000000ULL

#define MAX_PROCESSES 32
#define TASK_STATE_EMPTY   0
#define TASK_STATE_READY   1
#define TASK_STATE_RUNNING 2
#define TASK_STATE_SLEEP   3
#define TASK_STATE_ZOMBIE  4

#define TASK_STACK_LIMIT 0xFFFF800000000000ULL

#define MASTER_PIC_CMD  0x20
#define MASTER_PIC_DATA 0x21
#define SLAVE_PIC_CMD   0xA0
#define SLAVE_PIC_DATA  0xA1

typedef struct {
    unsigned long long rax;
    unsigned long long rbx;
    unsigned long long rcx;
    unsigned long long rdx;
    unsigned long long rsi;
    unsigned long long rdi;
    unsigned long long rbp;
    unsigned long long rsp;
    unsigned long long rip;
    unsigned long long rflags;
    unsigned long long cr3;
    unsigned int pid;
    unsigned int pad0;
    unsigned int state;
    unsigned int pad1;
    unsigned long long ticks;
} pcb_t;

typedef struct {
    pcb_t *process_table_ptr;
    unsigned long long kernel_main_rsp;
    unsigned int current_task_index;
    unsigned int total_processes;
} tasking_state_t;

__attribute__((noreturn))
void kernel_idle_task(void) {
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void* uefi_kmalloc(unsigned long long size, unsigned long long kheap_first_page_phys) {
    size = (size + 15ULL) & ~15ULL;

    unsigned long long *hdr = (unsigned long long*)kheap_first_page_phys;
    unsigned long long block_size = *(hdr + 0);
    unsigned long long is_free    = *(hdr + 1);

    if (is_free == 1 && block_size >= size + 16ULL) {
        unsigned long long rem_size = block_size - size - 16ULL;
        *(hdr + 0) = size;
        *(hdr + 1) = 0; 

        if (rem_size >= 16ULL) {
            unsigned long long *next_hdr = (unsigned long long*)(kheap_first_page_phys + 16ULL + size);
            *(next_hdr + 0) = rem_size;
            *(next_hdr + 1) = 1;
        }
        return (void*)(KHEAP_START + 16ULL);
    }
    return 0;
}


typedef unsigned long long efi_status_t;
typedef void* efi_handle_t;

struct efi_simple_text_output_protocol;

typedef efi_status_t (__attribute__((ms_abi)) *efi_text_string_t) (
    struct efi_simple_text_output_protocol *this,
    const unsigned short *string
);

typedef struct efi_simple_text_output_protocol {
    void* reset;
    efi_text_string_t output_string;
} efi_simple_text_output_protocol_t;

typedef struct {
    unsigned long long mb_memory;
    unsigned long long pt_begin;
    unsigned long long pt_end;
    unsigned long long ptv_end;
    unsigned long long bitmap_end;
    unsigned long long pml4_addr;
    unsigned long long kheap_first_page_phys;
    unsigned long long kheap_current_end;
} memory_layout_t;

typedef struct {
    unsigned long long signature;
    unsigned int revision;
    unsigned int header_size;
    unsigned int crc32;
    unsigned int reserved;
} efi_table_header_t;

typedef struct __attribute__((packed)) {
    unsigned int   e_ident_magic;
    unsigned char  e_ident_class;
    unsigned char  e_ident_data;
    unsigned char  e_ident_version;
    unsigned char  e_ident_osabi;
    unsigned char  e_ident_abiversion;
    unsigned char  pad0;
    unsigned char  pad1;
    unsigned char  pad2;
    unsigned char  pad3;
    unsigned char  pad4;
    unsigned char  pad5;
    unsigned char  pad6;
    unsigned short e_type;
    unsigned short e_machine;
    unsigned int   e_version;
    unsigned long long e_entry;
    unsigned long long e_phoff;
    unsigned long long e_shoff;
    unsigned int   e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
} elf64_ehdr_t;

typedef struct __attribute__((packed)) {
    unsigned int   p_type;
    unsigned int   p_flags;
    unsigned long long p_offset;
    unsigned long long p_vaddr;
    unsigned long long p_paddr;
    unsigned long long p_filesz;
    unsigned long long p_memsz;
    unsigned long long p_align;
} elf64_phdr_t;

typedef struct {
    unsigned int   data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char  data4[8];
} efi_guid_t;

typedef struct {
    unsigned long long entry_point;
    unsigned long long mb_memory;
    unsigned long long pt_begin;
    unsigned long long pt_end;
    unsigned long long ptv_end;
    unsigned long long bitmap_end;
    unsigned long long lfb_buffer_addr;
    unsigned long long lapic_base_addr;
    unsigned long long timer_ticks;
    unsigned long long process_table_ptr;
    unsigned long long kheap_current_end;
    unsigned long long acpi_rsdp_addr;
    unsigned long long boot_info_addr;
    unsigned long long kernel_rsp;
} kernel_handoff_t;

typedef struct {
    efi_table_header_t hdr;

    void* raise_tpl;
    void* restore_tpl;

    efi_status_t (__attribute__((ms_abi)) *allocate_pages)(int type, int memory_type, unsigned long long pages, unsigned long long *memory);
    efi_status_t (__attribute__((ms_abi)) *free_pages)(unsigned long long memory, unsigned long long pages);
    efi_status_t (__attribute__((ms_abi)) *get_memory_map)(unsigned long long *memory_map_size, void *memory_map, unsigned long long *map_key, unsigned long long *descriptor_size, unsigned int *descriptor_version);
    efi_status_t (__attribute__((ms_abi)) *allocate_pool)(int pool_type, unsigned long long size, void **buffer);
    efi_status_t (__attribute__((ms_abi)) *free_pool)(void *buffer);

    void* create_event;
    void* set_timer;
    void* wait_for_event;
    void* signal_event;
    void* close_event;
    void* check_event;

    void* install_protocol_interface;
    void* reinstall_protocol_interface;
    void* uninstall_protocol_interface;
    efi_status_t (__attribute__((ms_abi)) *handle_protocol)(efi_handle_t handle, efi_guid_t *protocol, void **interface);
    void* reserved;
    void* register_protocol_notify;
    void* locate_handle;
    void* locate_device_path;
    void* install_configuration_table;

    void* load_image;
    void* start_image;
    void* exit;
    void* unload_image;
    efi_status_t (__attribute__((ms_abi)) *exit_boot_services)(efi_handle_t image_handle, unsigned long long map_key);

    void* get_next_monotonic_count;
    void* stall;
    efi_status_t (__attribute__((ms_abi)) *set_watchdog_timer)(unsigned long long timeout, unsigned long long watchdog_code, unsigned long long data_size, unsigned short *watchdog_data);
    void* connect_controller;
    void* disconnect_controller;
    void* open_protocol;
    void* close_protocol;
    void* open_protocol_information;
    void* protocols_per_handle;
    efi_status_t (__attribute__((ms_abi)) *locate_handle_buffer)(
        int search_type, efi_guid_t *protocol, void *search_key,
        unsigned long long *no_handles, efi_handle_t **buffer
    );

    efi_status_t (__attribute__((ms_abi)) *locate_protocol)(efi_guid_t *protocol, void *registration, void **interface);
} efi_boot_services_t;

typedef struct {
    efi_guid_t vendor_guid;
    void *vendor_table;
} efi_configuration_table_t;

typedef struct __attribute__((packed)) {
    unsigned short size;
    unsigned short flags;
    unsigned int   cylinders;
    unsigned int   heads;
    unsigned int   sectors_per_track;
    unsigned long long total_sectors;
    unsigned short bytes_per_sector;
    
    unsigned int   edd_cfg_ptr;
    unsigned short host_bus_type;
    unsigned long long interface_type;
    unsigned long long interface_path;
    unsigned long long device_path;
    unsigned char  reserved;
    unsigned char  checksum;
} edd_drive_params_t;

typedef struct __attribute__((packed)) {
    unsigned short vbe_hor_size;
    unsigned short vbe_ver_size;
    unsigned char  vbe_color_depth;
    unsigned short vbe_pitch;
    unsigned char  loading_drive;
    unsigned int   lfb_buffer_addr;
    edd_drive_params_t drive_params;
    unsigned char reserved[14];
    unsigned int   boot_pvd_hash;
    unsigned int   cpu_count;
    unsigned char  cpus_ids[256];
    unsigned long long bsp_factory_ist1;
    unsigned char  is_setup;
    unsigned int   vbe_modes_count;
    void*          vbe_modes_list;
} boot_info_t;

typedef struct {
    efi_table_header_t hdr;
    const unsigned short* firmware_vendor;
    unsigned int firmware_revision;
    efi_handle_t console_in_handle;
    void* con_in;
    efi_handle_t console_out_handle;
    efi_simple_text_output_protocol_t *con_out;
    efi_handle_t standard_error_handle;
    void* std_err;
    void* runtime_services;
    efi_boot_services_t *boot_services;
    unsigned long long number_of_table_entries;
    efi_configuration_table_t *configuration_table;
} efi_system_table_t;

typedef struct {
    unsigned int revision;
    efi_handle_t parent_handle;
    efi_system_table_t *system_table;
    efi_handle_t device_handle;
    void *file_path;
    void *reserved;
    unsigned int load_options_size;
    void *load_options;
    void *image_base;
    unsigned long long image_size;
    int image_code_type;
    int image_data_type;
    void *unload;
} efi_loaded_image_protocol_t;

struct efi_file_protocol;

typedef struct efi_file_protocol {
    unsigned long long revision;
    efi_status_t (__attribute__((ms_abi)) *open)(
        struct efi_file_protocol *this,
        struct efi_file_protocol **new_handle,
        const unsigned short *file_name,
        unsigned long long open_mode,
        unsigned long long attributes
    );
    efi_status_t (__attribute__((ms_abi)) *close)(struct efi_file_protocol *this);
    void* delete_file;
    efi_status_t (__attribute__((ms_abi)) *read)(
        struct efi_file_protocol *this,
        unsigned long long *buffer_size,
        void *buffer
    );
    void* write;
    efi_status_t (__attribute__((ms_abi)) *get_position)(
        struct efi_file_protocol *this, unsigned long long *position
    );
    efi_status_t (__attribute__((ms_abi)) *set_position)(
        struct efi_file_protocol *this, unsigned long long position
    );
    efi_status_t (__attribute__((ms_abi)) *get_info)(
        struct efi_file_protocol *this,
        efi_guid_t *information_type,
        unsigned long long *buffer_size,
        void *buffer
    );
} efi_file_protocol_t;

typedef struct efi_simple_file_system_protocol {
    unsigned long long revision;
    efi_status_t (__attribute__((ms_abi)) *open_volume)(
        struct efi_simple_file_system_protocol *this,
        efi_file_protocol_t **root
    );
} efi_simple_file_system_protocol_t;

typedef struct {
    unsigned int media_id;
    unsigned char removable_media;
    unsigned char media_present;
    unsigned char logical_partition;
    unsigned char read_only;
    unsigned char write_caching;
    unsigned int block_size;
    unsigned int io_align;
    unsigned long long last_block;
} efi_block_io_media_t;

typedef struct efi_block_io_protocol {
    unsigned long long revision;
    efi_block_io_media_t *media;
    void* reset;
    efi_status_t (__attribute__((ms_abi)) *read_blocks)(
        struct efi_block_io_protocol *this,
        unsigned int media_id,
        unsigned long long lba,
        unsigned long long buffer_size,
        void *buffer
    );
    void* write_blocks;
    void* flush_blocks;
} efi_block_io_protocol_t;

typedef struct __attribute__((packed)) {
    unsigned int   reserved0;
    unsigned long long rsp0;
    unsigned long long rsp1;
    unsigned long long rsp2;
    unsigned long long reserved1;
    unsigned long long ist1;
    unsigned long long ist2;
    unsigned long long ist3;
    unsigned long long ist4;
    unsigned long long ist5;
    unsigned long long ist6;
    unsigned long long ist7;
    unsigned long long reserved2;
    unsigned short reserved3;
    unsigned short iomap_base;
} tss64_t;

typedef struct __attribute__((packed)) {
    unsigned char type;
    unsigned char sub_type;
    unsigned short length;
} efi_device_path_protocol_t;

typedef struct __attribute__((packed)) {
    unsigned short limit;
    unsigned long long base;
} gdtr_t;

typedef struct {
    unsigned long long gdt_phys_addr;
    unsigned long long tss_phys_addr;
    unsigned long long bsp_factory_ist1;
    gdtr_t gdtr;
} gdt_tss_state_t;

typedef struct {
    unsigned int red_mask;
    unsigned int green_mask;
    unsigned int blue_mask;
    unsigned int reserved_mask;
} efi_pixel_bitmask_t;

typedef struct {
    unsigned int version;
    unsigned int horizontal_resolution;
    unsigned int vertical_resolution;
    int pixel_format;
    efi_pixel_bitmask_t pixel_information;
    unsigned int pixels_per_scan_line;
} efi_graphics_output_mode_information_t;

typedef struct {
    unsigned int max_mode;
    unsigned int mode;
    efi_graphics_output_mode_information_t *info;
    unsigned long long size_of_info;
    unsigned long long frame_buffer_base;
    unsigned long long frame_buffer_size;
} efi_graphics_output_protocol_mode_t;

typedef struct {
    efi_status_t (__attribute__((ms_abi)) *query_mode)(void *this, unsigned int mode_number, unsigned long long *size_of_info, efi_graphics_output_mode_information_t **info);
    efi_status_t (__attribute__((ms_abi)) *set_mode)(void *this, unsigned int mode_number);
    void* blt;
    efi_graphics_output_protocol_mode_t *mode;
} efi_graphics_output_protocol_t;

typedef struct __attribute__((packed)) {
    unsigned long long base;
    unsigned long long length;
    unsigned int type;
    unsigned int acpi_ext;
} e820_entry_t;

typedef struct {
    unsigned int type;
    unsigned long long physical_start;
    unsigned long long virtual_start;
    unsigned long long number_of_pages;
    unsigned long long attribute;
} efi_memory_descriptor_t;

void* memset(void* dst, int val, unsigned long long count) {
    unsigned char* ptr = (unsigned char*)dst;
    while (count--) {
        *ptr++ = (unsigned char)val;
    }
    return dst;
}

void* memcpy(void* dst, const void* src, unsigned long long count) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dst;
}

void uefi_mark_page_used(unsigned long long phys_addr, memory_layout_t *layout) {
    unsigned long long page_idx = phys_addr >> 12;
    unsigned long long max_pages = layout->mb_memory * 256ULL;
    if (page_idx >= max_pages) {
        return;
    }

    unsigned long long qword_idx = page_idx >> 6;
    unsigned long long bit_idx   = page_idx & 63;

    unsigned long long *bitmap = (unsigned long long*)layout->ptv_end;
    *(bitmap + qword_idx) |= (1ULL << bit_idx);
}

void protect_system_regions(memory_layout_t *layout) {
    unsigned long long addr = 0;
    while (addr < 0x100000ULL) {
        uefi_mark_page_used(addr, layout);
        addr += 4096ULL;
    }

    unsigned long long start = layout->pt_begin & ~0xFFFULL;
    unsigned long long end   = (layout->bitmap_end + 0xFFFULL) & ~0xFFFULL;

    addr = start;
    while (addr < end) {
        uefi_mark_page_used(addr, layout);
        addr += 4096ULL;
    }
}

unsigned int uefi_type_to_e820(unsigned int uefi_type) {
    switch (uefi_type) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 7:
            return 1;
        case 9:
            return 3;
        case 10:
            return 4;
        default:
            return 2;
    }
}

efi_status_t build_memory_video_and_allocator(
    efi_boot_services_t *bs,
    e820_entry_t *e820_map,
    unsigned int e820_count,
    unsigned long long lfb_buffer_addr,
    memory_layout_t *out,
    unsigned long long lfb_size
) {
    unsigned long long total_bytes = 0;
    unsigned int i = 0;
    while (i < e820_count) {
        e820_entry_t *entry = e820_map + i;
        if (entry->type == 1) {
            total_bytes += entry->length;
        }
        i++;
    }

    unsigned long long mb_memory = (total_bytes + 1048575ULL) / 1048576ULL;
    unsigned long long map_mb = mb_memory;

    unsigned long long pt_count = (map_mb + 1) / 2;
    unsigned long long pd_count = (pt_count + 511) / 512;
    if (pd_count == 0) {
        pd_count = 1;
    }

    unsigned long long base_pt_pages = 1 + 1 + pd_count + pt_count;

    unsigned long long lfb_pt_count = (lfb_size + 0x1FFFFFULL) / 0x200000ULL;
    if (lfb_pt_count < 2) {
        lfb_pt_count = 2;
    }

    unsigned long long video_reserve_pages = 2 + lfb_pt_count;

    unsigned long long bitmap_qwords = mb_memory * 4;
    unsigned long long bitmap_bytes = bitmap_qwords * 8;
    unsigned long long bitmap_pages = (bitmap_bytes + 4095ULL) / 4096ULL;

    unsigned long long total_pages = base_pt_pages + video_reserve_pages + bitmap_pages;

    unsigned long long pt_begin = 0;
    efi_status_t status = bs->allocate_pages(0, 2, total_pages, &pt_begin);
    if (status != 0) {
        return status;
    }

    unsigned long long *clear_ptr = (unsigned long long*)pt_begin;
    unsigned long long idx = 0;
    while (idx < total_pages * 512) {
        *(clear_ptr + idx) = 0;
        idx++;
    }

    unsigned long long ram_flags = 0x7;
    unsigned long long *pml4 = (unsigned long long*)pt_begin;
    unsigned long long *pdpt = (unsigned long long*)(pt_begin + 0x1000);
    unsigned long long *pd_base = (unsigned long long*)(pt_begin + 0x2000);
    unsigned long long *pt_base = (unsigned long long*)(pt_begin + (2 + pd_count) * 0x1000);

    *(pml4 + 0) = ((unsigned long long)pdpt) | ram_flags;

    idx = 0;
    while (idx < pd_count) {
        *(pdpt + idx) = ((unsigned long long)pd_base + idx * 0x1000) | ram_flags;
        idx++;
    }

    idx = 0;
    while (idx < pt_count) {
        *(pd_base + idx) = ((unsigned long long)pt_base + idx * 0x1000) | ram_flags;
        idx++;
    }

    idx = 0;
    while (idx < pt_count * 512) {
        *(pt_base + idx) = (idx * 0x1000ULL) | ram_flags;
        idx++;
    }

    unsigned long long cur_end = pt_begin + base_pt_pages * 0x1000;

    unsigned long long lfb_flags = 0x1F;
    unsigned long long lfb_aligned = lfb_buffer_addr & ~0x1FFFFFULL;

    unsigned long long pml4_idx = (lfb_aligned >> 39) & 0x1FF;
    unsigned long long pdpt_idx = (lfb_aligned >> 30) & 0x1FF;
    unsigned long long pd_idx   = (lfb_aligned >> 21) & 0x1FF;

    unsigned long long *target_pdpt = pdpt;
    if (pml4_idx != 0) {
        if ((*(pml4 + pml4_idx) & 1) == 0) {
            *(pml4 + pml4_idx) = cur_end | lfb_flags;
            target_pdpt = (unsigned long long*)cur_end;
            cur_end += 0x1000;
        } else {
            target_pdpt = (unsigned long long*)(*(pml4 + pml4_idx) & 0x000FFFFFFFFFF000ULL);
        }
    }

    unsigned long long *target_pd = 0;
    if ((*(target_pdpt + pdpt_idx) & 1) == 0) {
        *(target_pdpt + pdpt_idx) = cur_end | lfb_flags;
        target_pd = (unsigned long long*)cur_end;
        cur_end += 0x1000;
    } else {
        target_pd = (unsigned long long*)(*(target_pdpt + pdpt_idx) & 0x000FFFFFFFFFF000ULL);
    }

    unsigned long long *lfb_pt = (unsigned long long*)cur_end;
    *(target_pd + pd_idx)     = cur_end | lfb_flags;
    *(target_pd + pd_idx + 1) = (cur_end + 0x1000) | lfb_flags;

    idx = 0;
    while (idx < 1024) {
        *(lfb_pt + idx) = (lfb_aligned + idx * 0x1000ULL) | lfb_flags;
        idx++;
    }

    cur_end += 0x2000;

    unsigned long long ptv_end = cur_end;
    unsigned long long *bitmap_ptr = (unsigned long long*)ptv_end;

    idx = 0;
    while (idx < bitmap_qwords) {
        *(bitmap_ptr + idx) = 0xFFFFFFFFFFFFFFFFULL;
        idx++;
    }

    unsigned long long bitmap_end = ptv_end + bitmap_bytes;

    out->mb_memory  = mb_memory;
    out->pt_begin   = pt_begin;
    out->pt_end     = ptv_end;
    out->ptv_end    = ptv_end;
    out->bitmap_end = bitmap_end;
    out->pml4_addr  = pt_begin;

    return 0;
}

efi_status_t prepare_uefi_memory(
    efi_boot_services_t *bs,
    unsigned long long fb_base,
    unsigned long long fb_size,
    e820_entry_t **out_e820_map,
    unsigned int *out_e820_count,
    memory_layout_t *out_layout
) {
    unsigned long long map_size = 0;
    unsigned long long map_key = 0;
    unsigned long long desc_size = 0;
    unsigned int desc_version = 0;

    bs->get_memory_map(&map_size, 0, &map_key, &desc_size, &desc_version);
    map_size += desc_size * 8;

    void *uefi_map = 0;
    efi_status_t status = bs->allocate_pool(2, map_size, &uefi_map);
    if (status != 0) return status;

    status = bs->get_memory_map(&map_size, uefi_map, &map_key, &desc_size, &desc_version);
    if (status != 0) return status;

    unsigned int entry_count = (unsigned int)(map_size / desc_size);

    e820_entry_t *e820_map = 0;
    status = bs->allocate_pool(2, entry_count * sizeof(e820_entry_t), (void**)&e820_map);
    if (status != 0) return status;

    unsigned int e820_idx = 0;
    for (unsigned int i = 0; i < entry_count; i++) {
        efi_memory_descriptor_t *desc = (efi_memory_descriptor_t*)((unsigned char*)uefi_map + i * desc_size);
        unsigned int e820_type = uefi_type_to_e820(desc->type);
        unsigned long long length = desc->number_of_pages * 4096ULL;

        if (e820_idx > 0 &&
            e820_map[e820_idx - 1].type == e820_type &&
            e820_map[e820_idx - 1].base + e820_map[e820_idx - 1].length == desc->physical_start) {
            e820_map[e820_idx - 1].length += length;
        } else {
            e820_map[e820_idx].base = desc->physical_start;
            e820_map[e820_idx].length = length;
            e820_map[e820_idx].type = e820_type;
            e820_map[e820_idx].acpi_ext = 1;
            e820_idx++;
        }
    }

    bs->free_pool(uefi_map);

    *out_e820_map = e820_map;
    *out_e820_count = e820_idx;

    return build_memory_video_and_allocator(bs, e820_map, e820_idx, fb_base, out_layout, fb_size);
}

void print_hex_digit(efi_simple_text_output_protocol_t* con_out, unsigned char val) {
    unsigned short str[2];
    if (val < 10) {
        str[0] = (unsigned short)('0' + val);
    } else {
        str[0] = (unsigned short)('A' + (val - 10));
    }
    str[1] = 0;
    con_out->output_string(con_out, str);
}

void print_status(efi_simple_text_output_protocol_t* con_out, efi_status_t status) {
    con_out->output_string(con_out, L"Status: 0x");
    for (int i = 15; i >= 0; i--) {
        unsigned char digit = (status >> (i * 4)) & 0xF;
        print_hex_digit(con_out, digit);
    }
    con_out->output_string(con_out, L"\r\n");
}

unsigned long long uefi_alloc_page(memory_layout_t *layout) {
    unsigned long long max_pages = layout->mb_memory * 256ULL;
    unsigned long long max_qwords = (max_pages + 63ULL) >> 6;
    unsigned long long *bitmap = (unsigned long long*)layout->ptv_end;

    unsigned long long q_idx = 4;
    while (q_idx != max_qwords) {
        unsigned long long val = *(bitmap + q_idx);
        if (val != 0xFFFFFFFFFFFFFFFFULL) {
            unsigned int bit = 0;
            while (bit != 64) {
                if ((val & (1ULL << bit)) == 0) {
                    unsigned long long page_idx = (q_idx << 6) + bit;
                    if (page_idx >= max_pages) {
                        return 0;
                    }
                    *(bitmap + q_idx) |= (1ULL << bit);
                    unsigned long long phys_addr = page_idx << 12;

                    unsigned long long *p = (unsigned long long*)phys_addr;
                    unsigned int z = 0;
                    while (z != 512) {
                        *(p + z) = 0;
                        z++;
                    }
                    return phys_addr;
                }
                bit++;
            }
        }
        q_idx++;
    }
    return 0;
}

void uefi_map_page(
    unsigned long long virt,
    unsigned long long phys,
    unsigned long long flags,
    memory_layout_t *layout
) {
    unsigned long long pml4_idx = (virt >> 39) & 0x1FF;
    unsigned long long pdpt_idx = (virt >> 30) & 0x1FF;
    unsigned long long pd_idx   = (virt >> 21) & 0x1FF;
    unsigned long long pt_idx   = (virt >> 12) & 0x1FF;

    unsigned long long *pml4 = (unsigned long long*)layout->pml4_addr;
    unsigned long long addr_mask = 0x000FFFFFFFFFF000ULL;

    unsigned long long *pdpt = 0;
    if ((*(pml4 + pml4_idx) & 1) == 0) {
        unsigned long long new_pdpt = uefi_alloc_page(layout);
        *(pml4 + pml4_idx) = new_pdpt | flags | 0x3;
        pdpt = (unsigned long long*)new_pdpt;
    } else {
        pdpt = (unsigned long long*)(*(pml4 + pml4_idx) & addr_mask);
    }

    unsigned long long *pd = 0;
    if ((*(pdpt + pdpt_idx) & 1) == 0) {
        unsigned long long new_pd = uefi_alloc_page(layout);
        *(pdpt + pdpt_idx) = new_pd | flags | 0x3;
        pd = (unsigned long long*)new_pd;
    } else {
        pd = (unsigned long long*)(*(pdpt + pdpt_idx) & addr_mask);
    }

    unsigned long long *pt = 0;
    if ((*(pd + pd_idx) & 1) == 0) {
        unsigned long long new_pt = uefi_alloc_page(layout);
        *(pd + pd_idx) = new_pt | flags | 0x3;
        pt = (unsigned long long*)new_pt;
    } else {
        pt = (unsigned long long*)(*(pd + pd_idx) & addr_mask);
    }

    *(pt + pt_idx) = (phys & addr_mask) | flags;
}

void load_gdt_and_tss(gdt_tss_state_t *state) {
    __asm__ __volatile__(
        "lgdt (%0)\n\t"
        "movw $0x28, %%ax\n\t"
        "ltr %%ax\n\t"
        "movw $0x10, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        "movw %%ax, %%ss\n\t"
        "pushq $0x08\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "pushq %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        :
        : "r"(&state->gdtr)
        : "rax", "memory"
    );
}

void kheap_init(memory_layout_t *layout) {
    unsigned long long virt = KHEAP_START;
    unsigned long long first_page_phys = 0;

    unsigned int i = 0;
    while (i != 4) {
        unsigned long long phys = uefi_alloc_page(layout);
        if (i == 0) {
            first_page_phys = phys;
        }
        uefi_map_page(virt, phys, 0x3, layout);
        virt += 4096ULL;
        i++;
    }

    unsigned long long *heap_hdr = (unsigned long long*)first_page_phys;
    *(heap_hdr + 0) = 16368ULL;
    *(heap_hdr + 1) = 1ULL;

    layout->kheap_first_page_phys = first_page_phys;
    layout->kheap_current_end     = virt;
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

void uefi_free_page(unsigned long long phys_addr, memory_layout_t *layout) {
    if (phys_addr == 0) {
        return;
    }
    if (phys_addr >= layout->pt_begin && phys_addr <= layout->bitmap_end) {
        return;
    }

    unsigned long long page_idx = phys_addr >> 12; // номер 4КБ страницы
    unsigned long long max_pages = layout->mb_memory * 256ULL;
    if (page_idx >= max_pages) {
        return;
    }

    unsigned long long qword_idx = page_idx >> 6;
    unsigned long long bit_idx   = page_idx & 63;

    unsigned long long *bitmap = (unsigned long long*)layout->ptv_end;
    *(bitmap + qword_idx) &= ~(1ULL << bit_idx);
}

void mark_usable_memory_free(
    e820_entry_t *e820_map,
    unsigned int e820_count,
    memory_layout_t *layout
) {
    unsigned int i = 0;
    while (i < e820_count) {
        e820_entry_t *entry = e820_map + i;
        if (entry->type == 1 && entry->length != 0) {
            unsigned long long start = (entry->base + 0xFFFULL) & ~0xFFFULL;
            unsigned long long end   = entry->base + entry->length;

            unsigned long long cur = start;
            while (cur + 4096ULL <= end) {
                uefi_free_page(cur, layout);
                cur += 4096ULL;
            }
        }
        i++;
    }
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

void uefi_tasking_init(
    memory_layout_t *layout,
    unsigned long long kheap_first_page_phys,
    tasking_state_t *out_tasking
) {
    unsigned long long table_size = MAX_PROCESSES * sizeof(pcb_t); // 32 * 112 = 3584 байта

    pcb_t *proc_table_virt = (pcb_t*)uefi_kmalloc(table_size, kheap_first_page_phys);
    pcb_t *proc_table_phys = (pcb_t*)(kheap_first_page_phys + 16ULL);

    unsigned long long *clear_ptr = (unsigned long long*)proc_table_phys;
    unsigned long long qwords = table_size / 8;
    unsigned long long i = 0;
    while (i != qwords) {
        *(clear_ptr + i) = 0;
        i++;
    }

    pcb_t *idle_pcb = proc_table_phys + 0;
    idle_pcb->pid    = 0;
    idle_pcb->state  = TASK_STATE_READY;
    idle_pcb->cr3    = layout->pml4_addr;
    idle_pcb->rflags = 0x200;

    unsigned long long virt_idle = TASK_STACK_LIMIT;
    unsigned long long last_idle_phys = 0;
    i = 0;
    while (i != 4) {
        unsigned long long phys = uefi_alloc_page(layout);
        if (i == 3) {
            last_idle_phys = phys;
        }
        uefi_map_page(virt_idle, phys, 0x7 /* 111b */, layout);
        virt_idle += 4096ULL;
        i++;
    }

    unsigned long long virt_top = (virt_idle - 8ULL) & ~0xFULL;
    unsigned long long offset_in_page = virt_top & 0xFFFULL;
    unsigned long long *stack_phys = (unsigned long long*)(last_idle_phys + offset_in_page);

    *(stack_phys - 0) = 0x10;                          // SS
    *(stack_phys - 1) = virt_top;                      // RSP
    *(stack_phys - 2) = 0x200;                         // RFLAGS
    *(stack_phys - 3) = 0x08;                          // CS
    *(stack_phys - 4) = (unsigned long long)kernel_idle_task; // RIP

    unsigned int r = 0;
    while (r != 15) {
        *(stack_phys - 5 - r) = 0;
        r++;
    }

    idle_pcb->rsp = virt_top - (19ULL * 8ULL);

    pcb_t *kern_pcb = proc_table_phys + 1;
    kern_pcb->pid    = 1;
    kern_pcb->state  = TASK_STATE_RUNNING;
    kern_pcb->cr3    = layout->pml4_addr;
    kern_pcb->rflags = 0x200;

    unsigned long long virt_kern = TASK_STACK_LIMIT + (16ULL * 4096ULL);
    i = 0;
    while (i != 4) {
        unsigned long long phys = uefi_alloc_page(layout);
        uefi_map_page(virt_kern, phys, 0x7, layout);
        virt_kern += 4096ULL;
        i++;
    }

    out_tasking->process_table_ptr  = proc_table_virt;
    out_tasking->kernel_main_rsp    = (virt_kern - 8ULL) & ~0xFULL;
    out_tasking->current_task_index = 1;
    out_tasking->total_processes    = 2;
}

void uefi_tss_gdt_init(
    memory_layout_t *layout,
    unsigned long long kernel_rsp0,
    gdt_tss_state_t *out_state
) {
    unsigned long long page_phys = uefi_alloc_page(layout);

    unsigned long long *gdt = (unsigned long long*)page_phys;
    *(gdt + 0) = 0x0000000000000000ULL;
    *(gdt + 1) = 0x00AF9A000000FFFFULL;
    *(gdt + 2) = 0x00AF92000000FFFFULL;
    *(gdt + 3) = 0x00AFFA000000FFFFULL;
    *(gdt + 4) = 0x00AFF2000000FFFFULL;
    *(gdt + 5) = 0x0000000000000000ULL;
    *(gdt + 6) = 0x0000000000000000ULL; 

    unsigned long long tss_addr = page_phys + 0x100ULL;
    tss64_t *tss0 = (tss64_t*)tss_addr;

    unsigned long long ist1_page = uefi_alloc_page(layout);
    unsigned long long ist1_top  = (ist1_page + 4096ULL) & ~0xFULL;

    tss0->rsp0       = kernel_rsp0;
    tss0->ist1       = ist1_top;
    tss0->iomap_base = 104;

    unsigned long long tss_limit = 103ULL;
    unsigned long long desc_low  = (tss_limit & 0xFFFFULL)
                                 | ((tss_addr & 0xFFFFFFULL) << 16)
                                 | (0x89ULL << 40)
                                 | (((tss_addr >> 24) & 0xFFULL) << 56);
    unsigned long long desc_high = (tss_addr >> 32) & 0xFFFFFFFFULL;

    *(gdt + 5) = desc_low;
    *(gdt + 6) = desc_high;

    out_state->gdt_phys_addr    = page_phys;
    out_state->tss_phys_addr    = tss_addr;
    out_state->bsp_factory_ist1 = ist1_top;
    out_state->gdtr.limit       = (7 * 8) - 1;
    out_state->gdtr.base        = page_phys;
}

efi_status_t try_read_from_handle(
    efi_boot_services_t *bs,
    efi_handle_t device_handle,
    efi_guid_t *fs_guid,
    const unsigned short *filename,
    unsigned long long *out_buffer_phys,
    unsigned long long *out_file_size
) {
    efi_simple_file_system_protocol_t *fs = 0;
    efi_status_t status = bs->handle_protocol(device_handle, fs_guid, (void**)&fs);
    if (status != 0 || fs == 0) {
        return 1;
    }

    efi_file_protocol_t *root = 0;
    status = fs->open_volume(fs, &root);
    if (status != 0 || root == 0) {
        return 2;
    }

    efi_file_protocol_t *file = 0;
    status = root->open(root, &file, filename, 1, 0);
    if (status != 0 || file == 0) {
        root->close(root);
        return 3;
    }

    unsigned long long file_size = 0;
    file->set_position(file, 0xFFFFFFFFFFFFFFFFULL);
    file->get_position(file, &file_size);
    file->set_position(file, 0);

    if (file_size == 0) {
        file->close(file);
        root->close(root);
        return 4;
    }

    unsigned long long pages = (file_size + 4095ULL) / 4096ULL;
    unsigned long long buffer_phys = 0;
    status = bs->allocate_pages(0, 2 /* EfiLoaderData */, pages, &buffer_phys);
    if (status != 0) {
        file->close(file);
        root->close(root);
        return status;
    }

    unsigned long long read_size = file_size;
    status = file->read(file, &read_size, (void*)buffer_phys);

    file->close(file);
    root->close(root);

    if (status != 0 || read_size != file_size) {
        return 5;
    }

    *out_buffer_phys = buffer_phys;
    *out_file_size   = file_size;
    return 0;
}

typedef struct __attribute__((packed)) {
    char signature[8];
    unsigned char checksum;
    char oem_id[6];
    unsigned char revision;
    unsigned int rsdt_address;
    unsigned int length;
    unsigned long long xsdt_address;
    unsigned char extended_checksum;
    unsigned char reserved[3];
} acpi_rsdp_t;

typedef struct __attribute__((packed)) {
    unsigned int signature;
    unsigned int length;
    unsigned char revision;
    unsigned char checksum;
    char oem_id[6];
    char oem_table_id[8];
    unsigned int oem_revision;
    unsigned int creator_id;
    unsigned int creator_rev;
} acpi_sdt_header_t;

typedef struct {
    unsigned long long acpi_rsdp_addr;
    unsigned long long acpi_rsdt_addr;
    unsigned long long madt_addr;
    unsigned long long lapic_base_addr;
    unsigned long long io_apic_base_addr;
    unsigned int io_apic_gsi_base;
    unsigned int cpu_count;
    unsigned char cpus_temp_array[256];
} acpi_info_t;

int guid_equals(efi_guid_t *a, efi_guid_t *b) {
    if (a->data1 != b->data1 || a->data2 != b->data2 || a->data3 != b->data3) {
        return 0;
    }
    unsigned int i = 0;
    while (i != 8) {
        if (*(a->data4 + i) != *(b->data4 + i)) {
            return 0;
        }
        i++;
    }
    return 1;
}

void map_acpi_table(unsigned long long table_addr, memory_layout_t *layout) {
    unsigned long long first_page = table_addr & ~0xFFFULL;
    uefi_map_page(first_page, first_page, 0x13, layout);

    acpi_sdt_header_t *hdr = (acpi_sdt_header_t*)table_addr;
    unsigned long long end_addr = table_addr + (unsigned long long)hdr->length;

    unsigned long long cur_page = first_page + 4096ULL;
    while (cur_page <= end_addr) {
        uefi_map_page(cur_page, cur_page, 0x13, layout);
        cur_page += 4096ULL;
    }
}

efi_status_t uefi_init_acpi(
    efi_system_table_t *st,
    memory_layout_t *layout,
    acpi_info_t *out
) {
    efi_guid_t acpi2_guid = {
        0x8868e871, 0xe4f1, 0x11d3,
        { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 }
    };
    efi_guid_t acpi1_guid = {
        0xeb9d2d30, 0x2d88, 0x11d3,
        { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d }
    };

    acpi_rsdp_t *rsdp = 0;
    unsigned long long i = 0;

    while (i != st->number_of_table_entries) {
        efi_configuration_table_t *entry = st->configuration_table + i;
        if (guid_equals(&entry->vendor_guid, &acpi2_guid)) {
            rsdp = (acpi_rsdp_t*)entry->vendor_table;
            break;
        }
        if (guid_equals(&entry->vendor_guid, &acpi1_guid) && rsdp == 0) {
            rsdp = (acpi_rsdp_t*)entry->vendor_table;
        }
        i++;
    }

    if (rsdp == 0) {
        return 1;
    }

    unsigned long long rsdp_phys = (unsigned long long)rsdp;
    uefi_map_page(rsdp_phys & ~0xFFFULL, rsdp_phys & ~0xFFFULL, 0x13, layout);
    out->acpi_rsdp_addr = rsdp_phys;

    int use_xsdt = 0;
    unsigned long long root_sdt_addr = 0;
    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
        use_xsdt = 1;
        root_sdt_addr = rsdp->xsdt_address;
    } else {
        root_sdt_addr = (unsigned long long)rsdp->rsdt_address;
    }

    if (root_sdt_addr == 0) {
        return 2;
    }

    map_acpi_table(root_sdt_addr, layout);
    out->acpi_rsdt_addr = root_sdt_addr;

    acpi_sdt_header_t *root_hdr = (acpi_sdt_header_t*)root_sdt_addr;
    unsigned int entry_size = use_xsdt ? 8 : 4;
    unsigned int entries_count = (root_hdr->length - sizeof(acpi_sdt_header_t)) / entry_size;
    unsigned char *entries_base = (unsigned char*)(root_sdt_addr + sizeof(acpi_sdt_header_t));

    unsigned long long madt_phys = 0;
    unsigned int idx = 0;
    while (idx != entries_count) {
        unsigned long long tbl_addr = 0;
        if (use_xsdt) {
            tbl_addr = *(unsigned long long*)(entries_base + idx * 8);
        } else {
            tbl_addr = (unsigned long long)(*(unsigned int*)(entries_base + idx * 4));
        }

        if (tbl_addr != 0) {
            uefi_map_page(tbl_addr & ~0xFFFULL, tbl_addr & ~0xFFFULL, 0x13, layout);
            acpi_sdt_header_t *tbl = (acpi_sdt_header_t*)tbl_addr;
            if (tbl->signature == 0x43495041U) {
                madt_phys = tbl_addr;
                break;
            }
        }
        idx++;
    }

    if (madt_phys == 0) {
        return 3;
    }

    map_acpi_table(madt_phys, layout);
    out->madt_addr = madt_phys;

    unsigned int lapic_addr32 = *(unsigned int*)(madt_phys + 36);
    out->lapic_base_addr = (unsigned long long)lapic_addr32;
    uefi_map_page(out->lapic_base_addr, out->lapic_base_addr, 0x13, layout);

    acpi_sdt_header_t *madt_hdr = (acpi_sdt_header_t*)madt_phys;
    unsigned char *cur = (unsigned char*)(madt_phys + 44);
    unsigned char *end = (unsigned char*)(madt_phys + madt_hdr->length);

    out->cpu_count = 0;

    while (cur != end) {
        if ((unsigned long long)cur >= (unsigned long long)end) {
            break;
        }
        unsigned char entry_type = *(cur + 0);
        unsigned char entry_len  = *(cur + 1);
        if (entry_len == 0) {
            break;
        }

        if (entry_type == 0) {
            unsigned int flags = *(unsigned int*)(cur + 4);
            if ((flags & 1) != 0) {
                unsigned char apic_id = *(cur + 3);
                if (out->cpu_count != 256) {
                    *(out->cpus_temp_array + out->cpu_count) = apic_id;
                    out->cpu_count++;
                }
            }
        } else if (entry_type == 1) {
            unsigned int ioapic_addr32 = *(unsigned int*)(cur + 4);
            unsigned int gsi_base      = *(unsigned int*)(cur + 8);
            out->io_apic_base_addr = (unsigned long long)ioapic_addr32;
            out->io_apic_gsi_base  = gsi_base;
            uefi_map_page(out->io_apic_base_addr, out->io_apic_base_addr, 0x13, layout);
        }

        cur += entry_len;
    }

    return 0;
}

efi_status_t load_kernel_elf_file(
    efi_handle_t image_handle,
    efi_boot_services_t *bs,
    unsigned long long *out_buffer_phys,
    unsigned long long *out_file_size
) {
    efi_guid_t loaded_image_guid = {
        0x5B1B31A1, 0x9562, 0x11d2,
        { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
    };
    efi_guid_t fs_guid = {
        0x964E5B22, 0x6459, 0x11D2,
        { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
    };

    const unsigned short *kernel_path = L"\\OROSKRNL.ELF";

    efi_loaded_image_protocol_t *loaded_image = 0;
    if (bs->handle_protocol(image_handle, &loaded_image_guid, (void**)&loaded_image) == 0 && loaded_image != 0) {
        if (try_read_from_handle(bs, loaded_image->device_handle, &fs_guid, kernel_path, out_buffer_phys, out_file_size) == 0) {
            return 0;
        }
    }

    unsigned long long handle_count = 0;
    efi_handle_t *handles = 0;
    if (bs->locate_handle_buffer(2, &fs_guid, 0, &handle_count, &handles) == 0 && handles != 0) {
        unsigned long long i = 0;
        while (i != handle_count) {
            efi_handle_t h = *(handles + i);
            if (try_read_from_handle(bs, h, &fs_guid, kernel_path, out_buffer_phys, out_file_size) == 0) {
                bs->free_pool(handles);
                return 0;
            }
            i++;
        }
        bs->free_pool(handles);
    }

    return 0x800000000000000EULL;
}

void init_rtc() {
    unsigned long long rflags;
    __asm__ __volatile__("pushfq; popq %0; cli" : "=r"(rflags) : : "memory");

    outb(0x70, 0x8A);
    unsigned char prev_a = inb(0x71);
    outb(0x70, 0x8A);
    outb(0x71, (prev_a & 0xF0) | 0x06);

    outb(0x70, 0x8B);
    unsigned char prev_b = inb(0x71);
    outb(0x70, 0x8B);
    outb(0x71, prev_b | 0x40);

    outb(0x70, 0x8C);
    (void)inb(0x71);

    outb(0x70, 0x0D);

    __asm__ __volatile__("pushq %0; popfq" : : "r"(rflags) : "memory", "cc");
}

void init_pic() {
    outb(MASTER_PIC_CMD, 0x11);
    io_wait();
    outb(SLAVE_PIC_CMD, 0x11);
    io_wait();

    outb(MASTER_PIC_DATA, 0x20);
    io_wait();
    outb(SLAVE_PIC_DATA, 0x28);
    io_wait();

    outb(MASTER_PIC_DATA, 0x04);
    io_wait();
    outb(SLAVE_PIC_DATA, 0x02);
    io_wait();

    outb(MASTER_PIC_DATA, 0x01);
    io_wait();
    outb(SLAVE_PIC_DATA, 0x01);
    io_wait();

    outb(MASTER_PIC_DATA, 0x00);
    io_wait();
    outb(SLAVE_PIC_DATA, 0x00);
    io_wait();
}

static inline void lapic_write(unsigned long long lapic_base, unsigned int reg_offset, unsigned int value) {
    volatile unsigned int *reg = (volatile unsigned int*)(lapic_base + reg_offset);
    *reg = value;
}

void init_apic(unsigned long long lapic_base) {
    if (lapic_base == 0) {
        return;
    }

    lapic_write(lapic_base, 0x80, 0x00);
    lapic_write(lapic_base, 0xF0, 0x1FF);

    lapic_write(lapic_base, 0x320, 0x10000);
    lapic_write(lapic_base, 0x330, 0x10000);
    lapic_write(lapic_base, 0x340, 0x10000);
    lapic_write(lapic_base, 0x350, 0x10000);
    lapic_write(lapic_base, 0x360, 0x10000);
    lapic_write(lapic_base, 0x370, 0x10000);

    lapic_write(lapic_base, 0xB0, 0x00);
}

static inline void io_apic_write(unsigned long long ioapic_base, unsigned char reg, unsigned int value) {
    if (ioapic_base == 0) {
        return;
    }
    volatile unsigned int *ioregsel = (volatile unsigned int*)(ioapic_base + 0x00);
    volatile unsigned int *iowin    = (volatile unsigned int*)(ioapic_base + 0x10);
    *ioregsel = (unsigned int)reg;
    *iowin    = value;
}

int is_usb_device_path(efi_device_path_protocol_t *dp) {
    if (dp == 0) {
        return 0;
    }
    while (dp->type != 0x7F) {
        if (dp->length < 4) {
            break;
        }
        if (dp->type == 3 && (dp->sub_type == 5 || dp->sub_type == 15 || dp->sub_type == 16)) {
            return 1;
        }
        dp = (efi_device_path_protocol_t*)((unsigned char*)dp + dp->length);
    }
    return 0;
}

void init_io_apic_irqs(unsigned long long ioapic_base) {
    if (ioapic_base == 0) {
        return;
    }

    unsigned int all_irq = 0;
    while (all_irq != 24) {
        unsigned char reg_low  = (unsigned char)(0x10 + all_irq * 2);
        unsigned char reg_high = (unsigned char)(0x10 + all_irq * 2 + 1);
        io_apic_write(ioapic_base, reg_high, 0x00000000);
        io_apic_write(ioapic_base, reg_low,  0x00010000); // Mask bit = 1
        all_irq++;
    }

    unsigned int irq = 1;
    while (irq != 16) {
        unsigned char reg_low  = (unsigned char)(0x10 + irq * 2);
        unsigned char reg_high = (unsigned char)(0x10 + irq * 2 + 1);
        unsigned int vector    = 0x20 + irq;

        io_apic_write(ioapic_base, reg_high, 0x00000000);
        io_apic_write(ioapic_base, reg_low,  vector);
        irq++;
    }
}

unsigned int hash_fnv1a(const unsigned char *buf, unsigned int count) {
    unsigned int hash = 2166136261U;
    unsigned int i = 0;
    while (i != count) {
        hash ^= (unsigned int)(*(buf + i));
        hash *= 16777619U;
        i++;
    }
    return hash;
}

unsigned int compute_uefi_boot_hash(
    efi_handle_t image_handle,
    efi_boot_services_t *bs,
    unsigned char *out_loading_drive,
    unsigned short *out_bytes_per_sector
) {
    efi_guid_t loaded_image_guid = {
        0x5B1B31A1, 0x9562, 0x11d2,
        { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B }
    };
    efi_guid_t block_io_guid = {
        0x964e5b21, 0x6459, 0x11d2,
        { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }
    };
    efi_guid_t device_path_guid = {
        0x09576e91, 0x6d3f, 0x11d2,
        { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b }
    };

    unsigned long long buf_phys = 0;
    if (bs->allocate_pages(0, 2, 1, &buf_phys) != 0) {
        return 0;
    }
    unsigned char *sector_buf = (unsigned char*)buf_phys;

    efi_loaded_image_protocol_t *li = 0;
    if (bs->handle_protocol(image_handle, &loaded_image_guid, (void**)&li) != 0 || li == 0) {
        bs->free_pages(buf_phys, 1);
        return 0;
    }

    int is_usb = 0;
    efi_device_path_protocol_t *dp = 0;
    if (bs->handle_protocol(li->device_handle, &device_path_guid, (void**)&dp) == 0 && dp != 0) {
        is_usb = is_usb_device_path(dp);
    }

    unsigned int result_hash = 0;

    if (is_usb) {
        unsigned long long handle_count = 0;
        efi_handle_t *handles = 0;
        if (bs->locate_handle_buffer(2, &block_io_guid, 0, &handle_count, &handles) == 0 && handles != 0) {
            unsigned long long i = 0;
            while (i != handle_count) {
                efi_handle_t h = *(handles + i);
                efi_device_path_protocol_t *h_dp = 0;
                if (bs->handle_protocol(h, &device_path_guid, (void**)&h_dp) == 0 && is_usb_device_path(h_dp)) {
                    efi_block_io_protocol_t *bio = 0;
                    if (bs->handle_protocol(h, &block_io_guid, (void**)&bio) == 0 && bio != 0 && bio->media != 0) {
                        if (bio->media->media_present && bio->media->logical_partition == 0 && bio->media->block_size != 0) {
                            unsigned int bsize = bio->media->block_size;
                            if (bio->read_blocks(bio, bio->media->media_id, 0, (unsigned long long)bsize, sector_buf) == 0) {
                                result_hash = hash_fnv1a(sector_buf, 100);
                                *out_loading_drive = 0x80;
                                *out_bytes_per_sector = (unsigned short)bsize;
                                break;
                            }
                        }
                    }
                }
                i++;
            }
            bs->free_pool(handles);
        }
    } else {
        unsigned long long handle_count = 0;
        efi_handle_t *handles = 0;
        if (bs->locate_handle_buffer(2, &block_io_guid, 0, &handle_count, &handles) == 0 && handles != 0) {
            unsigned long long i = 0;
            while (i != handle_count) {
                efi_block_io_protocol_t *bio = 0;
                if (bs->handle_protocol(*(handles + i), &block_io_guid, (void**)&bio) == 0 && bio != 0 && bio->media != 0) {
                    unsigned int bsize = bio->media->block_size;
                    if (bio->media->media_present && bsize != 0 && bsize <= 2048) {
                        unsigned long long pvd_lba = 32768ULL / (unsigned long long)bsize;
                        if ((pvd_lba + (2048ULL / bsize)) <= bio->media->last_block) {
                            if (bio->read_blocks(bio, bio->media->media_id, pvd_lba, 2048, sector_buf) == 0) {
                                if (*(sector_buf + 0) == 0x01 &&
                                    *(sector_buf + 1) == 'C'  &&
                                    *(sector_buf + 2) == 'D'  &&
                                    *(sector_buf + 3) == '0'  &&
                                    *(sector_buf + 4) == '0'  &&
                                    *(sector_buf + 5) == '1') {
                                    result_hash = hash_fnv1a(sector_buf, 2048);
                                    *out_loading_drive = 0xE0;
                                    *out_bytes_per_sector = 2048;
                                    break;
                                }
                            }
                        }
                    }
                }
                i++;
            }
            bs->free_pool(handles);
        }
    }

    bs->free_pages(buf_phys, 1);
    return result_hash;
}

efi_status_t load_elf64_segments(
    unsigned long long elf_buffer_phys,
    memory_layout_t *layout,
    unsigned long long *out_entry_point
) {
    elf64_ehdr_t *ehdr = (elf64_ehdr_t*)elf_buffer_phys;

    if (ehdr->e_ident_magic != 0x464C457FU || ehdr->e_ident_class != 2 || ehdr->e_machine != 0x3E) {
        return 1;
    }

    unsigned char *ph_base = (unsigned char*)(elf_buffer_phys + ehdr->e_phoff);
    unsigned short i = 0;

    while (i != ehdr->e_phnum) {
        elf64_phdr_t *phdr = (elf64_phdr_t*)(ph_base + (unsigned long long)i * ehdr->e_phentsize);

        if (phdr->p_type == 1 && phdr->p_memsz != 0) {
            unsigned long long seg_vstart = phdr->p_vaddr;
            unsigned long long seg_vend   = phdr->p_vaddr + phdr->p_memsz;
            unsigned long long file_vend  = phdr->p_vaddr + phdr->p_filesz;

            unsigned long long page_vaddr = seg_vstart & ~0xFFFULL;
            unsigned long long end_vaddr  = (seg_vend + 0xFFFULL) & ~0xFFFULL;

            while (page_vaddr != end_vaddr) {
                unsigned long long phys_page = uefi_alloc_page(layout);
                if (phys_page == 0) {
                    return 2;
                }

                uefi_map_page(page_vaddr, phys_page, 0x3, layout);

                unsigned long long page_next = page_vaddr + 4096ULL;
                if (page_vaddr != file_vend && page_next != seg_vstart) {
                    unsigned long long copy_vstart = page_vaddr;
                    if (seg_vstart > copy_vstart) {
                        copy_vstart = seg_vstart;
                    }

                    unsigned long long copy_vend = page_next;
                    if (file_vend < copy_vend) {
                        copy_vend = file_vend;
                    }

                    if (copy_vend > copy_vstart) {
                        unsigned long long bytes_to_copy  = copy_vend - copy_vstart;
                        unsigned long long dst_offset     = copy_vstart - page_vaddr;
                        unsigned long long file_offset    = phdr->p_offset + (copy_vstart - seg_vstart);

                        memcpy(
                            (void*)(phys_page + dst_offset),
                            (const void*)(elf_buffer_phys + file_offset),
                            bytes_to_copy
                        );
                    }
                }

                page_vaddr += 4096ULL;
            }
        }
        i++;
    }

    *out_entry_point = ehdr->e_entry;
    return 0;
}

__attribute__((noreturn))
void jump_to_kernel(kernel_handoff_t *h) {
    __asm__ __volatile__(
        "cli\n\t"
        "movq %0, %%rsi\n\t"
        "movq 16(%%rsi), %%rax\n\t"
        "movq %%rax, %%cr3\n\t"
        "movq 104(%%rsi), %%rsp\n\t"
        "movq 0(%%rsi), %%r15\n\t"
        "movq 8(%%rsi), %%rax\n\t"
        "movq 16(%%rsi), %%rbx\n\t"
        "movq 24(%%rsi), %%rcx\n\t"
        "movq 32(%%rsi), %%rdx\n\t"
        "movq 40(%%rsi), %%r8\n\t"
        "movq 48(%%rsi), %%r9\n\t"
        "movq 56(%%rsi), %%r10\n\t"
        "movq 64(%%rsi), %%r11\n\t"
        "movq 72(%%rsi), %%r12\n\t"
        "movq 80(%%rsi), %%r13\n\t"
        "movq 88(%%rsi), %%r14\n\t"
        "movq 96(%%rsi), %%rdi\n\t"
        "xorq %%rbp, %%rbp\n\t"
        "jmp *%%r15\n\t"
        :
        : "r"(h)
        : "memory"
    );

    __builtin_unreachable();
}

__attribute__((noreturn))
void finalize_and_boot_kernel(
    efi_handle_t image_handle,
    efi_system_table_t *st,
    efi_graphics_output_protocol_t *gop,
    memory_layout_t *layout,
    tasking_state_t *tasking,
    gdt_tss_state_t *gdt_tss,
    acpi_info_t *acpi,
    unsigned long long kernel_entry
) {
    efi_boot_services_t *bs = st->boot_services;

    unsigned long long boot_info_phys = uefi_alloc_page(layout);
    boot_info_t *bi = (boot_info_t*)boot_info_phys;

    bi->vbe_hor_size     = (unsigned short)gop->mode->info->horizontal_resolution;
    bi->vbe_ver_size     = (unsigned short)gop->mode->info->vertical_resolution;
    bi->vbe_color_depth  = 32;
    bi->vbe_pitch        = (unsigned short)(gop->mode->info->pixels_per_scan_line * 4);
    bi->loading_drive    = 0xE0;
    bi->lfb_buffer_addr  = (unsigned int)(gop->mode->frame_buffer_base & 0xFFFFFFFFULL);

    bi->drive_params.bytes_per_sector = 2048;

    bi->boot_pvd_hash    = 0;
    bi->cpu_count        = acpi->cpu_count;
    memcpy(bi->cpus_ids, acpi->cpus_temp_array, 256);
    bi->bsp_factory_ist1 = gdt_tss->bsp_factory_ist1;
    bi->is_setup         = 0;
    bi->vbe_modes_count  = 0;
    bi->vbe_modes_list   = 0;

    unsigned char loading_drive = 0xE0;
    unsigned short bytes_per_sector = 2048;
    bi->boot_pvd_hash = compute_uefi_boot_hash(image_handle, bs, &loading_drive, &bytes_per_sector);
    bi->loading_drive = loading_drive;
    bi->drive_params.bytes_per_sector = bytes_per_sector;

    unsigned long long map_size = 0;
    unsigned long long map_key = 0;
    unsigned long long desc_size = 0;
    unsigned int desc_version = 0;

    bs->get_memory_map(&map_size, 0, &map_key, &desc_size, &desc_version);
    map_size += desc_size * 8;

    void *final_map = 0;
    bs->allocate_pool(2, map_size, &final_map);

    while (1) {
        unsigned long long cur_size = map_size;
        bs->get_memory_map(&cur_size, final_map, &map_key, &desc_size, &desc_version);
        if (bs->exit_boot_services(image_handle, map_key) == 0) {
            break;
        }
    }

    load_gdt_and_tss(gdt_tss);
    init_pic();
    init_rtc();
    init_apic(acpi->lapic_base_addr);
    init_io_apic_irqs(acpi->io_apic_base_addr);

    outb(MASTER_PIC_DATA, 0xFF);
    io_wait();
    outb(SLAVE_PIC_DATA, 0xFF);
    io_wait();

    kernel_handoff_t *handoff = (kernel_handoff_t*)(boot_info_phys + 0x800ULL);
    handoff->entry_point       = kernel_entry;
    handoff->mb_memory         = layout->mb_memory;
    handoff->pt_begin          = layout->pt_begin;
    handoff->pt_end            = layout->pt_end;
    handoff->ptv_end           = layout->ptv_end;
    handoff->bitmap_end        = layout->bitmap_end;
    handoff->lfb_buffer_addr   = gop->mode->frame_buffer_base;
    handoff->lapic_base_addr   = acpi->lapic_base_addr;
    handoff->timer_ticks       = 0;
    handoff->process_table_ptr = (unsigned long long)tasking->process_table_ptr;
    handoff->kheap_current_end = layout->kheap_current_end;
    handoff->acpi_rsdp_addr    = acpi->acpi_rsdp_addr;
    handoff->boot_info_addr    = boot_info_phys;
    handoff->kernel_rsp        = tasking->kernel_main_rsp;

    jump_to_kernel(handoff);
}

__attribute__((ms_abi))
efi_status_t efi_main(efi_handle_t image_handle, efi_system_table_t *system_table) {
    (void)image_handle;

    system_table->con_out->output_string(system_table->con_out, L"Setup is inspecting your computer's hardware configuration...\r\n");
    efi_guid_t gop_guid = { 
        0x9042a9de, 
        0x23dc, 
        0x4a38, 
        { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } 
    };

    system_table->boot_services->set_watchdog_timer(0, 0, 0, 0);
    
    efi_graphics_output_protocol_t *gop = 0;

    efi_status_t status = system_table->boot_services->locate_protocol(&gop_guid, 0, (void**)&gop);

    if (status == 0 && gop != 0 && gop->mode != 0) {
        if (gop->mode->frame_buffer_base == 0 || gop->mode->info == 0) {
            gop->set_mode(gop, gop->mode->mode);
        }

        if (gop->mode->frame_buffer_base != 0 && gop->mode->info != 0) {
            unsigned int* fb = (unsigned int*)gop->mode->frame_buffer_base;
            unsigned int pitch = gop->mode->info->pixels_per_scan_line;
            unsigned int height = gop->mode->info->vertical_resolution;

            e820_entry_t *e820_map = 0;
            unsigned int e820_count = 0;
            memory_layout_t mem_layout = {0};

            efi_status_t mem_status = prepare_uefi_memory(
                system_table->boot_services,
                gop->mode->frame_buffer_base,
                gop->mode->frame_buffer_size,
                &e820_map,
                &e820_count,
                &mem_layout
            );

            if (mem_status == 0) {

                mark_usable_memory_free(e820_map, e820_count, &mem_layout);
                protect_system_regions(&mem_layout);
                kheap_init(&mem_layout);

                tasking_state_t tasking;
                uefi_tasking_init(&mem_layout, mem_layout.kheap_first_page_phys, &tasking);

                gdt_tss_state_t gdt_tss;
                uefi_tss_gdt_init(&mem_layout, tasking.kernel_main_rsp, &gdt_tss);

                unsigned long long kernel_buf = 0;
                unsigned long long kernel_size = 0;

                efi_status_t kern_status = load_kernel_elf_file(
                    image_handle,
                    system_table->boot_services,
                    &kernel_buf,
                    &kernel_size
                );

                if (kern_status == 0) {
                    acpi_info_t acpi_info;
                    memset(&acpi_info, 0, sizeof(acpi_info_t));

                    efi_status_t acpi_status = uefi_init_acpi(system_table, &mem_layout, &acpi_info);
                    if (acpi_status == 0) {
                        
                        unsigned long long kernel_entry = 0;
                        efi_status_t elf_status = load_elf64_segments(kernel_buf, &mem_layout, &kernel_entry);
                        if (elf_status == 0) {
                            
                            finalize_and_boot_kernel(
                                image_handle,
                                system_table,
                                gop,
                                &mem_layout,
                                &tasking,
                                &gdt_tss,
                                &acpi_info,
                                kernel_entry
                            );

                        } else {
                            system_table->con_out->output_string(system_table->con_out, L"OrangeOS can not start because the following file the missing or corrupt:\n\nOROSKRNL.ELF\n\nPlease re-install a copy of the above file.");
                            print_status(system_table->con_out, elf_status);
                        }

                    } else {
                        system_table->con_out->output_string(system_table->con_out, L"ACPI Error: ");
                        print_status(system_table->con_out, acpi_status);
                    }
                } else {
                    system_table->con_out->output_string(system_table->con_out, L"OrangeOS can not start because the following file the missing or corrupt:\n\nOROSKRNL.ELF\n\nPlease re-install a copy of the above file.");
                }

            } else {
                system_table->con_out->output_string(system_table->con_out, L"OrangeOS failed to initialize memory subsystem. The system can not continue.\r\n");
            }
        }
    } else {
        system_table->con_out->output_string(system_table->con_out, L"GOP initialization failed.\r\n");
    }

    while(1) { __asm__ __volatile__("hlt"); }
    return 0;
}