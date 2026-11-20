use16

start:

mov [loading_drive], dl
mov [loading_type], dh
mov [console_line], byte 0

mov ah, 0Fh
int 10h
mov ah, 00h
int 10h

mov bp, kernel_loaded
call print_line_16

mov ah, 01h
mov ch, 32
mov cl, 07h
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

clc

call check_memory_requirements

xor ax, ax
mov ds, ax
mov es, ax
mov di, 1A00h
xor ebx, ebx
xor bp, bp

detecting_ram:
    mov dword [es:di + 20], 1
    mov eax, 0E820h
    mov ecx, 24
    mov edx, 0x534D4150
    int 15h

    jc finished_get_ram
    cmp eax, 0x534D4150
    jne failed_get_ram

    cmp cl, 20
    jb .skip_entry

    cmp cl, 24
    jb .check_len
    test byte [es:di + 20], 1
    jz .skip_entry

.check_len:
    mov eax, [es:di + 8]
    or eax, [es:di + 12]
    jz .skip_entry

    add di, 24
    inc bp

.skip_entry:
    test ebx, ebx
    jnz detecting_ram

finished_get_ram:
    test bp, bp
    jz failed_get_ram

    mov word [1800h], 24
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

    mov word [drive_params], 1Eh

    mov ah, 48h
    mov dl, [loading_drive]
    mov si, drive_params
    int 13h

    jc .drive_params_error

    cmp [loading_type], 1
    je .hash_mbr

    mov ax, 0x0000
    mov es, ax
    mov di, 0x5000

    mov eax, 16

    call read_sector

    mov si, 0x5000
    mov cx, 2048
    call hash_pvd_16bit

    mov [boot_pvd_hash], eax

    jmp .hash_done

.hash_mbr:

mov si, 0x7C00
mov cx, 100
call hash_pvd_16bit

mov [boot_pvd_hash], eax

.hash_done:

    mov [is_setup], byte 1

    call gather_vbe_modes

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

read_sector:
    pushad
    mov dword [dap_lba], eax
    mov [dap_buffer_off], di
    mov [dap_buffer_seg], es 

    mov ah, 42h
    mov dl, [loading_drive]
    mov si, dap
    int 13h
    jc .error
    popad
    ret

.error:
    mov bp, drive_params_error_text
    call print_line_16
    hlt
    jmp $

set_videomode:

mov ax, 4F02h
mov bx, [vbe_mode]
or bx, 4000h
int 10h

cmp ah,00h
jne mode_set_error

jmp 0000h:r_start

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

failed_get_ram:

mov bp, failed_to_get_ram_count
call print_line_16

jmp $

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

check_memory_requirements:
    xor ebx, ebx
    mov dword [total_usable_ram], 0
    mov dword [total_usable_ram + 4], 0

.mmap_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    mov di, e820_buffer
    int 15h
    jc .mmap_done
    
    cmp eax, 0x534D4150
    jne .mmap_done
    
    test ecx, ecx
    jz .skip_entry
    
    cmp dword [e820_buffer.type], 1
    jne .skip_entry
    
    mov eax, [e820_buffer.len_low]
    mov edx, [e820_buffer.len_high]
    
    add dword [total_usable_ram], eax
    adc dword [total_usable_ram + 4], edx

.skip_entry:
    test ebx, ebx
    jnz .mmap_loop

.mmap_done:

    mov eax, dword [total_usable_ram]
    mov edx, dword [total_usable_ram+4]
    
    test edx, edx
    jnz .ram_is_ok
    
    cmp eax, 0x700000
    jae .ram_is_ok

    mov si, msg_newline
    call print_string_16
    
    mov si, msg_ram_error
    call print_string_16
    
    cli
.halt_loop:
    hlt
    jmp .halt_loop

.ram_is_ok:
    ret

print_string_16:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string_16
.done:
    ret

print_hex_32:
    pushad
    mov ecx, 8
    mov ebx, eax

.loop_hex:
    rol ebx, 4
    mov al, bl
    and al, 0x0F

    cmp al, 9
    jbe .is_digit
    add al, 'A' - 10 - '0'
.is_digit:
    add al, '0'

    mov ah, 0x0E
    push ebx
    xor bx, bx
    int 10h
    pop ebx
    
    loop .loop_hex

    popad
    ret

gather_vbe_modes:
    mov di, 0x4000
    xor cx, cx
    
    mov bx, 0x100

.loop_bruteforce:
    cmp bx, 0x3FF
    ja .done

    push cx
    push di
    push bx

    mov [tmp_mode_info], dword 0

    mov ax, 0x4F01
    mov cx, bx
    mov di, tmp_mode_info
    int 0x10

    pop bx
    pop di
    pop cx

    cmp ax, 0x004F
    jne .next_id

    mov al, [tmp_mode_info]          
    and al, 0x90
    cmp al, 0x90
    jne .next_id

    mov [di + 0], bx
    
    mov ax, [tmp_mode_info + 18]     ; width
    mov [di + 2], ax                 
    
    mov ax, [tmp_mode_info + 20]     ; height
    mov [di + 4], ax                 
    
    mov al, [tmp_mode_info + 25]     ; bpp
    mov [di + 6], al                 

    mov eax, [tmp_mode_info + 40]    ; LFB
    mov [di + 7], eax                

    add di, 11
    inc cx

