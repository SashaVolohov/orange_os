org 8000h

include "init_16.asm"
include "init_32.asm"

include "system_font.asm"
include "kernel_functions.asm"
include "exceptions.asm"
include "memory_manager.asm"
include "ata.asm"
include "system_timer.asm"
include "floppy.asm"
include "keyboard.asm"

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
call init_pit

LIDT [IDTR]

sti

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

mov r13, string_to_print
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text



mov r15, [mb_memory]
mov r14, ram_is
mov r13, 19
call format_10_string_with_64_int

mov r13, r14
mov r15d, 0
mov r14d, 20
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

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

mov r15, [r8]
mov r14, memory_map
mov r13, 0
call format_string_with_64_int

mov r15, [r8]
add r15, [r8+8]
mov r14, memory_map
mov r13, 21
mov r12d, 30
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call print_format_string_with_64_int

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

mov r15, 0
mov r15d, [lfb_buffer_addr]
mov r14, lfb_buffer
mov r13, 14
mov r12d, 0
mov r11d, 580
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call print_format_string_with_64_int

mov r15, [pt_end]
mov r14, page_table
mov r13, 18
mov r12d, 0
mov r11d, 560
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call print_format_string_with_64_int

mov r15, [ptv_end]
mov r14, page_v_table
mov r13, 24
mov r12d, 0
mov r11d, 540
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call print_format_string_with_64_int

mov r15, [bitmap_end]
mov r14, bitmap_end_text
mov r13, 18
mov r12d, 0
mov r11d, 520
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call print_format_string_with_64_int

call get_used_memory_mb

mov r15, rax
mov r14, used_mem_text
mov r13, 13
call format_10_string_with_64_int

mov r13, r14
mov r15d, 0
mov r14d, 480
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

mov rbx, [mb_memory]
sub rbx, rax

mov r15, rbx
mov r14, free_mem_text
mov r13, 13
call format_10_string_with_64_int

mov r13, r14
mov r15d, 0
mov r14d, 460
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text



call alloc_page
mov [mem_page], rax

mov r15, rax
mov r14, test_alloc_text
mov r13, 23
mov r12d, 0
mov r11d, 400
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call print_format_string_with_64_int

    mov rdi, 0x777777777000
    mov rsi, rax
    mov rdx, 111b
    call map_page

    test rax, rax
    jz .vmm_test_fail

    mov rax, 0xABCDEF0123456789
    mov rbx, 0x777777777000
    mov [rbx], rax

    xor rax, rax
    mov rax, [rbx]

    mov rbx, rax
    mov r15, rbx
    mov r14, test_vmm_text
    mov r13, 15
    mov r12d, 0
    mov r11d, 380
    mov cl, 0FFh
    mov bh, 0FFh
    mov bl, 0FFh
    mov ch, 0FFh
    call print_format_string_with_64_int
    
    jmp .vmm_test_done

.vmm_test_fail:
    jmp $

.vmm_test_done:

mov rdi, 0x777777777000
call unmap_page

mov al, 0xA0
mov rdi, [mem_page]
call get_drive_model_string

mov r13, [mem_page]
mov r15d, 0
mov r14d, 360
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp $

string_to_print db 'OrangeOS, x86-64 mode',0
ram_is db 'Total RAM count is 0x1111111111111111   MB. Memory map:',0
memory_map db '0x0000000000000000 - 0x0000000000000000',0
lfb_buffer db 'LFB buffer on 0x0000000000000000',0
page_table db 'Page table end on 0x0000000000000000',0
page_v_table db 'Page table video end on 0x0000000000000000',0
bitmap_end_text db 'Mem bitmap end on 0x0000000000000000',0
test_alloc_text db 'Test alloc page! Addr: 0x0000000000000000',0
test_vmm_text db 'Test VMM! RAX: 0x0000000000000000',0
used_mem_text db 'Used memory: 0x1111111111111111   MB.',0
free_mem_text db 'Free memory: 0x1111111111111111   MB.',0
mem_page dq 0

loading_drive db 0
lfb_buffer_addr dd 0

efer_bsod dd 0

vbe_hor_size dw 800
vbe_ver_size dw 600
vbe_color_depth db 32