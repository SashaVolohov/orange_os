use64

extrn KiSetup
extrn ShowExit

public t_print
public clear_setup_screen

setup_kernel:

mov rsi, welcome
mov dh, 4
mov dl, 3
mov ah, 1Fh
call print_string_64

mov rsi, first_stage
call draw_status_bar_long_mode

mov rsi, first_stage_description
mov dh, 6
mov dl, 3
mov ah, 1Fh
call print_string_64

mov rsi, enter_choice
mov dh, 8
mov dl, 3
mov ah, 1Fh
call print_string_64

mov rsi, r_choice
mov dh, 9
mov dl, 3
mov ah, 1Fh
call print_string_64

mov rsi, f3_choice
mov dh, 10
mov dl, 3
mov ah, 1Fh
call print_string_64

.wait_key:

call getch

cmp rax, 13
je KiSetup

cmp rax, 27
je ShowExit

jmp .wait_key

print_string_64: ; RSI - string
                 ;   DH  - Y
                 ;   DL  - X
                 ;   AH  - color
    push rsi
    push rdi
    push rbx
    push rax

    xor rbx, rbx
    mov bl, dh
    imul rbx, 80
    
    xor rdi, rdi
    mov dil, dl
    add rbx, rdi
    shl rbx, 1

    mov rdi, 0xB8000
    add rdi, rbx

    mov bh, ah

.loop:
    mov al, [rsi]
    inc rsi
    cmp al, 0
    je .done

    mov ah, bh
    mov [rdi], ax
    add rdi, 2
    jmp .loop

.done:
    pop rax
    pop rbx
    pop rdi
    pop rsi
ret

t_print:
    mov r8, rdi
    
    mov dh, dl
    mov dl, sil
    mov ah, cl
    mov rsi, r8

    call print_string_64
ret

clear_setup_screen:
    push rdi
    push rcx
    push rax

    mov rdi, 0xB8000    

    mov rax, 0x1F201F201F201F20

    mov rcx, 500

.loop:
    mov [rdi], rax
    add rdi, 8
    dec rcx
    jnz .loop

    mov rsi, orange_os_setup
    mov dh, 1
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, orange_os_setup_down
    mov dh, 2
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, null_stage
    call draw_status_bar_long_mode

    pop rax
    pop rcx
    pop rdi
ret

welcome db "Welcome to Setup.", 0
first_stage db "ENTER=Continue R=Repair F3=Exit",0
null_stage db ' ',0
first_stage_description db "This portion of the Setup prepares OrangeOS to run on your computer.",0
enter_choice db "To set up OrangeOS now, press ENTER.",0
r_choice db "To repair a OrangeOS installation using Recovery Console, press R.",0
f3_choice db "To quit Setup without installing OrangeOS, press F3.",0

orange_os_setup db "OrangeOS Setup",0
orange_os_setup_down db "====================",0