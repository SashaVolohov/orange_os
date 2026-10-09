use64

draw_rect: ; r13d=X, r12d=Y, r11d=Width, r10d=Height, cl=R, bh=G, bl=B, ch=A
    push rbp
    push rdi
    push rsi
    push r15
    push r14

    mov rsi, r13
    add rsi, r11
    mov rdi, r12
    add rdi, r10
    mov r14d, r12d

.loop_y:
    cmp r14d, edi
    jge .done
    mov r15d, r13d

.loop_x:
    cmp r15d, esi
    jge .next_line
    
    push rcx
    push rbx
    call draw_pixel
    pop rbx
    pop rcx

    inc r15d
    jmp .loop_x

.next_line:
    inc r14d
    jmp .loop_y

.done:

    pop r14
    pop r15
    pop rsi
    pop rdi
    pop rbp
ret

draw_close_icon: ; r13d = X, r12d = Y, cl=R, bh=G, bl=B, ch=A
    push r15
    push r14
    push rsi
    push rax

    mov r14d, r12d
    lea rsi, [.icon_data]
    mov eax, 0

.loop_y:
    cmp eax, 7
    jge .done

    mov r15d, r13d
    mov dl, [rsi + rax]
    shl dl, 1 

    push rax
    mov ecx, 0

.loop_x:
    cmp ecx, 7
    jge .next_line

    shl dl, 1
    jnc .skip_pixel

    push rdx
    push rcx
    push rbx
    
    mov cl, 0xFF
    mov bh, 0xFF
    mov bl, 0xFF
    mov ch, 0xFF
    call draw_pixel
    
    pop rbx
    pop rcx
    pop rdx

.skip_pixel:
    inc r15d
    inc ecx
    jmp .loop_x

.next_line:
    pop rax
    inc r14d
    inc eax
    jmp .loop_y

.done:
    pop rax
    pop rsi
    pop r14
    pop r15
ret

.icon_data: db 0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41

draw_window: ; r13d=X, r12d=Y, r11d=Width, r10d=Height
    push r13
    push r12
    push r11
    push r10

    mov cl, 0x40
    mov bh, 0x40
    mov bl, 0x40
    mov ch, 0xFF
    call draw_rect

    add r13d, 2
    add r12d, 2
    sub r11d, 4
    sub r10d, 4

    mov cl, 0xC0
    mov bh, 0xC0
    mov bl, 0xC0
    mov ch, 0xFF
    call draw_rect

    push r10
    
    mov r10d, 22
    mov cl, 0x00
    mov bh, 0x50
    mov bl, 0xA0
    mov ch, 0xFF
    call draw_rect

    push r13
    push r12

    mov r15d, r13d
    mov r14d, r12d
    
    add r15d, 2
    add r14d, 5

    mov rdi, test_header
    mov cl, 0FFh
    mov bh, 0FFh
    mov bl, 0FFh
    mov ch, 0FFh
    call kprint_utf8

    pop r12
    pop r13

    add r13d, r11d
    sub r13d, 20
    
    add r12d, 3

    mov r11d, 16
    mov r10d, 16

    mov cl, 0xE0
    mov bh, 0x30
    mov bl, 0x30
    mov ch, 0xFF
    call draw_rect

    add r13d, 5
    add r12d, 4
    call draw_close_icon

    pop r10

    pop r10
    pop r11
    pop r12
    pop r13
ret

test_header db 'OrangeOS Device Manager',0