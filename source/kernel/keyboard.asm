use64

keymap_us:
    db 0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', 8, 9 ; 0x00 - 0x0F
    db 'q','w','e','r','t','y','u','i','o','p','[',']', 13, 0, 'a','s' ; 0x10 - 0x1F
    db 'd','f','g','h','j','k','l',';', 39, '`', 0, '\','z','x','c','v' ; 0x20 - 0x2F
    db 'b','n','m',',','.','/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0     ; 0x30 - 0x3F

irq1_keyboard_handler:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r11
    push r10

    in al, 0x60

    test al, 0x80
    jnz .key_released

.key_pressed:

    mov r15, keymap_us
    add r15b, al

    mov r10, [r15]

    test r10, r10
    jz .send_eoi

    mov rdx, r15
    mov r15d, 0
    mov r14d, 340
    mov cl, 0FFh
    mov bh, 0FFh
    mov bl, 0FFh
    mov ch, 0FFh
    call draw_symbol

    jmp .send_eoi

.key_released:
    jmp .send_eoi

.send_eoi:
    mov al, 0x20
    out 0x20, al

    pop r10
    pop r11
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
iretq