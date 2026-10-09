use64

ATA_REG_DATA       equ 0x1F0
ATA_REG_FEATURES   equ 0x1F1
ATA_REG_SC_COUNT   equ 0x1F2
ATA_REG_LBA_LOW    equ 0x1F3
ATA_REG_LBA_MID    equ 0x1F4
ATA_REG_LBA_HIGH   equ 0x1F5
ATA_REG_DRV_SEL    equ 0x1F6
ATA_REG_COMMAND    equ 0x1F7
ATA_REG_STATUS     equ 0x1F7

asm_make_read12_packet:
;   Input: RDI = 12-byte buffer
;          ESI = LBA
;          EDX = count of sectors
    mov qword [rdi], 0
    mov dword [rdi + 8], 0

    mov byte [rdi + 0], 0xA8

    mov eax, esi
    bswap eax
    mov [rdi + 2], eax

    mov eax, edx
    bswap eax
    mov [rdi + 6], eax

ret

ata_io_wait:
    mov dx, ATA_REG_STATUS
    in al, dx
    in al, dx
    in al, dx
    in al, dx
    ret

ata_wait_drq: ; Output - al - register of status
    mov dx, ATA_REG_STATUS
.loop:
    in al, dx
    test al, 0x80
    jnz .loop
    test al, 0x08
    jz .loop
    ret

; Input: RDI = 12-bit SCSI command
atapi_send_packet:
    push rsi
    push rcx
    push rdx

    mov rsi, rdi
    mov dx, ATA_REG_DATA
    mov rcx, 6
    cld

.send_word:
    lodsw
    out dx, ax
    loop .send_word

    pop rdx
    pop rcx
    pop rsi
ret

read_atapi_sector: ; Input: RDI = buffer, RSI = LBA, RDX = count of sectors
    push rbp
    push rbx
    push rdi
    push rsi
    push rdx

    xor rbx, rbx
    mov [current_progress], rbx

    sub rsp, 16
    
    mov rdi, rsp 
    call asm_make_read12_packet

    mov dx, ATA_REG_DRV_SEL
    mov al, 0xB0    ; Slave (0xA0 = Master)
    out dx, al
    call ata_io_wait

    mov dx, ATA_REG_LBA_MID
    mov al, 0x00    
    out dx, al
    mov dx, ATA_REG_LBA_HIGH
    mov al, 0x08    
    out dx, al

    mov dx, ATA_REG_COMMAND
    mov al, 0xA0
    out dx, al
    call ata_io_wait

    call ata_wait_drq

    mov rdi, rsp        
    call atapi_send_packet

    add rsp, 16
    
    mov rdi, [rsp + 16]
    
    mov rbx, [rsp + 0]

.next_sector:
    call ata_wait_drq

    mov dx, ATA_REG_DATA
    mov rcx, 1024   
    cld
    rep insw

    add [current_progress], 1

    dec rbx

    jnz .next_sector

    pop rdx
    pop rsi
    pop rdi
    pop rbx
    pop rbp
ret