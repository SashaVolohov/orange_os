org 0x7C00
use16

start:
    cli
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    cld
    sti

    jmp 0x0000:.norm_cs
.norm_cs:
    mov [boot_drive], dl

    mov ax, 0x0003
    int 0x10

    mov eax, 16
    mov di, 0x8000
    call read_iso_block

    cmp dword [0x8001], 'CD00'
    jne .not_found

    mov eax, [0x8000 + 156 + 2]
    mov [root_lba_2048], eax
    mov eax, [0x8000 + 156 + 10]
    mov [root_size], eax

    mov eax, [root_lba_2048]
    mov di, 0x8000
    call read_iso_block

    mov si, 0x8000

.search_loop:
    movzx bx, byte [si]
    cmp bx, 0
    je .not_found

    mov cl, [si + 32]
    cmp cl, 7
    jb .next_record

    push si
    add si, 33
    mov di, filename
    mov cx, 7
    repe cmpsb
    pop si
    je .found

.next_record:
    add si, bx
    mov ax, si
    sub ax, 0x8000
    cmp ax, 2048
    jl .search_loop

.not_found:
    mov si, msg_missing
    call print_string
    jmp reboot

.found:
    mov eax, [si + 2]
    mov ecx, [si + 10]

    add ecx, 2047
    shr ecx, 11

    mov di, 0x8000

.load_loop:
    push ecx
    push eax
    call read_iso_block
    pop eax
    pop ecx
    inc eax

    mov bx, es
    add bx, 0x80
    mov es, bx
    loop .load_loop

    xor ax, ax
    mov es, ax

    mov dl, [boot_drive]
    mov dh, 1

    jmp 0x0000:0x8000

read_iso_block:
    pushad
    shl eax, 2
    mov [dap_lba], eax
    mov [dap_buffer_off], di
    mov [dap_buffer_seg], es

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc .error
    popad
    ret

.error:
    mov si, msg_read_error
    call print_string
    jmp reboot

print_string:
    pushad
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp .loop
.done:
    popad
    ret

reboot:
    xor ax, ax
    int 0x16
    int 0x19

filename        db "BOOTMGR", 0
msg_missing     db "BOOTMGR is missing.", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0
msg_read_error  db "USB CDFS Read Error.", 13, 10, 0

boot_drive      db 0
root_lba_2048   dd 0
root_size       dd 0

align 4
dap:
dap_size        db 16
dap_reserved    db 0
dap_sectors     dw 4
dap_buffer_off  dw 0
dap_buffer_seg  dw 0
dap_lba         dd 0
dap_lba_high    dd 0

times 446 - ($ - $$) db 0
db 0x80
db 0x00, 0x01, 0x00
db 0x17
db 0xFE, 0xFF, 0xFF
dd 0
dd 0x00100000
times 510 - ($ - $$) db 0
dw 0xAA55