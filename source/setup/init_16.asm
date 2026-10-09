use16

start:

mov [loading_drive], dl
mov [console_line], byte 0

mov ah, 0Fh
int 10h
mov ah, 00h
int 10h

mov ah, 01h
mov ch, 20h
mov cl, 00h
int 10h

mov bp, orange_os_start
call print_line_16

mov ah, 41h
mov bx, 55aah
mov dl, [loading_drive]
int 13h

jc .no_extensions

cmp bx, 0aa55h
jne .no_extensions

test cx, 1
jz .no_lba

jmp .continue

.no_extensions:
    mov bp, no_disk_extensions
    call print_line_16
    hlt
    jmp $

.no_lba:
    mov bp, no_lba_extensions
    call print_line_16
    hlt
    jmp $

.drive_params_error:
    mov bp, drive_params_error_text
    call print_line_16
    hlt
    jmp $

.continue:

mov word [drive_params], 1Ah

mov ah, 48h
mov dl, [loading_drive]
mov si, drive_params
int 13h

jc .drive_params_error

mov eax, dword [drive_params + 10h]
mov edx, dword [drive_params + 14h]

; EDX:EAX теперь содержат полный объём диска в секторах.

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

    jmp set_videomode

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

mov ah, 06h
mov al, 00h
mov bh, 1Fh
mov ch, 00h
mov cl, 00h
mov dh, 24
mov dl, 79
int 10h

mov si, orange_os_setup
mov dh, 1
mov dl, 0
mov bl, 1Fh

call print_string

mov si, switch_to_protected_mode
call draw_status_bar

jmp 0000h:r_start

failed_get_ram:

mov bp, failed_to_get_ram_count
call print_line_16

hlt
jmp $

small_low_memory:

mov bp, small_low_memory_text
call print_line_16

jmp $

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

print_string: ;   SI - string
              ;   DH -  Y (строка 0-24)
              ;   DL - X (столбец 0-79)
              ;   BL - color - 1Fh(blue, white)
    pusha

    mov di, si
    xor cx, cx 
.loop_width:
    cmp byte [di], 0
    je .width_done
    inc di
    inc cx
    jmp .loop_width
.width_done:
    jcxz .exit

    mov ah, 13h
    mov al, 01h
    mov bh, 00h
   
    mov bp, si
    int 10h

.exit:
    popa
ret

draw_status_bar: ; SI - string
    pusha
    push es

    mov ax, 0B800h
    mov es, ax

    mov di, 3840
    
    mov cx, 80
    mov ah, 70h

.text_loop:
    lodsb
    cmp al, 0
    je .fill_loop

    stosw
    dec cx
    jcxz .done
    jmp .text_loop

.fill_loop:
    mov al, ' '
.fill_loop_run:
    stosw
    loop .fill_loop_run

.done:
    pop es
    popa
ret

vbe_not_supported_text db "FATAL ERROR: Your videocard or BIOS doesn't support VESA extensions.",0
vbe_not_video_mode db "FATAL ERROR: System unable to found 800x600x32 video mode on your videocard.",0
failed_to_get_ram_count db "FATAL ERROR: Cannot get count of RAM.",0
no_cpuid_text db "FATAL ERROR: Your processor doesn't support CPUID instruction. The system can not continue.",0
no_cpuid_extended_text db "FATAL ERROR: Your processor doesn't support CPUID extensions. The system can not continue.",0
no_long_mode_text db "FATAL ERROR: Your processor doesn't support x86-64 instructions. The system can not continue.",0
small_low_memory_text db "FATAL ERROR: OrangeOS has not found enough memory. 1MB required to run OrangeOS",0
kernel_loaded db "OrangeOS Kernel: starting...",0
orange_os_start db "Setup is inspecting your computer's hardware configuration...",0
no_disk_extensions db "FATAL ERROR: Your BIOS doesn't support BIOS disk extensions. The system can not continue.",0
no_lba_extensions db "FATAL ERROR: Your BIOS doesn't support BIOS disk LBA. The system can not continue.",0
drive_params_error_text db "FATAL ERROR: Fail to get loading device information. The system can not continue.",0
orange_os_setup db "OrangeOS Setup",0xD, 0xA, "====================",0
switch_to_protected_mode db 'Setup is switching to protected mode...',0

console_line db 0
vbe_mode dw 0

vbe_hor_size dw 800
vbe_ver_size dw 600
vbe_color_depth db 32

loading_drive db 0
lfb_buffer_addr dd 0
drive_params dq 0, 0, 0, 0