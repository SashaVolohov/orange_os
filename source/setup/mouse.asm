use64

mouse_logo:
db 2,2,0,0,0,0,0,0,0,0,0,0
db 2,1,2,0,0,0,0,0,0,0,0,0
db 2,1,1,2,0,0,0,0,0,0,0,0
db 2,1,1,1,2,0,0,0,0,0,0,0
db 2,1,1,1,1,2,0,0,0,0,0,0
db 2,1,1,1,1,1,2,0,0,0,0,0
db 2,1,1,1,1,1,1,2,0,0,0,0
db 2,1,1,1,1,1,1,1,2,0,0,0
db 2,1,1,1,1,1,1,1,1,2,0,0
db 2,1,1,1,1,1,1,1,1,1,2,0
db 2,1,1,1,1,1,2,2,2,2,2,2
db 2,1,1,2,1,1,2,0,0,0,0,0
db 2,1,2,0,2,1,1,2,0,0,0,0
db 2,2,0,0,2,1,1,2,0,0,0,0
db 0,0,0,0,0,2,1,1,2,0,0,0
db 0,0,0,0,0,2,1,1,2,0,0,0
db 0,0,0,0,0,0,2,1,1,2,0,0
db 0,0,0,0,0,0,2,1,1,2,0,0
db 0,0,0,0,0,0,0,2,2,0,0,0

mouse_background:
times 864 db 0

ps2_wait_read:

push rax

.wait:

in al, 0x64
test al, 0x01

jz .wait

pop rax

ret

ps2_wait_write:

push rax

.wait:

in al, 0x64
test al, 0x02

jnz .wait

pop rax

ret

ps2_write_command: ; al - command

call ps2_wait_write
out 0x64, al

ret

mouse_write: ; bl - command

push rax

mov al, 0xD4
call ps2_write_command

call ps2_wait_write

mov al, bl
out 0x60, al

pop rax

ret

mouse_read: ; Output - AL 

call ps2_wait_read

in al, 0x60

ret

mouse_init:

mov al, 0xA8
call ps2_write_command

mov al, 0x20
call ps2_write_command

call ps2_wait_read

in al, 0x60
mov bl, al

or bl, 2
and bl, 0xDF

mov al, 0x60
call ps2_write_command

call ps2_wait_write

mov al, bl
out 0x60, al

mov bl, 0xF6
call mouse_write

call mouse_read

mov bl, 0xF4
call mouse_write

call mouse_read

ret

mouse_handler:
    push rax
    push rbx
    push rcx
    push rdx 

    in al, 0x64
    test al, 0x20
    jz .exit

    in al, 0x60

    mov rbx, mouse_packet
    movzx rcx, byte [mouse_cycle] 
    mov [rbx + rcx], al

    test rcx, rcx
    jnz .inc_cycle
    test al, 0x08
    jz .exit          

.inc_cycle:
    inc byte [mouse_cycle]
    cmp byte [mouse_cycle], 3
    jne .exit
    mov byte [mouse_cycle], 0
    
    mov al, [mouse_packet]
    mov bl, al
    and al, 1
    mov [mouse_buttons], al     
    mov al, bl
    shr al, 1
    and al, 1
    mov [mouse_buttons+1], al   
    mov al, bl
    shr al, 2
    and al, 1
    mov [mouse_buttons+2], al   

    movsx eax, byte [mouse_packet+1] 
    movsx ebx, byte [mouse_packet+2] 
    neg ebx                          

    cmp dword [old_mouse_x], 1000
    je .skip_restore

.render_screen:
    push r15
    push r14
    push r12
    push r13
    push rdx
    push rcx
    push rbx

    mov r12d, [old_mouse_x]
    mov r13d, [old_mouse_y]
    mov rdx, mouse_background
    
    xor r9, r9
.restore_paint_y:
    xor r8, r8
