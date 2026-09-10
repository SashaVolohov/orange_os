DMA_MASK_REG   equ 0x0A
DMA_MODE_REG   equ 0x0B
DMA_FLIP_FLOP  equ 0x0C
DMA_CH2_ADDR   equ 0x04
DMA_CH2_COUNT  equ 0x05
DMA_PAGE_CH2   equ 0x81

FDC_MSR        equ 0x3F4
FDC_FIFO       equ 0x3F5
FDC_DOR        equ 0x3F2

FLOPPY_SPT   equ 18
FLOPPY_HEADS equ 2

fdc_irq_fired db 0

fdc_current_cylinder db 0xFF

use64

lba_to_chs: ; Input - RAX(LBA)
            ; Output - CH - Cylinder, DH - Head, CL - Sector

    push rbx
    
    xor rdx, rdx
    mov rbx, 18
    div rbx

    inc rdx
    mov r8, rdx

    xor rdx, rdx
    mov rbx, 2
    div rbx
    
    mov ch, al
    mov cl, r8b
    
    mov dh, dl
    mov dl, 0

    pop rbx
ret

init_dma_read:
    mov al, 0x06
    out DMA_MASK_REG, al

    mov al, 0x46
    out DMA_MODE_REG, al

    out DMA_FLIP_FLOP, al

    mov al, 0x00
    out DMA_CH2_ADDR, al
    mov al, 0x30
    out DMA_CH2_ADDR, al
    mov al, 0x00
    out DMA_PAGE_CH2, al

    out DMA_FLIP_FLOP, al

    mov al, 0xFF
    out DMA_CH2_COUNT, al
    mov al, 0x01
    out DMA_CH2_COUNT, al

    mov al, 0x02
    out DMA_MASK_REG, al
ret

init_dma_write:
    push rax
    mov al, 0x06
    out 0x0A, al

    mov al, 0x4A
    out 0x0B, al

    xor al, al
    out 0x0C, al
    mov al, 0x00
    out 0x04, al
    mov al, 0x30
    out 0x04, al
    mov al, 0x00
    out 0x81, al

    xor al, al
    out 0x0C, al
    mov al, 0xFF
    out 0x05, al
    mov al, 0x01
    out 0x05, al

    mov al, 0x02
    out 0x0A, al
    pop rax
ret

fdc_wait_write:
push rdx
push rax
    mov dx, FDC_MSR
.loop:
    in al, dx
    and al, 0xC0
    cmp al, 0x80
    jne .loop
pop rax
pop rdx
ret

fdc_send_byte:
    push rax
    push rdx
    call fdc_wait_write
    mov dx, FDC_FIFO
    out dx, al
    pop rdx
    pop rax
ret

fdc_recalibrate:
    push rax
    push rbx
    push rdx
    push rcx

    mov rcx, 3

.try_again:
    push rcx
    mov byte [fdc_irq_fired], 0

    mov al, 0x07
    call fdc_send_byte
    mov al, 0x00
    call fdc_send_byte

.wait_irq:
    hlt
    cmp byte [fdc_irq_fired], 1
    jne .wait_irq

    mov al, 0x08
    call fdc_send_byte
    
    mov dx, 0x3F5
    in al, dx
    mov bl, al
    in al, dx

    and bl, 0xC0
    cmp bl, 0x00
    je .success

    pop rcx
    loop .try_again

    jmp .exit

.success:
    pop rcx
.exit:
    pop rcx
    pop rdx
    pop rbx
    pop rax
ret

; CH - Cylinder
fdc_seek:
    push rcx
    push rbx
    push rax
    push rdx

    mov byte [fdc_irq_fired], 0

    mov al, 0x0F
    call fdc_send_byte
    mov al, 0x00
    call fdc_send_byte
    mov al, ch
    call fdc_send_byte

.wait_irq:
    hlt
    cmp byte [fdc_irq_fired], 1
    jne .wait_irq

    mov al, 0x08
    call fdc_send_byte
    
    mov dx, 0x3F5
    in al, dx
    mov bl, al
    in al, dx

    pop rdx
    pop rax
    pop rbx
    pop rcx
    and bl, 0xC0
    jnz .error
    ret
.error:
    call fdc_recalibrate
    pop rdx
    pop rax
    pop rbx
    pop rcx
ret

read_floppy_sector: ; RAX - number of sector, RDI - buffer to write

    mov byte [fdc_irq_fired], 0

    push rax
    
    call lba_to_chs

    push rcx
    push rdx

    mov al, 0x1C          
    mov dx, FDC_DOR
    out dx, al

    mov rcx, 50
    call sleep_ticks

    cmp byte [fdc_current_cylinder], 0xFF
    jne .skip_recal
    call fdc_recalibrate

    .skip_recal:

    call init_dma_read

    pop rbx
    pop rcx

    cmp ch, [fdc_current_cylinder]
    je .skip_seek
    
    call fdc_seek
    mov [fdc_current_cylinder], ch

    .skip_seek:

    mov al, 0x46
    call fdc_send_byte
    
    mov al, bl
    and al, 1
    
    shl al, 2
    call fdc_send_byte
    
    mov al, ch
    call fdc_send_byte
    
    mov al, bl
    call fdc_send_byte
    
    mov al, cl
    call fdc_send_byte
    
    mov al, 0x02
    call fdc_send_byte
    
    mov al, 18
    call fdc_send_byte
    
    mov al, 0x1B
    call fdc_send_byte
    
    mov al, 0xFF
    call fdc_send_byte

    .wait_irq:
        hlt
        cmp byte [fdc_irq_fired], 1
        jne .wait_irq

    mov rcx, 7
    .read_status:
        ; Здесь можно использовать вашу исправленную fdc_wait_read, 
        ; но учтите, что для ЧТЕНИЯ статуса бит DIO в MSR должен быть равен 1!
        ; Для простоты сейчас просто прочитаем их, так как они уже готовы в FIFO:
        mov dx, 0x3F5
        in al, dx
        loop .read_status
    
    mov rsi, 0x3000
    mov rcx, 64
    rep movsq

    mov al, 0x0C          
    mov dx, FDC_DOR
    out dx, al
    
    pop rax
ret

write_floppy_sector: ; RAX = LBA. 512 bytes of data at 0x3000
    mov byte [fdc_irq_fired], 0

    call lba_to_chs

    mov al, ch
    movzx r8, al
    mov al, cl
    movzx r9, al
    mov al, dh
    movzx r10, r10b       
    mov r10b, al

    push rax
    push rbx
    push rdx
    push rsi
    push rdi

    call init_dma_write 

    mov al, 0x45
    call fdc_send_byte
    
    mov al, r10b
    and al, 1
    shl al, 2             
    call fdc_send_byte
    
    mov al, r8b
    call fdc_send_byte
    
    mov al, r10b
    call fdc_send_byte
    
    mov al, r9b
    call fdc_send_byte
    
    mov al, 0x02
    call fdc_send_byte
    mov al, 18
    call fdc_send_byte
    mov al, 0x1B
    call fdc_send_byte
    mov al, 0xFF
    call fdc_send_byte

.wait_irq:
    hlt
    cmp byte [fdc_irq_fired], 1
    jne .wait_irq

    mov rbx, 7
.read_status_loop:
    mov dx, 0x3F5
    in al, dx
    dec rbx
    jnz .read_status_loop

    pop rdi
    pop rsi
    pop rdx
    pop rbx
    pop rax
ret

fdc_irq_handler:
    push rax
    
    mov byte [fdc_irq_fired], 1
    
    mov al, 0x20
    out 0x20, al

    pop rax
iretq