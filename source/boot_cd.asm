org 0x7C00

jmp short start
nop
times 8 - ($ - $$) db 0

bi_pvd          dd 0
bi_file         dd 0
bi_length       dd 0
bi_csum         dd 0
bi_reserved     db 40 dup (0)

boot_drive      db 0
root_lba        dd 0
root_size       dd 0

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    cld

    mov [boot_drive], dl

    mov ah, 0Fh
    int 10h
    mov ah, 00h
    int 10h

    mov bx, 0x3000

    mov ah, 02h
    mov al, 1
    mov ch, 0
    mov cl, 1
    mov dh, 0
    mov dl, 80h

    int 13h
    jc .load_from_cd

    mov ax, [0x31FE]
    cmp ax, 0xAA55
    jne .load_from_cd


.countdown:
    mov si, msg_press_key
    call print_string

    mov cx, 5

.countdown_loop:
    push cx 

    mov bl, 20
.delay_loop:
    mov ah, 01h
    int 16h
    jnz .key_pressed

    mov ah, 86h
    mov cx, 0
    mov dx, 0xC350
    int 15h

    dec bl
    jnz .delay_loop

    mov al, '.'
    call print_char

    pop cx
    loop .countdown_loop

    mov si, 0x7C00
    mov di, 0x0600
    mov cx, 256
    rep movsw

    jmp 0x0000:(.relocated - 0x7600)

.relocated:
    mov si, msg_newline
    call print_string

    mov si, 0x3000
    mov di, 0x7C00
    mov cx, 256
    rep movsw

    mov dl, 80h
    jmp 0000h:7c00h

.key_pressed:
    pop cx
    mov ah, 00h
    int 16h

.load_from_cd:

    mov si, msg_newline
    call print_string

    mov eax, [bi_pvd]
    mov di, 0x8000
    call read_sector

    mov eax, [0x8000 + 156 + 2]
    mov [root_lba], eax
    mov eax, [0x8000 + 156 + 10]
    mov [root_size], eax

    mov eax, [root_lba]
    mov di, 0x8000
    call read_sector

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
    hlt
    jmp $

.found:
    mov eax, [si + 2]
    mov ecx, [si + 10]

    add ecx, 2047
    shr ecx, 11

    mov di, 0x8000

.load_loop:
    push ecx
    push eax
    call read_sector
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
    mov dh, 0

    jmp 0x0000:0x8000

read_sector:
    pushad
    mov [dap_lba], eax
    mov [dap_buffer_off], di
    mov [dap_buffer_seg], es 

    mov ah, 42h
    mov dl, [boot_drive]
    mov si, dap
    int 13h
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
    call print_char
    jmp .loop
.done:
    popad
    ret 

print_char:
    mov ah, 0Eh
    mov bh, 00h
    int 10h
    ret 

reboot:
    xor ax, ax
    int 16h
    int 19h

filename        db "BOOTMGR", 0
msg_press_key   db "Press any key to boot from CD", 0
msg_missing     db "BOOTMGR is missing.", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0
msg_read_error  db "CD-ROM Read Error.", 13, 10, 0
msg_newline     db 13, 10, 0 

align 4
dap:
dap_size        db 16
dap_reserved    db 0
dap_sectors     dw 4
dap_buffer_off  dw 0
dap_buffer_seg  dw 0
dap_lba         dd 0 

times 2048 - ($ - $$) db 0