.restore_paint_x:
    lea r15, [r12 + r8] 
    lea r14, [r13 + r9] 

    mov ecx, dword [rdx]
    mov bl, cl
    mov bh, ch
    shr ecx, 16
    
    push r8
    push r9
    push rdx
    call draw_pixel
    pop rdx
    pop r9
    pop r8

    add rdx, 4          
    inc r8              
    cmp r8, 12          
    jne .restore_paint_x

    xor r8, r8          
    inc r9              
    cmp r9, 18          
    jne .restore_paint_y

    pop rbx
    pop rcx
    pop rdx
    pop r13
    pop r12
    pop r14
    pop r15

.skip_restore:

    add [global_mouse_x], eax
    add [global_mouse_y], ebx

    cmp dword [global_mouse_x], 0
    jge .check_max_x
    mov dword [global_mouse_x], 0
    jmp .handle_y
.check_max_x:
    cmp dword [global_mouse_x], 799
    jle .handle_y
    mov dword [global_mouse_x], 799

.handle_y:
    cmp dword [global_mouse_y], 0
    jge .check_max_y
    mov dword [global_mouse_y], 0
    jmp .save_new_background
.check_max_y:
    cmp dword [global_mouse_y], 581
    jle .save_new_background
    mov dword [global_mouse_y], 581

.save_new_background:
    push r15
    push r14
    push r12
    push r13
    push rdx
    push rcx
    push rbx
    mov r12d, [global_mouse_x]
    mov r13d, [global_mouse_y]
    mov rsi, mouse_background
    xor r9, r9
.save_paint_y:
    xor r8, r8
.save_paint_x:
    lea r15, [r12 + r8]
    lea r14, [r13 + r9]

    push r8
    push r9
    push rsi
    call read_pixel     
    pop rsi
    pop r9
    pop r8
    
    mov [rsi], bl
    mov [rsi+1], bh
    mov [rsi+2], cl
    mov [rsi+3], ch
    add rsi, 4

    inc r8
    cmp r8, 12
    jne .save_paint_x
    inc r9
    cmp r9, 18
    jne .save_paint_y
    pop rbx
    pop rcx
    pop rdx
    pop r13
    pop r12
    pop r14
    pop r15
.begin_draw:
    call draw_mouse

    mov eax, [global_mouse_x]
    mov [old_mouse_x], eax
    mov eax, [global_mouse_y]
    mov [old_mouse_y], eax

.exit:
    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq

draw_mouse:
    push r15
    push r14
    push r12
    push r13
    push rdx
    push rcx
    push rbx

    mov r12d, [global_mouse_x]
    mov r13d, [global_mouse_y]

    mov rdx, mouse_logo
    xor r9, r9          ; Y
.mouse_paint_y:
    xor r8, r8          ; X
.mouse_paint_x:
    movzx eax, byte [rdx] 
    test al, al           
    jz .skip_pixel

    lea r15, [r12 + r8]   
    lea r14, [r13 + r9]   

    cmp al, 2
    je .set_black

    ; Белый цвет
    mov ecx, 0FFFFFFFFh
    mov ebx, 0FFFFFFFFh
    jmp .draw

.set_black:
    xor ecx, ecx
    xor ebx, ebx

.draw:
    push rdx
    push r8
    push r9
    call draw_pixel
    pop r9
    pop r8
    pop rdx

.skip_pixel:
    inc rdx
    inc r8
    cmp r8, 12
    jne .mouse_paint_x

    xor r8, r8          
    inc r9              
    cmp r9, 18
    jne .mouse_paint_y

.exit:
    pop rbx
    pop rcx
    pop rdx
    pop r13
    pop r12
    pop r14
    pop r15
    ret

mouse_packet db 0,0,0
mouse_cycle db 0
mouse_buttons db 0,0,0
mouse_x db 0
mouse_y db 0
global_mouse_x dd 400
global_mouse_y dd 300
old_mouse_x dd 1000
old_mouse_y dd 1000