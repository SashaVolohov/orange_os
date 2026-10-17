org 8000h

include "init_16.asm"
include "init_32.asm"

include "system_font.asm"
include "kernel_functions.asm"
include "exceptions.asm"
include "memory_manager.asm"
include "ata.asm"
include "scheduler.asm"
include "system_timer.asm"
include "floppy.asm"
include "keyboard.asm"
include "fat.asm"
include "mouse.asm"
include "com.asm"
include "lpt.asm"
include "cmos.asm"
include "acpi.asm"
include "apic.asm"
include "atapi.asm"
include "cdfs.asm"

long_start:

mov ax, 10h
mov ds, ax
mov es, ax
mov fs, ax
mov ss, ax
mov esp, 7000h
mov gs, ax

call memory_init
call videomem_init
call allocator_init

call init_pic

LIDT [IDTR]

call com1_init
call com2_init

call init_rtc

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov eax, 480000

gui_screen:

mov [rsi], byte 0
mov [rsi+1], byte 0
mov [rsi+2], byte 0
mov [rsi+3], byte 0

dec eax
add rsi, 4
cmp eax, 00h
jne gui_screen
je continue

continue:

; 1A00h - начало
; [1900h] - конец

mov r8, 1A00h
mov r11d, 40
mov r9, 0

print_memory_map:
    push r8
    push rsi
    push rbx

    mov ebx, [r8 + 16]
    cmp ebx, 1
    jne .restore_and_done

    mov rdi, [r8]
    mov rcx, [r8 + 8]

    test rcx, rcx
    jz .restore_and_done

    mov rbx, rdi
    add rbx, rcx
    
    add rdi, 0xFFF
    and rdi, not 0xFFF

.loop_pages:
    cmp rdi, rbx
    jae .restore_and_done

    push rax
    push rcx
    push rdx
    push rdi
    push r8
    
    call free_page
    
    pop r8
    pop rdi
    pop rdx
    pop rcx
    pop rax

    add rdi, 4096
    jc .restore_and_done

    jmp .loop_pages

.restore_and_done:
    pop rbx
    pop rsi
    pop r8
    jmp .done

.done:

mov rdx, 0
mov dx, [1800h]
add r8, rdx
add r11d, 20
add r9, 1

mov rbx, 0
mov bx, [1900h]
cmp r8, rbx
jne print_memory_map

protect_core:
    mov rdi, 0x0
    mov rcx, [bitmap_end]

.loop_protect:
    cmp rdi, rcx
    jae .pmm_fully_ready

    mov rbx, rdi
    shr rbx, 12
    
    mov rdx, [ptv_end]
    
    bts [rdx], rbx          

    add rdi, 4096
    jmp .loop_protect

.pmm_fully_ready:

call kheap_init
call tasking_init

lea rdi, [tss0]
mov rcx, 13
xor rax, rax
rep stosq

lea rbx, [tss0]
mov rax, rsp
mov [rbx + TSS64_rsp0], rax

call alloc_page
add rax, 4096
and rax, not 0xF
mov [rbx + TSS64_ist1], rax
mov [bsp_factory_ist1], rax

mov word [rbx + TSS64_iomap_base], 104

call install_tss_descriptor

lgdt [GDTR_64]

mov ax, 0x28
ltr ax

call init_acpi
call init_apic

call calibrate_lapic_timer
call init_lapic_timer

call init_io_apic_keyboard
call init_io_apic_irq2
call init_io_apic_irq3
call init_io_apic_irq4
call init_io_apic_irq5
call init_io_apic_irq6
call init_io_apic_irq7
call init_io_apic_irq8
call init_io_apic_irq9
call init_io_apic_irq10
call init_io_apic_irq11
call init_io_apic_irq12
call init_io_apic_irq13
call init_io_apic_irq14
call init_io_apic_irq15

call iso9660_init
cmp rax, -1
je kernel_not_found


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
mov rsi, kernel_file
mov rdx, 12

