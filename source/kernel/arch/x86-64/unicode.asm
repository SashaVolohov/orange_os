use64

public print_unicode

utf8_next_codepoint: ; Input - RSI - current position in the Unicode string.
                     ; Output - RAX - decoded unicode codepoint(0 if end of the string), RSI changed to the some bytes

    push rbx
    push rcx

    xor eax, eax
    mov al, [rsi]
    test al, al
    jz .done

    inc rsi

    test al, 0x80
    jz .done

    mov bl, al
    and bl, 0xE0
    cmp bl, 0xC0
    jne .check_3_bytes

    and al, 0x1F
    shl eax, 6
    mov cl, [rsi]
    and cl, 0x3F
    or al, cl
    inc rsi
    jmp .done

.check_3_bytes:
    mov bl, al
    and bl, 0xF0
    cmp bl, 0xE0
    jne .check_4_bytes

    and al, 0x0F
    shl eax, 6
    mov cl, [rsi]
    and cl, 0x3F
    or al, cl
    shl eax, 6
    mov cl, [rsi+1]
    and cl, 0x3F
    or al, cl
    add rsi, 2
    jmp .done

.check_4_bytes:
    mov bl, al
    and bl, 0xF8
    cmp bl, 0xF0
    jne .invalid

    and al, 0x07
    shl eax, 6
    mov cl, [rsi]
    and cl, 0x3F
    or al, cl
    shl eax, 6
    mov cl, [rsi+1]
    and cl, 0x3F
    or al, cl
    shl eax, 6
    mov cl, [rsi+2]
    and cl, 0x3F
    or al, cl
    add rsi, 3
    jmp .done

.invalid:
    mov eax, '?'
.done:
    pop rcx
    pop rbx
ret

draw_unicode_symbol:

    push rax
    push r12
    push r13
    push r14
    push r15
    push rbx
    push rcx
    push rbx

    mov r12d, [current_unicode_x]
    mov r13d, [current_unicode_y]
    add r13d, r11d
    xor r8, r8

.loop_y:
    xor r9, r9

.loop_x:
    mov rax, r8
    imul rax, rax, 10
    add rax, r9
    
    mov rsi, rdx

    mov al, [rsi + rax]

    cmp al, 0
    je .next_pixel

    mov r15d, r12d
    add r15d, r9d
    
    mov r14d, r13d
    add r14d, r8d

    mov rcx, [rsp+8]
    mov rbx, [rsp]

    call draw_pixel

.next_pixel:

    inc r9
    cmp r9, 10
    jl .loop_x

    inc r8
    cmp r8, 10
    jl .loop_y

    add rsp, 24
    pop r15
    pop r14
    pop r13
    pop r12
    pop rax

ret

draw_unicode_char: ; rdi - unicode codepoint, cl - R, bh - G, bl - B, ch - alpha
push rdx
push r15
push r14
push rcx
push rbx
push r8
push rax
push r11
push rsi
push rdi

cmp rdi, 255
jle .draw_ascii_char

mov rax, main_unicode_system_font

.find_unicode_symbol:

mov edx, [rax]
cmp edx, edi
je .char_found

add rax, 16
mov rdx, main_unicode_system_font_end
cmp rax, rdx
jae .undefined_unicode_char

jmp .find_unicode_symbol

.undefined_unicode_char:

mov rdi, '?'
jmp .draw_ascii_char

.char_found:
mov rdx, [rax+4]
mov r11w, [rax+14]

call draw_unicode_symbol

mov r11w, [rax+12]
add [current_unicode_x], r11d

jmp .exit

.draw_ascii_char:

mov [ascii_symbol], dil

mov r8, y_distance
add r8, rdi

mov eax, 0
mov al, [r8]

mov rdx, ascii_symbol
mov r15d, [current_unicode_x]
mov r14d, [current_unicode_y]
add r14d, eax
call draw_symbol

mov r8, x_distance
add r8, rdi

mov eax, 0
mov al, [r8]

add [current_unicode_x], eax

.exit:

pop rdi
pop rsi
pop r11
pop rax
pop r8
pop rbx
pop rcx
pop r14
pop r15
pop rdx

ret

kprint_utf8: ; Input - RDI, unicode string with \0, r15d - x, r14d - y, cl - R, bh - G, bl - B, ch - alpha
    push rsi
    push rbx

    mov [current_unicode_x], r15d
    mov [current_unicode_y], r14d

    mov [begin_unicode_x], r15d
    
    mov rsi, rdi

.loop:
    call utf8_next_codepoint
    test eax, eax
    jz .exit

    cmp eax, 0x0A
    je .handle_newline

    mov rdi, rax
    call draw_unicode_char
    
    jmp .loop

.handle_newline:
    mov eax, [begin_unicode_x]
    mov [current_unicode_x], eax
    add [current_unicode_y], 15
    jmp .loop

.exit:

    pop rbx
    pop rsi
ret

print_unicode:

    push r14
    push r15
    push rbx
    mov r15d, esi
    mov r14d, edx

    movzx eax, r8b
    movzx edx, r9b
    shl eax, 8
    or  eax, edx
    mov bx, ax

    movzx r10d, cl
    movzx eax, byte [rsp + 32]
    shl eax, 8 
    or  eax, r10d
    mov cx, ax


    call kprint_utf8
    pop rbx
    pop r15
    pop r14
ret

begin_unicode_x dd 0
current_unicode_x dd 0
current_unicode_y dd 0
ascii_symbol db 0