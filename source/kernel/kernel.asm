org 3000h

include "init_16.asm"
include "init_32.asm"

include "system_font.asm"
include "kernel_functions.asm"
include "exceptions.asm"
include "memory_manager.asm"

long_start:

mov ax, 10h
mov ds, ax
mov es, ax
mov fs, ax
mov ss, ax
mov esp, 10000h
mov gs, ax

call memory_init
call videomem_init

LIDT [IDTR]

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
mov r15d, 400
mov r14d, 300
mov cl, 0FFh
mov bh, 00h
mov bl, 00h
mov ch, 0FFh
call draw_text

mov [7FFFFFFFh], byte 255

jmp $

string_to_print db 'OrangeOS',0

loading_drive db 0
lfb_buffer_addr dd 0

efer_bsod dd 0

vbe_hor_size dw 800
vbe_ver_size dw 600
vbe_color_depth db 32