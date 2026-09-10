use64

; Read from First Primary Master
read_sectors_ata_pio: ; Input: RAX - LBA, RDI - addr buffer, cx - count of sectors
    push rbx
    push rcx
    push rdx

    mov rbx, rax
    mov dx, 0x1F2
    mov al, cl
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
    or al, 0xE0
    mov dx, 0x1F6
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

.loop_sectors:
    push rcx

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
    loop .read_words

    pop rcx
    loop .loop_sectors

    pop rdx
    pop rcx
    pop rbx

ret

; Write to First Primary Master
write_sectors_ata_pio:; RAX - LBA, RSI - addr for data, cx - count of sectors
    push rbx
    push rcx
    push rdx

    mov rbx, rax
    mov dx, 0x1F2
    mov al, cl
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
    or al, 0xE0
    mov dx, 0x1F6
    out dx, al

    mov dx, 0x1F7
    mov al, 0x30
    out dx, al

.loop_sectors:
    push rcx

.wait_ready:
    mov dx, 0x1F7
    in al, dx
    test al, 0x80
    jnz .wait_ready
    test al, 0x08
    jz .wait_ready

    mov rcx, 256
    mov dx, 0x1F0

.write_words:
    mov ax, [rsi]
    out dx, ax
    add rsi, 2
    loop .write_words

    mov dx, 0x1F7
    in al, dx

    pop rcx
    loop .loop_sectors

.wait_flush:
    in al, dx
    test al, 0x80
    jnz .wait_flush

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