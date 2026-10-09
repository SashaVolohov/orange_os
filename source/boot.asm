org 0x0600

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, 0x7C00
    mov di, 0x0600
    mov cx, 256
    rep movsw

    jmp 0x0000:relocated

relocated:

    mov [drive_number], dl

    mov si, 0x07BE
    mov cx, 4

.find_active:
    cmp byte [si], 0x80
    je .found_active
    add si, 16
    loop .find_active

    mov si, msg_no_active
    call print_string
    jmp $

.found_active:

    mov eax, [si + 8]
    mov [dap_lba], eax

    mov word [0x0500], 0x0010
    mov word [0x0502], 0x0001
    mov word [0x0504], 0x7C00
    mov word [0x0506], 0x0000
    
    mov [0x0508], eax
    mov dword [0x050C], 0

    mov ah, 0x42
    mov dl, [drive_number]
    mov si, 0x0500
    int 13h
    jc .read_error

    cmp word [0x7DFE], 0xAA55
    jne .invalid_vbr

    mov dl, [drive_number]
    jmp 0x0000:0x7C00

.read_error:
    mov si, msg_error
    call print_string
    jmp $

.invalid_vbr:
    mov si, msg_invalid
    call print_string
    jmp $

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

align 4
dap:
    db 0x10
    db 0x00
    dw 0x0001
    dw 0x7C00
    dw 0x0000
dap_lba:
    dq 0x0000000000000000

msg_no_active db "Active partition not found!", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0
msg_error     db "A disk read error occurred", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0
msg_invalid   db "Invalid VBR signature", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0

drive_number db 0

times 446-($-$$) db 0

times 64 db 0

dw 0xAA55