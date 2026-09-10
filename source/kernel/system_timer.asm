PIT_COMMAND equ 0x43
PIT_CHANNEL0 equ 0x40

timer_ticks dq 0

use64

init_pit:
    mov al, 0x36
    out PIT_COMMAND, al

    mov al, 0x9B
    out PIT_CHANNEL0, al
    mov al, 0x2E
    out PIT_CHANNEL0, al
ret

irq0_timer_handler:
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

    inc qword [timer_ticks]

    ; print on screen, etc...

    mov al, 0x20
    out 0x20, al

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

iretq