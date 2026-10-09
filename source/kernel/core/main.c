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

#include "Am79C973.h"

uintptr_t main_mac;

void KeBugCheck(int stopCode) {
    if(stopCode == 0x20) {
        KeBugMemoryManagement();
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

    for (uint32_t code = 0x80000002; code <= 0x80000004; code++) {
        cpu_get_cpuid(code, regs);
        
        memcpy(buffer, &regs[0], 16);
        buffer += 16;
    }

    *buffer = '\0';
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
            
            switch (*fmt) {
                case 'c': {
                    *out++ = (char)va_arg(args, int);
                    break;
                }
                case 's': {
                    char *s = va_arg(args, char*);
                    if (!s) s = "(null)";
                    while (*s) {
                        *out++ = *s++;
                    }
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    int_to_str(d, out, 10, 1);
                    while (*out) out++;
                    break;
                }
                case 'x': {
                    unsigned int x = va_arg(args, unsigned int);
                    int_to_str(x, out, 16, 0);
                    while (*out) out++;
                    break;
                }
                case '%': {
                    *out++ = '%';
                    break;
                }
                default: {
                    *out++ = '%';
                    *out++ = *fmt;
                    break;
                }
            }
        } else if (*fmt == '\n') {
            *out = '\0';
            if (out != buffer) {
                print_text(buffer, x, y, r, g, b, alpha);
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
        print_text(buffer, x, y, r, g, b, alpha);
    }
}

void draw_device_manager_content(const window_t* win, int bx, int by, int bw, int bh) {

    char cpu_name[64];
    get_cpu_brand_string(cpu_name);

    kprintf(bx+10, by+10, 0x00, 0x00, 0x00, 0xFF, "%d devices found on this computer.", total_devices);
    kprintf(bx+10, by+30, 0x20, 0x20, 0x20, 0xFF, "- CPU: %s", cpu_name);
    kprintf(bx+10, by+50, 0x20, 0x20, 0x20, 0xFF, "- Physical memory available for OrangeOS: %d MB", mb_memory);
    
    draw_rect(bx + 10, by + 80, bw - 20, 15, 0x22, 0x8B, 0x22, 0xFF);
}

static button_t btn_ok   = { .rel_x = 20,  .rel_y = 60, .width = 80, .height = 24, .text = "OK",   .is_pressed = 0 };

void draw_about_os_content(const window_t* win, int bx, int by, int bw, int bh) {
    print_unicode("OrangeOS Alpha 1.0", bx + 20, by + 20, 0x80, 0x00, 0x00, 0xFF);
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
    createWindow(400, 400, 350, 200, "Device Manager", draw_device_manager_content, NULL);
}

void openAbout() {
    createWindow(500, 400, 300, 150, "About OrangeOS", draw_about_os_content, mouse_about_os_content);
}

void draw_network_adapter_window(const window_t* win, int bx, int by, int bw, int bh) {
    
    int8_t* mac_array = (int8_t*)main_mac;
    kprintf(bx + 10, by + 10, 0x00, 0x00, 0x00, 0xFF, "OrangeOS detected and initialized the Am79C973 device (PCnet-FAST III Ethernet Controller)\n\nMAC address:%x:%x:%x:%x:%x:%x", mac_array[0], mac_array[1], mac_array[2], mac_array[3], mac_array[4], mac_array[5]);
}

void KiInitializeKernel() {

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

        if(xhci_addr == 0) {

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

    main_mac = init_am79c973();
    if(main_mac != 0) {
        createWindow(300, 200, 300, 150, "New device found", draw_network_adapter_window, NULL);
    }

    while (1) {
        draw_all_windows();
        sleep_ms(30000);
    }
}