org 0xFFFF900000000000

format ELF64

public kernel_header
public kernel_start
public pci_devices

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
mov [lapic_base_addr], r10
mov [timer_ticks], r11
mov [process_table_ptr], r12
mov [kheap_current_end], r13
mov [acpi_rsdp_addr], r14

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

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov eax, 480000

gui_screen:

mov [rsi], byte 255
mov [rsi+1], byte 148
mov [rsi+2], byte 0
mov [rsi+3], byte 255

dec eax
add rsi, 4
cmp eax, 00h
jne gui_screen
je continue

continue:

call init_dynamic_idt
call init_syscall_interface

;call init_acpi_system

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

mov r13d, 10
mov r12d, 10
mov r11d, 300
mov r10d, 300
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh

call draw_window


mov r15, [total_devices]
mov r14, total_devices_text
mov r13, 0
call format_10_string_with_64_int

mov r13, r14
mov r15d, 20
mov r14d, 50
mov cl, 0
mov bh, 0
mov bl, 0
mov ch, 0FFh
call draw_text

mov rcx, 0
mov r8, 70

.draw_all_pci_devices:

mov rax, rcx
mov rbx, 16
mul rbx

mov rbx, [pci_devices]
add rbx, rax

mov r15b, [rbx+4] ; Class

mov rdx, pci_devices_list_text

.search_class:

mov al, [rdx]
cmp al, r15b
je .class_found

add rdx, 16
mov r10, pci_devices_list_text_end
cmp rdx, r10
je .class_not_found

jmp .search_class

.class_not_found:

mov rdx, pci_devices_list_text

.class_found:

mov rax, [rdx+1]
mov r13, rax

push r15
push r14
push rcx
push rbx

mov r15d, 20
mov r14d, r8d
mov cl, 0
mov bh, 0
mov bl, 0
mov ch, 0FFh
call draw_text

pop rbx
pop rcx
pop r14
pop r15

.next_device:

add rcx, 1
add r8, 15

cmp rcx, [total_devices]
je .pci_devices_drawed

jmp .draw_all_pci_devices

.pci_devices_drawed:

call KiInitializeKernel

.no_tasks:
hlt
jmp .no_tasks

kernel_idle_task:
hlt
jmp kernel_idle_task

file_addr dq 0

loading_drive db 0
lfb_buffer_addr dd 0

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