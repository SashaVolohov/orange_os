use64

public ata_detect_drive
public read_ata
public write_ata

public ata_irq14_fired
public ata_irq15_fired

; Read from First Primary Master
read_ata:
    push rbx
    push rbp

    mov rbx, rdi
    mov rdi, rsi
    mov r8, rdx
    mov r9, rcx

    mov dx, 0x1F2
    mov al, r8b
    out dx, al

    mov rax, rbx
    mov dx, 0x1F3
    out dx, al

    shr rax, 8
    mov dx, 0x1F4
    out dx, al

    shr rax, 8
    mov dx, 0x1F5
    out dx, al

    shr rax, 8
    and al, 0x0F
    
    shl r9b, 4          ; 0 -> 0x00, 1 -> 0x10
    or al, r9b
    or al, 0xE0
    mov dx, 0x1F6
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

.loop_sectors:
    cmp r8, 0
    jz .done

.wait_ready:
    mov dx, 0x1F7
    in al, dx
    test al, 0x80
    jnz .wait_ready
    test al, 0x08
    jz .wait_ready

    mov rcx, 256
    mov dx, 0x1F0

.read_words:
    in ax, dx
    mov [rdi], ax
    add rdi, 2
    dec rcx
    jnz .read_words

    dec r8
    jmp .loop_sectors

.done:
    pop rbp
    pop rbx
    ret

write_ata:
    ; Input - RDI = lba
    ; RSI = buffer
    ; RDX = sector_count
    ; RCX = drive_select

    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    mov al, dl
    mov dx, 0x1F2
    out dx, al

    mov rdx, [rsp + 16]

    mov al, dil
    mov dx, 0x1F3
    out dx, al

    mov rax, rdi
    shr rax, 8
    mov dx, 0x1F4
    out dx, al

    mov rax, rdi
    shr rax, 16
    mov dx, 0x1F5
    out dx, al

    mov rax, rdi
    shr rax, 24
    and al, 0x0F
    
    or al, cl
    or al, 0x40
    
    mov dx, 0x1F6
    out dx, al

    mov al, 0x30
    mov dx, 0x1F7
    out dx, al

    mov rbx, [rsp + 16]
    movzx rbx, bx

.sector_loop:
    mov dx, 0x1F7
.wait_bsy:
    in al, dx
    test al, 0x80
    jnz .wait_bsy
.wait_drq:
    in al, dx
    test al, 0x08
    jz .wait_drq

    mov rcx, 256
    mov dx, 0x1F0
.write_data:
    lodsw
    out dx, ax
    loop .write_data

    dec rbx
    jnz .sector_loop

.done:
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
ret

detect_drive_type: ; AL - 0xA0(Master), 0xB0(Slave)
                   ; Output - RAX - Type: 0 - no device, 1 - ATA, 2 - ATAPI, RDX: count of sectors
    push rcx
    push rbx

    mov rbx, rax

    mov dx, 0x1F6
    mov al, bl
    out dx, al
    
    mov dx, 0x3F6
    in al, dx
    in al, dx
    in al, dx
    in al, dx

    mov dx, 0x1F7
    in al, dx
    cmp al, 0xFF
    je .no_device

    mov dx, 0x1F7
    mov al, 0xEC
    out dx, al

.wait_ata:
    in al, dx
    test al, 0x80
    jnz .wait_ata

    in al, dx
    test al, 0x01
    jnz .try_atapi
    
    test al, 0x08
    jz .try_atapi

    sub rsp, 512
    mov rdi, rsp

    mov rcx, 256
    mov dx, 0x1F0
.read_buffer:
    in ax, dx
    mov [rdi], ax
    add rdi, 2
    loop .read_buffer
    
    mov ax, [rsp + 83 * 2]
    test ax, 0x0400
    jz .use_lba28

.use_lba48:
    mov rdx, [rsp + 100 * 2]
    jmp .end_parsing

.use_lba28:
    mov eax, [rsp + 60 * 2]
    mov rdx, rax

.end_parsing:
    add rsp, 512

    mov rax, 1
    jmp .done

.try_atapi:
    mov dx, 0x1F7
    mov al, 0xA1
    out dx, al

.wait_atapi:
    in al, dx
    test al, 0x80
    jnz .wait_atapi

    in al, dx
    test al, 0x01
    jnz .no_device

    test al, 0x08
    jz .no_device

    mov rcx, 256
    mov dx, 0x1F0
.clear_atapi_buffer:
    in ax, dx
    loop .clear_atapi_buffer

    mov rax, 2
    mov rdx, 0
    jmp .done

.no_device:
    mov rax, 0
    mov rdx, 0

.done:
    pop rbx
    pop rcx
ret

ata_detect_drive:
    mov al, dil
    call detect_drive_type
ret

check_primary_ide:

    mov dx, 0x1F2
    mov al, 0x5A
    out dx, al
    
    out 0x80, al        

    mov dx, 0x1F3
    mov al, 0xA5
    out dx, al
    
    out 0x80, al
    
    mov dx, 0x1F2
    in al, dx
    cmp al, 0x5A
    jne .not_found

    mov dx, 0x1F3
    in al, dx
    cmp al, 0xA5
    jne .not_found

    mov rax, 1
    ret

.not_found:
    mov rax, 0
ret

get_drive_model_string: ; Input - AL(0xA0 - Master, 0xB0 - Slave), RDI - string buffer
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    mov bl, al

    sub rsp, 512
    mov rdi, rsp

    mov dx, 0x1F6
    mov al, bl
    out dx, al

    mov dx, 0x1F7
    mov al, 0xEC
    out dx, al

.wait_ready:
    in al, dx
    test al, 0x80
    jnz .wait_ready
    in al, dx
    test al, 0x08
    jz .wait_ready

    mov rcx, 256
    mov dx, 0x1F0
.read_loop:
    in ax, dx
    mov [rdi], ax
    add rdi, 2
    loop .read_loop

    mov rsi, rsp
    add rsi, 54
    
    mov rdi, [rsp + 512]

    mov rcx, 20
.swap_and_copy:
    mov al, [rsi]
    mov ah, [rsi + 1]
    
    mov [rdi], ah
    mov [rdi + 1], al
    
    add rsi, 2
    add rdi, 2
    loop .swap_and_copy

    mov byte [rdi], 0

    add rsp, 512

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
ret

vector_handler_46:

push rax
push rdx
push rsi

mov dx, 0x1F7
in al, dx

mov [ata_irq14_fired], byte 1

mov rsi, [lapic_base_addr]
mov dword [rsi + 0xB0], 0

pop rsi
pop rdx
pop rax

iretq

vector_handler_47:

push rax
push rdx
push rsi

mov dx, 0x177
in al, dx

mov [ata_irq15_fired], byte 1

mov rsi, [lapic_base_addr]
mov dword [rsi + 0xB0], 0

pop rsi
pop rdx
pop rax

iretq

ata_irq14_fired db 0
ata_irq15_fired db 0