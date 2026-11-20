use64

init_apic:

    mov r11, [lapic_base_addr]
    mov dword [r11 + 0x80], 0

    mov dword [r11 + 0xF0], 0x1FF

    mov dword [r11 + 0x320], 0x10000
    mov dword [r11 + 0x330], 0x10000
    mov dword [r11 + 0x340], 0x10000
    mov dword [r11 + 0x350], 0x10000
    mov dword [r11 + 0x360], 0x10000
    mov dword [r11 + 0x370], 0x10000

ret

io_apic_write: ; Input - CL(number of inner register), EAX - data for write
    mov rsi, [io_apic_base_addr]
    test rsi, rsi
    jz .done
    
    movzx ecx, cl
    mov dword [rsi + 0x00], ecx
    mov dword [rsi + 0x10], eax
.done:
    ret

init_io_apic_keyboard:
    mov cl, 0x13
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x12
    mov eax, 0x00000021
    call io_apic_write

ret

init_io_apic_irq2:
    mov cl, 0x14
    mov eax, 0x00010000
    call io_apic_write
    ret

init_io_apic_irq3:
    mov cl, 0x17
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x16
    mov eax, 0x00000023
    call io_apic_write

ret

init_io_apic_irq4:
    mov cl, 0x19
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x18
    mov eax, 0x00000024
    call io_apic_write

ret

init_io_apic_irq5:
    mov cl, 0x1B
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x1A
    mov eax, 0x00000025
    call io_apic_write

ret

init_io_apic_irq6:
    mov cl, 0x1D
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x1C
    mov eax, 0x00000026
    call io_apic_write

ret

init_io_apic_irq7:
    mov cl, 0x1F
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x1E
    mov eax, 0x00000027
    call io_apic_write

ret

init_io_apic_irq8:
    mov cl, 0x21
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x20
    mov eax, 0x00000028
    call io_apic_write

ret

init_io_apic_irq9:
    mov cl, 0x23
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x22
    mov eax, 0x0000A029           
    call io_apic_write

ret

init_io_apic_irq10:
    mov cl, 0x25
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x24
    mov eax, 0x0000A02A
    call io_apic_write

ret

init_io_apic_irq11:
    mov cl, 0x27
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x26
    mov eax, 0x0000A02B           
    call io_apic_write

ret

init_io_apic_irq12:
    mov cl, 0x29
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x28
    mov eax, 0x0000002C
    call io_apic_write

ret

init_io_apic_irq13:
    mov cl, 0x2A
    mov eax, 0x00010000
    call io_apic_write
ret

init_io_apic_irq14:
    mov cl, 0x2D
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x2C
    mov eax, 0x0000002E
    call io_apic_write

ret

init_io_apic_irq15:
    mov cl, 0x2F
    mov eax, 0x00000000
    call io_apic_write

    mov cl, 0x2E
    mov eax, 0x0000002F
    call io_apic_write

ret

lapic_ticks_per_ms dd 0