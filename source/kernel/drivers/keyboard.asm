use64

keymap_us:
    db 0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', 8, 9 ; 0x00 - 0x0F
    db 'q','w','e','r','t','y','u','i','o','p','[',']', 13, 0, 'a','s' ; 0x10 - 0x1F
    db 'd','f','g','h','j','k','l',';', 39, '`', 0, '\','z','x','c','v' ; 0x20 - 0x2F
    db 'b','n','m',',','.','/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0     ; 0x30 - 0x3F
    db 0,  0,  0,  0,  0,  0,  0,  '7',0,'9','-', '4','5','6','+', '1'; 0x40 - 0x4F
    db 0,'3','0',',', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 - 0x50 - 0x5F

vector_handler_33:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r11
    push r10
    push r15

    in al, 0x60

    mov bl, al
    test bl, 0x80
    jnz .key_released

.key_pressed:
    cmp bl, 0x2A
    je .shift_on
    cmp bl, 0x36
    je .shift_on

    movzx rbx, bl
    lea r15, [keymap_us]
    mov al, [r15 + rbx]

    test al, al
    jnz .handle_normal_ascii

.handle_special_key:
    cmp bl, 0x57
    je .use_e0
    cmp bl, 0x58
    je .use_e0
    cmp bl, 0x47
    jae .use_e0

.use_00:
    mov cl, 0x00
    call kbd_buffer_push
    jmp .push_scancode

.use_e0:
    mov cl, 0xE0
    call kbd_buffer_push

.push_scancode:
    mov cl, bl
    call kbd_buffer_push
    jmp .send_eoi

.handle_normal_ascii:
    cmp [kbd_shift_flag], byte 1
    jne .push_ascii
    call apply_shift_modifier

.push_ascii:
    mov cl, al
    call kbd_buffer_push
    jmp .send_eoi

.shift_on:
    mov [kbd_shift_flag], byte 1
    jmp .send_eoi

.key_released:
    and bl, 0x7F

    cmp bl, 0x2A
    je .shift_off
    cmp bl, 0x36
    je .shift_off
    jmp .send_eoi

.shift_off:
    mov [kbd_shift_flag], byte 0
    jmp .send_eoi

.send_eoi:
    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

    pop r15
    pop r10
    pop r11
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
iretq

kbd_buffer_push:
    push rbx
    push rsi

    mov rbx, [kbd_buf_head]
    lea rsi, [kbd_buffer]
    mov [rsi + rbx], cl

    inc rbx
    and rbx, 0xFF
    mov [kbd_buf_head], rbx

    pop rsi
    pop rbx
ret

apply_shift_modifier:
    cmp al, 'a'
    jb .check_digits
    cmp al, 'z'
    ja .check_digits
    sub al, 32
    ret

.check_digits:
    push rsi
    push rbx
    mov rsi, .shift_table
.loop_table:
    mov bl, [rsi]
    test bl, bl
    jz .done
    cmp al, bl
    je .found
    add rsi, 2
    jmp .loop_table
.found:
    mov al, [rsi + 1]
.done:
    pop rbx
    pop rsi
    ret

align 8
.shift_table:
    db '1','!', '2','@', '3','#', '4','$', '5','%', '6','^', '7','&', '8','*', '9','(', '0',')'
    db '-','_', '=','+', '[','{', ']','}', ';',':', 39, '"', '`','~', '\','|', ',','<', '.','>', '/','?'
    db 0, 0

section '.data' align 16

kbd_shift_flag        db 0

kbd_buf_head          dq 0
kbd_buf_tail          dq 0
kbd_buffer:           times 256 db 0