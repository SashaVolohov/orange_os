org 7c00h

start:

mov [loading_drive], dl

mov ah, 00h
mov al, 02h
int 10h

mov ah, 02h
mov bh, 0
mov dh, 0
mov dl, [console_line]
int 10h

mov [di], dword "VBE2"

mov al, 00h
mov ah, 4Fh
mov di, 1000h
int 10h

cmp al, 4fh
je jbe_supported
jmp jbe_not_supported

jbe_supported:

video_found:

mov ax, 0000h
mov es, ax
mov di, 1500h
mov ax, 4F01h
mov cx, [vbe_mode]
or cx, 4000h
int 10h

cmp ah, 00h
jne search_0

mov ax, [di+00h]
and ax, 40h

cmp ax, 40h
je search_0

mov ax, [di+12h]
cmp ax, [vbe_hor_size]
jz search_1
jmp search_0

search_1:
mov ax, [di+14h]
cmp ax, [vbe_ver_size]
jz search_2
jmp search_0

search_2:
mov al, [di+19h]
cmp al, [vbe_color_depth]
jz search_ended

search_0:
inc [vbe_mode]
mov ax, 00h
cmp ax, [vbe_mode]
jz vbe_error
jmp video_found

jmp while_loop

search_ended:

mov ah, 02h
mov dl, [loading_drive]
mov dh, 0
mov ch, 0
mov cl, 2
mov al, 31
mov bx, 3000h
int 13h

cmp ah,00h
je kernel_loaded
jmp kernel_load_error

kernel_loaded:

mov dl, [loading_drive]
mov dh, [console_line]
mov cx, [vbe_mode]
mov esi, [di+28h]

jmp 3000h

kernel_load_error:

mov bp, kernel_error_load_text
call print_line

jmp while_loop

vbe_error:

mov bp, vbe_not_video_mode
call print_line

jmp while_loop

jbe_not_supported:

mov bp, vbe_not_supported_text
call print_line

jmp while_loop

print_line:

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

while_loop:
jmp while_loop

vbe_not_supported_text db "FATAL ERROR: Your videocard or BIOS doesn't support VESA extensions.",0
vbe_not_video_mode db "FATAL ERROR: System unable to found 800x600x32 video mode on your videocard.",0
kernel_error_load_text db "FATAL ERROR: Unable to read kernel file from loading drive.", 0
console_line db 0
loading_drive db 0

vbe_hor_size dw 800
vbe_ver_size dw 600
vbe_color_depth db 32
vbe_mode dw 0

times 510-($-7c00h) db 0
db 055h, 0AAh