call iso9660_find_entry

test rax, rax
jz kernel_not_found


mov [setup_lba], rax
mov [setup_size], rcx

mov r15, rcx
add r15, 16384
mov r14, 0
mov rdi, 0xFFFF900000000000

alloc_pages_for_kernel:

mov [2000h], rdi

call alloc_page
test rax, rax
jz memory_management

mov rsi, rax
mov rdx, 011b
call map_page

test rax, rax
jz memory_management

add rdi, 4096
add r14, 4096

cmp r14, r15
jle alloc_pages_for_kernel

mov rdx, 0
mov rax, r15
mov rbx, 2048
div rbx

mov [needed_progress], rax

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

mov r13, orange_os_start
mov r15d, 325
mov r14d, 550
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

push rax

mov rdi, show_loading_progress
call create_task

pop rax

mov rdi, 0xFFFF900000000000
mov rsi, [setup_lba]
mov rdx, rax

call read_atapi_sector

mov rbx, [process_table_ptr]
add rbx, struct_pcb_size
add rbx, struct_pcb_size
mov dword [rbx + PCB_state], TASK_STATE_ZOMBIE

mov rax, 0xFFFF900000000000
add rax, 6
mov r15, [rax]

cli

mov rax, [mb_memory]
mov rbx, [pt_begin]
mov rcx, [pt_end]
mov rdx, [ptv_end]
mov r8, [bitmap_end]
mov r9d, [lfb_buffer_addr]
mov r10, [lapic_base_addr]
mov r11, [timer_ticks]
mov r12, [process_table_ptr]
mov r13, [kheap_current_end]
mov r14, [acpi_rsdp_addr]
mov rdi, boot_info

jmp r15

kernel_not_found:
cli

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov eax, 480000

mov [rsi], byte 0
mov [rsi+1], byte 0
mov [rsi+2], byte 0
mov [rsi+3], byte 0

mov r13, kernel_not_found_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

mov r13, kernel_file
mov r15d, 0
mov r14d, 20
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

mov r13, kernel_not_found_text_hint
mov r15d, 0
mov r14d, 40
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

hlt
jmp $

show_loading_progress:

    mov rax, [current_progress]
    mov rbx, 100
    mul rbx

    mov rcx, [needed_progress]
    div rcx

    mov rbx, 8
    mul rbx
    sub rax, 10

    mov cl, 0x80
    mov bh, 0x80
    mov bl, 0x80
    mov ch, 0xFF

    mov r14d, 570

.loop_y:
    mov r15d, 10

.loop_x:

    cmp r15, rax
    jle .white_color
    
    mov cl, 0x80
    mov bh, 0x80
    mov bl, 0x80
    mov ch, 0xFF

    jmp .draw

.white_color:
    mov cl, 0xFF
    mov bh, 0xFF
    mov bl, 0xFF
    mov ch, 0xFF

.draw:

    push r14
    push r15
    push rcx
    push rbx

    call draw_pixel

    pop rbx
    pop rcx
    pop r15
    pop r14

    inc r15d
    cmp r15d, 790
    jle .loop_x

    inc r14d
    cmp r14d, 590
    jle .loop_y

hlt
jmp show_loading_progress

kernel_idle_task:
hlt
jmp kernel_idle_task

kernel_not_found_text db 'OrangeOS could not start because the following file is missing or corrupt:',0
kernel_not_found_text_hint db 'Please re-install a copy of the above file.',0

fat_main_files dq 0
fat_filename db '00000000000',0
file_addr dq 0
kernel_file db 'OROSKRNL.OEX',0
orange_os_start db 'Starting OrangeOS...',0
current_progress dq 0
needed_progress dq 0
kernel_list dq 0

root_cd_buffer dq 0
setup_lba dq 0
setup_size dq 0


efer_bsod dd 0
is_kernel db 0