.next_id:
    inc bx
    jmp .loop_bruteforce

.done:
    mov word [vbe_modes_count], cx
    ret

macro align16 { db (16 - ($ mod 16)) mod 16 dup 0x90 }

save_gdtr: dq 0, 0
save_idtr: dq 0, 0
save_rsp:  dq 0

align 4
real_mode_idtr:
    dw 0x03FF
    dd 0x00000000

use64
call_bios_int13:
    ; rdi = disk number
    ; rsi = DAP address

    cmp [loading_type], byte 1
    jne .continue_int13

    mov rax, [dap_structure.target_lba]
    shl rax, 2
    mov [dap_structure.target_lba], rax

    mov cx, [dap_structure.sectors_count]
    shl cx, 2
    mov [dap_structure.sectors_count], cx

.continue_int13:

    push rbp
    mov [save_rsp], rsp

    mov edx, edi
    mov ebx, esi

    push 0x38                       
    lea rax, [.protected_16]
    push rax
    retfq

use16
.protected_16:

    mov ax, 0x40
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov eax, cr0
    and eax, 0x7FFFFFFF
    mov cr0, eax

    xor eax, eax
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    and eax, not 0x00000100
    wrmsr

    mov eax, cr0
    and eax, 0x7FFFFFFE
    mov cr0, eax

    jmp 0x0000:.real_16

.real_16:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    lidt [real_mode_idtr]

    mov sp, 0x7C00

    sti

    mov si, bx
    mov ah, 0x42
    movzx edx, byte [loading_drive]
    int 0x13

    cli

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    db 0x66, 0xEA
    dd .protected_32
    dw 0x48

use32
.protected_32:
    mov ax, 0x50
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov eax, cr4
    or eax, 1 shl 5
    mov cr4, eax

    mov eax, dword [pt_begin]
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 shl 8
    wrmsr

    mov eax, cr0
    or eax, 1 shl 31
    mov cr0, eax

    jmp 0x08:.long_mode_64 

use64
.long_mode_64:
    lgdt [GDTR_64]
    LIDT [IDTR]

    mov rsp, [save_rsp]

    pop rbp

ret

align 4
tmp_vbe_info:   times 512 db 0
tmp_mode_info:  times 256 db 0

hex_chars db "0123456789ABCDEF"

vbe_not_supported_text db "FATAL ERROR: Your videocard or BIOS doesn't support VESA extensions.",0
vbe_not_video_mode db "FATAL ERROR: System unable to found 800x600x32 video mode on your videocard.",0
mode_set_error_text db "FATAL ERROR: Cannot set 800x600x32 video mode.",0
failed_to_get_ram_count db "FATAL ERROR: Cannot get count of RAM.",0
no_cpuid_text db "FATAL ERROR: Your processor doesn't support CPUID instruction. The system can not continue.",0
no_cpuid_extended_text db "FATAL ERROR: Your processor doesn't support CPUID extensions. The system can not continue.",0
no_long_mode_text db "FATAL ERROR: Your processor doesn't support x86-64 instructions. The system can not continue.",0
small_low_memory_text db "FATAL ERROR: OrangeOS has not found enough memory. 1MB required to run OrangeOS",0
kernel_loaded db "Setup is inspecting your computer's hardware configuration...",0
drive_params_error_text db "FATAL ERROR: Fail to get loading device information. The system can not continue.",0

console_line db 0
vbe_mode dw 0

align 4
dap:
dap_size        db 16
dap_reserved    db 0
dap_sectors     dw 1
dap_buffer_off  dw 0
dap_buffer_seg  dw 0
dap_lba         dq 0

align 4
e820_buffer:
    .base_low   dd 0
    .base_high  dd 0
    .len_low    dd 0
    .len_high   dd 0
    .type       dd 0
    .acpi       dd 0

total_usable_ram dq 0

msg_ram_error   db "OrangeOS has not found enough extended memory. 8Mb of extended", 13, 10, "memory is required to run OrangeOS. You may need to upgrade your computer.", 13, 10, 0
msg_mmap_header db "Memory Map:", 13, 10, 0
msg_hex_prefix  db "0x", 0
msg_space       db "  Type: ", 0
msg_newline     db 13, 10, 0
msg_len_separator db "  Len: 0x", 0
msg_type_separator db "  Type: ", 0

align 4
dap_structure:
    .size           db 0x10
    .reserved       db 0x00
    .sectors_count  dw 0x00
    .buffer_offset  dw 0x00
    .buffer_segment dw 0x00
    .target_lba     dq 0x00

loading_type db 0

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
vbe_modes_count dd 0
vbe_modes_list dq 0x4000