#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "ata.h"
#include "ahci.h"
#include "core.h"
#include "disk.h"
#include "gui.h"
#include "pci.h"
#include "nvme.h"
#include "xhci.h"
#include "os_memory.h"

#include "Am79C973.h"

int8_t ap_running_flag = 0;
uint8_t bsp_apic_id;

am79c973_device_t am73c973_device;
ACPI_STATUS acpi_status;

uint64_t core_tss_stacks[256];
tss64_t tss_per_cpu[256];

extern uint32_t* fb_back_buffer;

char fs_types_mbr[256][25] = {
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
    "Extended partition"
};

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

Partition* find_boot_partition(boot_info_t* boot_info) {

    uint8_t buf[2048];

    for (int i = 0; i < get_partition_count(); i++) {
        Partition* p = get_partition(i);
        Drive* d = get_disk(p->drive_index);

        size_t sector_size = (d->atapi == 1) ? 2048 : 100;
        uint64_t target_lba = (d->atapi == 1) ? 16 : p->start_lba;

        int status = disk_read(p->drive_index, target_lba, 1, (uint64_t)buf);

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

void int_to_str(long long value, char *buf, int base, int is_signed) {
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
        const char *digits = "0123456789abcdef";
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
                    while (src[len]) len++;
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    int_to_str(d, temp_buf, 10, 1);
                    while (temp_buf[len]) len++;
                    break;
                }
                case 'x': {
                    unsigned int val_x = va_arg(args, unsigned int);
                    int_to_str(val_x, temp_buf, 16, 0);
                    while (temp_buf[len]) len++;
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
                print_unicode(buffer, x, y, r, g, b, alpha);
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
        print_unicode(buffer, x, y, r, g, b, alpha);
    }
}


char device_class_name[19][40] = {
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

void draw_device_manager_content(const window_t* win, int bx, int by, int bw, int bh) {

    char cpu_name[64];
    get_cpu_brand_string(cpu_name);

    kprintf(bx+10, by+10, 0x00, 0x00, 0x00, 0xFF, "%d devices found on this computer.", total_devices);
    kprintf(bx+10, by+30, 0x20, 0x20, 0x20, 0xFF, "- CPU: %s, %d logical processors", cpu_name, boot_info->cpu_count);
    kprintf(bx+10, by+50, 0x20, 0x20, 0x20, 0xFF, "- Physical memory available for OrangeOS: %d MB", mb_memory);
    
    int current_y = 70;

    for (uint64_t i = 0; i < total_devices; i++) {

        int class_code = pci_devices[i].class_code;

        if(class_code >= 19) class_code = 0;

        kprintf(bx+10, by+current_y, 0x20, 0x20, 0x20, 0xFF, "%s - PCI: Bus %d, Slot %d, Func %d -> Vendor: 0x%x, Device: 0x%x", device_class_name[class_code], pci_devices[i].bus, pci_devices[i].device, pci_devices[i].function, pci_devices[i].vendor_id, pci_devices[i].device_id);

        current_y += 20;

    }


}

static button_t btn_ok   = { .rel_x = 20,  .rel_y = 60, .width = 80, .height = 24, .text = "OK",   .is_pressed = 0 };

void draw_about_os_content(const window_t* win, int bx, int by, int bw, int bh) {
    print_unicode("OrangeOS Alpha 1.1", bx + 20, by + 20, 0x80, 0x00, 0x00, 0xFF);
    print_unicode("Copyright(C) Alexander Volokhov, 2023-2026", bx + 20, by + 35, 0x00, 0x00, 0x00, 0xFF);

    draw_button(bx, by, &btn_ok);

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

void draw_network_adapter_window(const window_t* win, int bx, int by, int bw, int bh) {
    kprintf(bx + 10, by + 10, 0x00, 0x00, 0x00, 0xFF, "OrangeOS detected and initialized the Am79C973 device (PCnet-FAST III Ethernet Controller)\n\nMAC address:%02x:%02x:%02x:%02x:%02x:%02x", am73c973_device.mac[0], am73c973_device.mac[1], am73c973_device.mac[2], am73c973_device.mac[3], am73c973_device.mac[4], am73c973_device.mac[5]);
}

void draw_acpi_error_text(const window_t* win, int bx, int by, int bw, int bh) {
    kprintf(bx + 10, by + 10, 0x00, 0x00, 0x00, 0xFF, "Failed to initialize ACPI. Error code: %d", acpi_status);

    draw_button(bx, by, &btn_ok);

}

void draw_startup_window(const window_t* win, int bx, int by, int bw, int bh) {

    int partitions_count = get_partition_count();

    kprintf(bx + 10, by + 10, 0x00, 0x00, 0x00, 0xFF, "Total partitions: %d, bootable partition hash %x", partitions_count, boot_info->boot_pvd_hash);

    for(int i = 0; i < partitions_count; i++) {

        Partition* partition = get_partition(i);

        int partition_fs = partition->fs_type;
        if(partition_fs >= 0x1F) partition_fs = 13;

        kprintf(bx + 10, by + (30 + (15 * i)), 0x00, 0x00, 0x00, 0xFF, "Partition %d: on drive %d, %s, %s, %d total sectors, start at %d LBA", i, partition->drive_index, fs_types_mbr[partition_fs], (partition->is_active) ? "active" : "not active", partition->sector_count, partition->start_lba);

    }

    draw_button(bx, by, &btn_ok);
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

typedef struct {
    uint64_t rsp0;
    uint64_t ist1;
} bsp_old_stacks_t;

uint64_t get_bsp_stack_top_from_initial_tss(void) {
    uint16_t tr_selector;
    asm volatile("str %0" : "=r"(tr_selector));

    struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
    } gdtr;
    asm volatile("sgdt %0" : "=m"(gdtr));

    uint8_t* gdt_entry = (uint8_t*)(gdtr.base + (tr_selector & 0xFFF8));

    uint64_t tss_base = ((uint64_t)gdt_entry[2]) |
                        ((uint64_t)gdt_entry[3] << 8) |
                        ((uint64_t)gdt_entry[4] << 16) |
                        ((uint64_t)gdt_entry[7] << 24) |
                        ((uint64_t)*(uint32_t*)&gdt_entry[8] << 32);

    tss64_t* initial_tss = (tss64_t*)tss_base;

    return initial_tss->ist[1];
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

void init_logical_processors() {

    bsp_apic_id = get_lapic_id();
    initialize_core_tss(bsp_apic_id, boot_info->bsp_factory_ist1);

    for(int i = 0; i < boot_info->cpu_count; i++) {

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

        wakeup_ap_core(boot_info->cpus_ids[i], 0x08);

        while(ap_running_flag == 0) {
            asm volatile("hlt");
        }

    }

}

void KiInitializeKernel() {

    init_double_buffering();

    init_ahci();
    init_ata_dma();
    int status = init_nvme();
    if(status != 0) {
        status = nvme_controller_reset();
        if(status != 0) {
            nvme_identify_disk();
            nvme_identify_namespace();

            status = nvme_create_io_queues();
            if(status != 0) {
                // NVMe detection
            }
        }
    }

    registerATADevices();
    registerSATADevices();

    pci_device_t* xhci_controller = find_xhci_controller();
    if(xhci_controller != 0) {
        uintptr_t xhci_addr = get_xhci_base_address(xhci_controller);

        if(xhci_addr != 0) {

            enable_xhci_pci_bus(xhci_controller);
            xhci_read_capabilities(xhci_addr);
            xhci_reset_controller(xhci_addr);
            xhci_init_event_ring(xhci_addr);
            xhci_init_dcbaa(xhci_addr);
            xhci_start_controller(xhci_addr);
            xhci_init_command_ring(xhci_addr);
            xhci_finalize_controller_init(xhci_addr);
            //xhci_probe_ports(xhci_addr);

        }

    }

    partition_scan_all_drives();

    status = init_am79c973(&am73c973_device);
    if(status != 0) {
        createWindow(100, 200, 550, 150, "New device found", draw_network_adapter_window, NULL);
        send_test_arp_packet(am73c973_device.mac);
    }

    AcpiDbgLayer = 0x0001FFFF;
    AcpiDbgLevel = 0x0000001F | 0x00000001;

    //acpi_status = AcpiInitializeSubsystem();

    //if(acpi_status != 0) {
        //createWindow(300, 200, 300, 150, "OrangeOS: Error", draw_acpi_error_text, mouse_about_os_content);
    //}
    
    Partition* bootPartition = find_boot_partition(boot_info);
    if(bootPartition == NULL) {
        KeBugCheck(0x22);
    }

    init_logical_processors();

    while (1) {
        draw_all_windows();
        sleep_ms(30000);
    } // check_incoming_packets();
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

    volatile int* flag = (volatile int*)&ap_running_flag;
    *flag = 1;

    while(1) {
        asm volatile("hlt" ::: "memory");
    }
}