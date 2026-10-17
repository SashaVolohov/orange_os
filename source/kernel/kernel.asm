org 0xFFFF900000000000

format ELF64

public kernel_header
public kernel_start
public pci_devices
public fb_front_buffer
public lfb_buffer_addr
public boot_info

section '.text.header' writeable executable

kernel_header:

magic_signature db 'OEX!' ; Сигнатура ядра
header_size dw 16 ; Размер заголовка OEX
entry_point dq kernel_start ; Точка входа
flags dw 2; Флаги

section '.text' writeable executable

extrn AcpiInitializeSubsystem
extrn AcpiInitializeTables
extrn AcpiLoadTables
extrn AcpiEnableSubsystem
extrn AcpiInitializeObjects

extrn KiInitializeKernel

extrn kp_entry_ap

use64

include "system_font.asm"
include "arch/x86-64/kernel_functions.asm"
include "arch/x86-64/exceptions.asm"
include "arch/x86-64/memory_manager.asm"
include "drivers/storage/ata.asm"
include "arch/x86-64/scheduler.asm"
include "drivers/system_timer.asm"
include "drivers/storage/floppy.asm"
include "drivers/keyboard.asm"
include "drivers/storage/fat.asm"
include "drivers/mouse.asm"
include "drivers/com.asm"
include "drivers/lpt.asm"
include "drivers/cmos.asm"
include "drivers/acpi/acpi.asm"
include "drivers/apic.asm"
include "arch/x86-64/gui.asm"
include "arch/x86-64/unicode.asm"
include "arch/x86-64/unicode_font.asm"
include "drivers/pci.asm"
include "drivers/audio/hd_audio.asm"
include "drivers/storage/atapi.asm"
include "drivers/storage/cdfs.asm"
include "setup/setup.asm"

kernel_start:

mov [mb_memory], rax
mov [pt_begin], rbx
mov [pt_end], rcx
mov [ptv_end], rdx
mov [bitmap_end], r8
mov [lfb_buffer_addr], r9d
mov [lfb_mouse_addr], r9d
mov [lapic_base_addr], r10
mov [timer_ticks], r11
mov [process_table_ptr], r12
mov [kheap_current_end], r13
mov [acpi_rsdp_addr], r14

mov [boot_info], rdi

mov rbx, [boot_info] ; Hor size
mov ax, word [rbx]
mov [vbe_hor_size], ax
mov ax, word [rbx+2] ; Ver size
mov [vbe_ver_size], ax
mov al, byte [rbx+4]; Color depth
mov [vbe_color_depth], al

mov rax, 0
mov eax, [lfb_buffer_addr]
mov [fb_front_buffer], rax

mov dword [current_task_index], 1
mov dword [total_processes], 2

mov rbx, [process_table_ptr]
mov rcx, [rbx + PCB_rsp]
add rcx, 120
mov rdx, kernel_idle_task
mov [rcx], rdx ; Replace RIP of the bootmgr idle process

check_popcnt:
    mov eax, 1
    cpuid

    bt ecx, 23
    jnc .no_popcnt
    jmp .clear_screen

.no_popcnt:
    mov rsi, get_used_memory_mb_no_popcnt
    mov [get_used_memory_mb_ptr], rsi

.clear_screen:

cmp [lfb_buffer_addr], 0
je continue

call mouse_init

continue:

call init_dynamic_idt
call init_syscall_interface

sti

mov rdi, 512 ; Для 32 устройств шины PCI
call kmalloc

test rax, rax
jz memory_management

mov [pci_devices], rax

call pci_scan_bus_linear

cmp [lfb_buffer_addr], 0
je setup_kernel

mov rbx, [pci_devices]
mov rcx, 0

.search_audio_device:

mov rax, 16
mul rcx
add rbx, rax

mov r8b, [rbx+4] ; Class
mov r9b, [rbx+8] ; Subclass
mov r10b, [rbx+5] ; Bus
mov r11b, [rbx+6] ; Device
mov r12b, [rbx+7] ; Function

cmp r8b, 0x04 ; Multimedia Controller
jne .next_audio_device

cmp r9b, 0x03 ; High Definition Audio Controller
jne .next_audio_device

jmp .audio_device_found

.next_audio_device:

add rcx, 1

mov r15, [total_devices]
cmp rcx, r15
jae .audio_device_not_found

jmp .search_audio_device

.audio_device_not_found:

call .draw_interface

.audio_device_found:

mov rdi, r10
mov rsi, r11
mov rdx, r12
mov rcx, 0
call pci_get_bar

mov [hda_addr], rax
cmp r9, 0
jne .audio_device_not_found

mov rdi, r10 ; Bus
mov rsi, r11 ; Device
mov rdx, r12 ; Function
mov rcx, 0x04
call pci_read_config_dword

or ax, 0x0006

mov r8, r10
shl r8, 16
mov r9, r11
shl r9, 11
or r8, r9
mov r9, r12
shl r9, 8
or r8, r9
or r8, -0x80000000
or r8, 0x04

push rax
mov edx, 0x0CF8
mov eax, r8d
out dx, eax

pop rax
mov edx, 0xCFC
out dx, eax



mov rdi, [hda_addr]
mov rsi, rdi
mov rdx, 011b
call map_page

test rax, rax
jz .audio_device_not_found

call hda_init_audio_system

