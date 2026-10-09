use64

vector_handler_40:

push rax
push rsi

    mov al, 0x0C
    out 0x70, al

    in al, 0x71

    test al, 0x40
    jnz .exit

    add [cmos_ticks], 1

.exit:

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

pop rsi
pop rax

iretq

init_rtc:
    push rax

    mov al, 0x0B
    out 0x70, al
    in al, 0x71
    or al, 0x40
    mov ah, al
    mov al, 0x0B
    out 0x70, al
    mov al, ah
    out 0x71, al 

    mov al, 0x0A
    out 0x70, al
    in al, 0x71
    and al, 0xF0
    or al, 0x06
    mov ah, al
    mov al, 0x0A
    out 0x70, al
    mov al, ah
    out 0x71, al

    mov al, 0x0C
    out 0x70, al
    in al, 0x71

    pop rax
ret

update_rtc_time:
    push rax
    push rbx
    push rcx
    push rdx

.wait_update:
    mov al, 0x0A
    out 0x70, al
    in al, 0x71
    test al, 0x80
    jnz .wait_update

    mov al, 0x00
    out 0x70, al
    in al, 0x71
    mov [rtc_sec], al

    mov al, 0x02
    out 0x70, al
    in al, 0x71
    mov [rtc_min], al

    mov al, 0x04
    out 0x70, al
    in al, 0x71
    mov [rtc_hour], al

    mov al, 0x07
    out 0x70, al
    in al, 0x71
    mov [rtc_day], al

    mov al, 0x08
    out 0x70, al
    in al, 0x71
    mov [rtc_month], al

    mov al, 0x09
    out 0x70, al
    in al, 0x71
    mov byte [rtc_year], al

    mov al, 0x0B
    out 0x70, al
    in al, 0x71
    mov bl, al

    test bl, 0x04
    jnz .convert_hours

    movzx ax, [rtc_sec]
    call bcd_to_bin
    mov [rtc_sec], al

    movzx ax, [rtc_min]
    call bcd_to_bin
    mov [rtc_min], al

    movzx ax, [rtc_hour]
    call bcd_to_bin
    mov [rtc_hour], al

    movzx ax, [rtc_day]
    call bcd_to_bin
    mov [rtc_day], al

    movzx ax, [rtc_month]
    call bcd_to_bin
    mov [rtc_month], al

    movzx ax, byte [rtc_year]
    call bcd_to_bin
    movzx ax, al
    add ax, 2000
    mov [rtc_year], ax

.convert_hours:
    test bl, 0x02
    jnz .done

    mov al, [rtc_hour]
    test al, 0x80
    jz .done
    and al, 0x7F
    add al, 12
    cmp al, 24
    jne .store_hour
    mov al, 12
.store_hour:
    mov [rtc_hour], al

.done:
    pop rdx
    pop rcx
    pop rbx
    pop rax
ret

bcd_to_bin:
    mov ah, al
    shr ah, 4
    and al, 0x0F
    imul ax, ax, 10
    push rbx
    push rax
    mov al, ah
    movzx rbx, al
    pop rax
    imul rbx, rbx, 10
    add al, bl
    pop rbx
ret

cmos_ticks dq 0

rtc_sec   db 0
rtc_min   db 0
rtc_hour  db 0
rtc_day   db 0
rtc_month db 0
rtc_year  dw 0