use16

start:

mov [loading_drive], dl
mov [console_line], dh
mov [vbe_mode], cx
mov [lfb_buffer_addr], esi

mov bp, kernel_loaded
call print_line_16

clc
int 12h

jc failed_get_ram

cmp ax, 639
jb small_low_memory

mov ebx, 0
mov di, 1A00h

detecting_ram:

mov eax, 0E820h
mov ecx, 24
mov edx, 0x534D4150
int 15h

cmp eax, 0x534D4150
jne failed_get_ram

mov ch, 0
add di, cx

mov [1800h], cx

cmp ebx, 0
jne detecting_ram

mov [1900h], di

mov ax, 4F02h
mov bx, [vbe_mode]
or cx, 4000h
int 10h

cmp ah,00h
jne mode_set_error

jmp 0000h:r_start

failed_get_ram:

mov bp, failed_to_get_ram_count
call print_line_16

jmp $

small_low_memory:

mov bp, small_low_memory_text
call print_line_16

r_start:

cli

mov ax, 0000h
mov ds, ax
mov es, ax

in al, 0x92
or al, 2
out 0x92, al

in al, 70h
or al, 0x80
out 70h, al

lgdt fword [GDTR]
mov eax, cr0
or al, 1
mov cr0, eax

jmp fword 08h:startup32;

mode_set_error:

mov bp, mode_set_error_text
call print_line_16

jmp while_loop_16

while_loop_16:
jmp while_loop_16

print_line_16:

mov bh, 0
mov dh, [console_line]
mov dl, 0

print:

push bx

mov al, [bp]
mov bh, 0
mov bl, 07h
mov cx, 1
mov ah, 09h
int 10h

pop bx

mov ah, 02h
add dl, 01h
int 10h


inc bp

cmp [bp],byte 00h
jne print

add [console_line], 1

mov ah, 02h
mov bh, 0
mov dh, [console_line]
mov dl, 0
int 10h

ret

mode_set_error_text db "FATAL ERROR: Cannot set 800x600x32 video mode.",0
failed_to_get_ram_count db "FATAL ERROR: Cannot get count of RAM.",0
small_low_memory_text db "FATAL ERROR: OrangeOS has not found enough memory. 1MB required to run OrangeOS",0
kernel_loaded db "OrangeOS Kernel: starting...",0

console_line db 0
vbe_mode dw 0