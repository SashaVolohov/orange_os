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

mov rsi, setup_loading_memory
call draw_status_bar_long_mode

call memory_init

mov rax, [pt_begin]
cmp rax, 100000h

je .more_than_128
mov [ptv_end], 100000h
jmp .allocator_create

.more_than_128:
mov rax, [pt_end]
mov [ptv_end], rax

.allocator_create:

call allocator_init

mov rsi, setup_loading_pic
call draw_status_bar_long_mode

call init_pic

mov rsi, setup_loading_idt
call draw_status_bar_long_mode

LIDT [IDTR]

mov rsi, setup_loading_com1
call draw_status_bar_long_mode
call com1_init

mov rsi, setup_loading_com2
call draw_status_bar_long_mode
call com2_init

mov rsi, setup_loading_rtc
call draw_status_bar_long_mode
call init_rtc

mov rsi, setup_loading_e820
call draw_status_bar_long_mode

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

mov rsi, setup_loading_heap
call draw_status_bar_long_mode

call kheap_init

mov rsi, setup_loading_scheduler
call draw_status_bar_long_mode

call tasking_init

mov rsi, setup_loading_tss
call draw_status_bar_long_mode

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

mov word [rbx + TSS64_iomap_base], 104

call install_tss_descriptor

lgdt [GDTR_64]

mov ax, 0x28
ltr ax

mov rsi, setup_loading_acpi
call draw_status_bar_long_mode

call init_acpi

mov rsi, setup_loading_apic
call draw_status_bar_long_mode
call init_apic

mov rsi, setup_loading_lapic
call draw_status_bar_long_mode

call calibrate_lapic_timer
call init_lapic_timer

mov rsi, setup_loading_ioapic
call draw_status_bar_long_mode

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

mov rsi, setup_loading_cdfs
call draw_status_bar_long_mode

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

mov rsi, setup_loading_kernel
call draw_status_bar_long_mode

mov rdi, [root_cd_buffer]
mov rsi, kernel_file
mov rdx, 12

call iso9660_find_entry

test rax, rax
jz kernel_not_found

mov [setup_lba], rax
mov [setup_size], rcx

mov r15, rcx
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
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

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

mov rsi, setup_starting_kernel
call draw_status_bar_long_mode

mov rax, [mb_memory]
mov rbx, [pt_begin]
mov rcx, [pt_end]
mov rdx, [ptv_end]
mov r8, [bitmap_end]
mov r9d, 0
mov r10, [lapic_base_addr]
mov r11, [timer_ticks]
mov r12, [process_table_ptr]
mov r13, [kheap_current_end]

jmp r15

kernel_not_found:
cli

mov rsi, kernel_not_found_text
call draw_status_bar_long_mode

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

draw_status_bar_long_mode:
    push rsi
    push rdi
    push rcx
    push rax

    mov rdi, 0xB8F00    
    
    mov rcx, 80
    mov ah, 70h

.text_loop:
    mov al, [rsi]
    inc rsi
    cmp al, 0
    je .fill_loop

    mov [rdi], ax
    add rdi, 2
    dec rcx
    jz .done
    jmp .text_loop

.fill_loop:
    mov al, ' '
.fill_loop_run:
    cmp rcx, 0
    je .done
    mov [rdi], ax
    add rdi, 2
    dec rcx
    jmp .fill_loop_run

.done:
    pop rax
    pop rcx
    pop rdi
    pop rsi
ret


kernel_not_found_text db 'FATAL ERROR: Setup cannot load kernel into memory, file missing or corrupt.',0

setup_loading_memory db 'Setup is preparing <Memory Allocator>...',0
setup_loading_pic db 'Setup is preparing <Programmable Interrupt Controller>...',0
setup_loading_idt db 'Setup is preparing <Interrupt Descriptor Table>...',0
setup_loading_com1 db 'Setup is preparing <Communication port 1>...',0
setup_loading_com2 db 'Setup is preparing <Communication port 2>...',0
setup_loading_rtc db 'Setup is preparing <Real-Time Clock>...',0
setup_loading_e820 db 'Setup is preparing <Memory Map>...',0
setup_loading_heap db 'Setup is preparing <Kernel Heap>...',0
setup_loading_scheduler db 'Setup is preparing <OrangeOS Scheduler>...',0
setup_loading_tss db 'Setup is preparing <Task State Segment>...',0
setup_loading_acpi db 'Setup is preparing <Advanced Configuration and Power Interface>...',0
setup_loading_apic db 'Setup is preparing <Advanced Programmable Interrupt Controller>...',0
setup_loading_lapic db 'Setup is preparing <Local Advanced Programmable Interrupt Controller>...',0
setup_loading_ioapic db 'Setup is preparing <Input/Output Advanced Programmable Interrupt Controller>...',0
setup_loading_cdfs db 'Setup is preparing <Compact Disc File System>...',0
setup_loading_kernel db 'Setup is loading <OrangeOS Kernel>...',0
setup_starting_kernel db 'Setup is starting OrangeOS',0

fat_main_files dq 0
fat_filename db '00000000000',0
file_addr dq 0
kernel_file db 'OROSKRNL.OEX',0
current_progress dq 0
needed_progress dq 0
kernel_list dq 0

root_cd_buffer dq 0
setup_lba dq 0
setup_size dq 0


efer_bsod dd 0
is_kernel db 0