mov r15, [hda_addr]
mov rdi, [corb_phys_addr]
mov rsi, [rirb_phys_addr]
mov edx, 0x000F0000
call [hda_send_verb_ptr]
jc .audio_device_not_found

mov rdi, 2000
call sleep_ms

mov r15, [hda_addr]
mov rdi, [corb_phys_addr]
mov rsi, [rirb_phys_addr]
call hda_scan_widgets


mov al, [dac_id]
mov bl, [pin_id]

mov edx, 0x00070500
movzx eax, al
shl eax, 20
or edx, eax
mov r15, [hda_addr]
mov rdi, [corb_phys_addr]
mov rsi, [rirb_phys_addr]

call [hda_send_verb_ptr]

mov al, [pin_id]

mov edx, 0x00070500
movzx eax, al
shl eax, 20
or edx, eax

mov r15, [hda_addr]
mov rdi, [corb_phys_addr]
mov rsi, [rirb_phys_addr]

call [hda_send_verb_ptr]

mov al, [pin_id]

mov edx, 0x00070C40
movzx eax, al
shl eax, 20
or edx, eax
mov r15, [hda_addr]
mov rdi, [corb_phys_addr]
mov rsi, [rirb_phys_addr]
call [hda_send_verb_ptr]


mov al, [pin_id]
mov edx, 0x0003B07F
movzx eax, al
shl eax, 20
or edx, eax
mov r15, [hda_addr]
mov rdi, [corb_phys_addr]
mov rsi, [rirb_phys_addr]
call [hda_send_verb_ptr]

call iso9660_init
cmp rax, -1
je acpi_error

call alloc_page
test rax, rax
jz memory_management

mov [root_cd_buffer], rax

mov rdi, rax
mov rsi, 0
mov esi, [root_lba]
mov rdx, 1
call read_atapi_sector

mov rdi, [root_cd_buffer]
mov rsi, concord_file
mov rdx, 11

call iso9660_find_entry

test rax, rax
jz acpi_error


mov [setup_lba], rax
mov [setup_size], rcx

call alloc_page

test rax, rax
jz memory_management

mov [music_buffer], rax

sub rcx, 4096
cmp rcx, 0
jle .music_memory

.more_pages:

call alloc_page

test rax, rax
jz memory_management

mov rdx, rax

sub rcx, 4096
cmp rcx, 0
jle .music_memory
jmp .more_pages

.music_memory:

mov rax, [setup_lba]
mov rcx, [setup_size]

.check_sectors:

mov rdx, 0
mov rax, [setup_size]
mov rbx, 2048

div rbx

cmp rdx, 0
ja .add_rax

jmp .load_file

.add_rax:
add rax, 1

.load_file:

mov rdi, [music_buffer]
mov rsi, [setup_lba]
mov rdx, rax

call read_atapi_sector

call alloc_page
test rax, rax
jz memory_management

mov rbx, rax
mov rax, [music_buffer]
mov rcx, [setup_size]

mov dword [rbx], eax
shr rax, 32
mov dword [rbx + 4], eax
mov dword [rbx + 8], ecx
mov dword [rbx + 12], 1

mov r15, [hda_addr]
mov r12, rbx
mov r13, rcx
mov r14, 0
call hda_setup_output_stream

.draw_interface:

call KiInitializeKernel

kernel_idle_task:
hlt
jmp kernel_idle_task

TRAMPOLINE_BASE = 0x8000

public trampoline_start
trampoline_start:
use16
    cli
    cld
    
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    lgdt [trampoline_gdt_desc - trampoline_start + TRAMPOLINE_BASE]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp 0x08:(trampoline_pm - trampoline_start + TRAMPOLINE_BASE)

use32
trampoline_pm:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    mov eax, cr4
    or eax, 1 shl 5
    mov cr4, eax
    
    mov eax, dword [ap_target_cr3 - trampoline_start + TRAMPOLINE_BASE]
    mov cr3, eax
    
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 shl 8
    wrmsr
    
    mov eax, cr0
    or eax, (1 shl 31) or 1 ; CR0.PG = 1, CR0.PE = 1
    mov cr0, eax
    
    jmp 0x18:(trampoline_lm - trampoline_start + TRAMPOLINE_BASE)

use64
trampoline_lm:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    mov rsp, [ap_stack_top]
    
    xor rbp, rbp
    
    mov rax, kp_entry_ap
    call rax

.halt:
    hlt
    jmp .halt

public ap_target_cr3
public ap_stack_top
ap_target_cr3    dq 0
ap_stack_top     dq 0

align 16
trampoline_gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00209A0000000000
trampoline_gdt_end:

trampoline_gdt_desc:
    dw trampoline_gdt_end - trampoline_gdt - 1
    dq TRAMPOLINE_BASE + (trampoline_gdt - trampoline_start)

public trampoline_end
trampoline_end:

file_addr dq 0

loading_drive db 0
lfb_buffer_addr dd 0
lfb_mouse_addr dd 0
fb_front_buffer dq 0

current_progress dq 0
needed_progress dq 0

efer_bsod dd 0

vbe_hor_size dw 800
vbe_ver_size dw 600
vbe_color_depth db 32
is_kernel db 1
pci_devices dq 0
root_cd_buffer dq 0
setup_lba dq 0
setup_size dq 0
setup_addr dq 0

total_devices_text db '0x1111111111111111   devices found on this computer.',0
concord_file db 'CONCORD.PCM'
music_buffer dq 0

boot_info dq 0