use64

public read_atapi_sector

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
    mov rbp, rsp
    
    push rbx
    push r12
    push r13

    mov r12, rdi
    mov r13, rdx

    xor rbx, rbx
    mov [current_progress], rbx

    sub rsp, 16
    
    mov rdi, rsp 
    call asm_make_read12_packet

    mov dx, ATA_REG_DRV_SEL ; Slave
    mov al, 0xB0    
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

.next_sector:
    test r13, r13
    jz .reading_done

    call ata_wait_drq

    mov dx, ATA_REG_DATA
    mov rcx, 1024   
    mov rdi, r12
    cld
    rep insw

    mov r12, rdi
    add [current_progress], 1

    dec r13
    jmp .next_sector

.reading_done:
    pop r13
    pop r12
    pop rbx
    pop rbp
ret