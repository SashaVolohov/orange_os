use16

start:

mov [loading_drive], dl
mov [console_line], byte 0

mov ah, 0Fh
int 10h
mov ah, 00h
int 10h

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

jbe_not_supported:

mov bp, vbe_not_supported_text
call print_line_16

jmp while_loop

while_loop:
jmp while_loop

vbe_error:

mov bp, vbe_not_video_mode
call print_line_16

jmp while_loop

search_ended:

mov cx, [vbe_mode]
mov ax, [di+10h]
mov esi, [di+28h]

mov [vbe_mode], cx
mov [vbe_pitch], ax
mov [lfb_buffer_addr], esi

mov bp, kernel_loaded
call print_line_16

clc
int 12h

jc failed_get_ram

cmp ax, 630
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

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 shl 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    xor eax, ecx
    jz .no_cpuid

    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000001
    jb      .no_cpuid_extended

    mov     eax, 0x80000001
    cpuid

    bt      edx, 29
    jnc     .no_long_mode

    mov word [drive_params], 42h

    mov ah, 48h
    mov dl, [loading_drive]
    mov si, drive_params
    int 13h

    jc .drive_params_error

    mov si, 0x7c00
    mov cx, 100
    call hash_pvd_16bit

    mov [boot_pvd_hash], eax

    jmp set_videomode

.drive_params_error:
    mov bp, drive_params_error_text
    call print_line_16
    cli
    hlt

.no_cpuid:
    mov bp, no_cpuid_text
    call print_line_16
    cli
    hlt

.no_cpuid_extended:
    mov bp, no_cpuid_extended_text
    call print_line_16
    cli
    hlt

.no_long_mode:
    mov bp, no_long_mode_text
    call print_line_16
    cli
    hlt

set_videomode:

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

; SI - buffer, CX - count of bytes
; EAX - checksum
hash_pvd_16bit:
    push cx
    push si
    mov eax, 2166136261
.loop:
    xor edx, edx
    mov dl, [si]
    xor eax, edx
    imul eax, 16777619
    inc si
    loop .loop
    pop si
    pop cx
ret

vbe_not_supported_text db "FATAL ERROR: Your videocard or BIOS doesn't support VESA extensions.",0
vbe_not_video_mode db "FATAL ERROR: System unable to found 800x600x32 video mode on your videocard.",0
mode_set_error_text db "FATAL ERROR: Cannot set 800x600x32 video mode.",0
failed_to_get_ram_count db "FATAL ERROR: Cannot get count of RAM.",0
no_cpuid_text db "FATAL ERROR: Your processor doesn't support CPUID instruction. The system can not continue.",0
no_cpuid_extended_text db "FATAL ERROR: Your processor doesn't support CPUID extensions. The system can not continue.",0
no_long_mode_text db "FATAL ERROR: Your processor doesn't support x86-64 instructions. The system can not continue.",0
small_low_memory_text db "FATAL ERROR: OrangeOS has not found enough memory. 1MB required to run OrangeOS",0
kernel_loaded db "OrangeOS Kernel: starting...",0
drive_params_error_text db "FATAL ERROR: Fail to get loading device information. The system can not continue.",0

console_line db 0
vbe_mode dw 0

boot_info:

vbe_hor_size dw 800
vbe_ver_size dw 600
vbe_color_depth db 32
vbe_pitch dw 0

loading_drive db 0
lfb_buffer_addr dd 0

drive_params dq 0, 0, 0, 0, 0, 0, 0, 0, 0
boot_pvd_hash dd 0
cpu_count dd 0
cpus_temp_array rb 256
bsp_factory_ist1 dq 0
is_setup db 0