use64

irq3_handler:

push rbp
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15

    mov dx, 0x2FA
    in al, dx
    test al, 0x01
    jnz .exit

    and al, 0x0E
    cmp al, 0x04
    jne .exit

    mov dx, 0x2F8
    in al, dx
    mov [com2_byte], al

.exit:

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
pop rbp

iretq

irq4_handler:

push rbp
push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15

    mov dx, 0x3FA
    in al, dx
    test al, 0x01
    jnz .exit

    and al, 0x0E
    cmp al, 0x04
    jne .exit

    mov dx, 0x3F8
    in al, dx
    mov [com1_byte], al

.exit:

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rdi
pop rsi
pop rdx
pop rcx
pop rbx
pop rax
pop rbp

iretq

com1_init:

mov dx, 0x3F8
add dx, 1 
mov al, 0x00
out dx, al

mov dx, 0x3F8
add dx, 3 
mov al, 0x80
out dx, al

mov dx, 0x3F8
mov al, 0x03
out dx, al

mov dx, 0x3F8
add dx, 1 
mov al, 0x00
out dx, al

mov dx, 0x3F8
add dx, 3
mov al, 0x03
out dx, al

mov dx, 0x3F8
add dx, 2 
mov al, 0xC7
out dx, al

mov dx, 0x3F8
add dx, 4
mov al, 0x0B
out dx, al

mov dx, 0x3F8
add dx, 1 
mov al, 0x01
out dx, al

ret

com2_init:

mov dx, 0x2F8
add dx, 1 
mov al, 0x00
out dx, al

mov dx, 0x2F8
add dx, 3 
mov al, 0x80
out dx, al

mov dx, 0x2F8
mov al, 0x03
out dx, al

mov dx, 0x2F8
add dx, 1 
mov al, 0x00
out dx, al

mov dx, 0x2F8
add dx, 3
mov al, 0x03
out dx, al

mov dx, 0x2F8
add dx, 2 
mov al, 0xC7
out dx, al

mov dx, 0x2F8
add dx, 4
mov al, 0x0B
out dx, al

mov dx, 0x2F8
add dx, 1 
mov al, 0x01
out dx, al

ret

com2_send: ; al - byte

push rax
push rdx

    mov ah, al

.WaitForEmpty:
    mov dx, 0x2FB
    in al, dx
    test al, 0x20
    jz .WaitForEmpty

    mov dx, 0x2F8
    mov al, ah
    out dx, al

pop rdx
pop rax

ret

kernel_serial_print:
    push rax
    push rdx
    push rsi

.next_char:
    lodsb
    test al, al
    jz .done

    mov ah, al

.wait_transmit:
    mov dx, 0x2FB
    in al, dx
    test al, 0x20
    jz .wait_transmit

    mov dx, 0x2F8
    mov al, ah
    out dx, al
    jmp .next_char

.done:
    pop rsi
    pop rdx
    pop rax
    ret

com1_byte db 0
com2_byte db 0