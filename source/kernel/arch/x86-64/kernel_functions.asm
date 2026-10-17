use64

public outb
public outw
public outl

public inb
public inw
public inl

public memcpy
public memset
public memcmp
public strlen

public strcmp
public strncmp
public strcpy
public strcat

public strncpy
public strncat

public print_text
public cpu_get_cpuid

public update_status_bar
public getch

public sleep_ms
public flip_buffers

public spin_lock
public spin_unlock

MAX_PROCESSES equ 32

TASK_STATE_EMPTY   equ 0
TASK_STATE_READY   equ 1
TASK_STATE_RUNNING equ 2
TASK_STATE_SLEEP   equ 3
TASK_STATE_ZOMBIE  equ 4

PCB_rax    equ 0
PCB_rbx    equ 8
PCB_rcx    equ 16
PCB_rdx    equ 24
PCB_rsi    equ 32
PCB_rdi    equ 40
PCB_rbp    equ 48
PCB_rsp    equ 56
PCB_rip    equ 64
PCB_rflags equ 72
PCB_cr3    equ 80
PCB_pid    equ 88
PCB_state  equ 96
PCB_ticks  equ 104

struct_pcb_size = 112

MSR_STAR    equ 0xC0000081
MSR_LSTAR   equ 0xC0000082
MSR_SFMASK  equ 0xC0000084


draw_pixel: ; r15d - x, r14d - y, cl - R, bh - G, bl - B, ch - alpha

push rsi
push rax
push r8
push rax
push rdx

cmp r15d, 800
jae .exit

cmp r14d, 600
jae .exit

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov rax, 0
mov eax, r14d
mov r8, 0
mov r8w, [vbe_hor_size]
mul r8d

mov r8d, 4
mul r8d

add esi, eax

mov rax, 0
mov eax, r15d
mov r8d, 4
mul r8d

add esi, eax

mov [esi], bl
mov [esi+1], bh
mov [esi+2], cl
mov [esi+3], ch

.exit:

pop rdx
pop rax
pop r8
pop rax
pop rsi

ret

draw_mouse_pixel: ; r15d - x, r14d - y, cl - R, bh - G, bl - B, ch - alpha

push rsi
push rax
push r8
push rax
push rdx

cmp r15d, 800
jae .exit

cmp r14d, 600
jae .exit

mov rsi, 0
mov esi, [lfb_mouse_addr]

mov rax, 0
mov eax, r14d
mov r8, 0
mov r8w, [vbe_hor_size]
mul r8d

mov r8d, 4
mul r8d

add esi, eax

mov rax, 0
mov eax, r15d
mov r8d, 4
mul r8d

add esi, eax

mov [esi], bl
mov [esi+1], bh
mov [esi+2], cl
mov [esi+3], ch

.exit:

pop rdx
pop rax
pop r8
pop rax
pop rsi

ret

read_pixel: ; r15d - x, r14d - y. Output: cl - R, bh - G, bl - B, ch - alpha

push rsi
push rax
push r8

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov rax, 0
mov eax, r14d
mov r8, 0
mov r8w, [vbe_hor_size]
mul r8d

mov r8d, 4
mul r8d

add esi, eax

mov rax, 0
mov eax, r15d
mov r8d, 4
mul r8d

add esi, eax

mov bl, [esi]
mov bh, [esi+1]
mov cl, [esi+2]
mov ch, [esi+3]

pop r8
pop rax
pop rsi

ret

draw_symbol: ; rdx - pointer to symbol, r15d - x, r14d - y, cl - R, bh - G, bl - B, ch - alpha

push rax
push rdx
push r15
push r14
push r13
push r12
push r11
push r10
push r9

mov r9, 0
mov r10d, r15d

mov rax, 0
mov al, [rdx]

mov r12, main_system_font
mov r11, 100
mul r11
add r12, rax

main_loop:

cmp byte [r12], 1
je draw_a

main_loop_font_continue:

dec r11
add r12, 1

add r15d, 1
add r9, 1


cmp r11, 0
je draw_symbol_exit

cmp r9, 10
je new_x_font

jmp main_loop

new_x_font:

mov r15d, r10d
add r14d, 1
mov r9, 0

jmp main_loop

draw_a:

call draw_pixel

jmp main_loop_font_continue

draw_symbol_exit:

pop r9
pop r10
pop r11
pop r12
pop r13
pop r14
pop r15
pop rdx
pop rax

ret

draw_text: ; r13 - pointer to string, r15d - x, r14d - y, cl - R, bh - G, bl - B, ch - alpha

push rdx
push r11
push r12
push r13
push r15

draw_text_loop:

mov r12, y_distance
mov r11, 0
mov r11b, [r13]
add r12, r11

mov r11, 0
mov r11b, [r12]

push r14w

add r14w, r11w

mov rdx, r13
call draw_symbol

pop r14w

mov r12, x_distance
mov r11, 0
mov r11b, [r13]
add r12, r11

mov r11, 0
mov r11b, [r12]

add r15w, r11w

add r13, 1

cmp byte [r13], 0
je exit_draw_text

jmp draw_text_loop

exit_draw_text:

pop r15
pop r13
pop r12
pop r11
pop rdx

ret

get_string_from_int_32: ; Input: ebx - 32-bit int
                        ; Output: eax - pointer to formatted string
push rbx
push r12
push rcx
push rdx
push r15
push r14
push r13
push r10
push r9

mov r15, get_string_from_int_32_string
add r15, 2

mov r12, get_string_from_int_32_string
add r12, 10

mov cl, 28

continue_string_from_int_32:

mov r11,0
mov r11d, ebx
shr r11d, cl
and r11d, 0Fh

mov rdx, codes_16_sc
add rdx, r11

mov r14b, [rdx]
mov [r15], r14b

add r15, 1
sub cl, 4

cmp r15, r12
je end_string_from_int_32
jmp continue_string_from_int_32

end_string_from_int_32:

mov rax, get_string_from_int_32_string

pop r9
pop r10
pop r13
pop r14
pop r15
pop rdx
pop rcx
pop r12
pop rbx

ret

get_string_from_int_64: ; Input: rbx - 64-bit int
                        ; Output: rax - pointer to formatted string
push rbx
push r12
push rcx
push rdx
push r15
push r14
push r13
push r10
push r9
push r11

mov r15, get_string_from_int_64_string
add r15, 2

mov r12, get_string_from_int_64_string
add r12, 18

mov cl, 60

continue_string_from_int_64:

mov r11, rbx
shr r11, cl
and r11, 0Fh

mov rdx, codes_16_sc
add rdx, r11

mov r14b, [rdx]
mov [r15], r14b

add r15, 1
sub cl, 4

cmp r15, r12
je end_string_from_int_64
jmp continue_string_from_int_64

end_string_from_int_64:

mov rax, get_string_from_int_64_string

pop r11
pop r9
pop r10
pop r13
pop r14
pop r15
pop rdx
pop rcx
pop r12
pop rbx

ret

get_10_string_from_int_64: ; Input: rbx - 64-bit int
                        ; Output: rax - pointer to formatted string

push rbx
push r12
push rcx
push rdx
push r15
push r14
push r13
push r10
push r9
push r11


mov rdx, 0
mov rax, rbx
mov cx, 0

continue_10_string:

mov r10, 10
div r10

push rdx
mov rdx, 0
add cx, 1

cmp rax, 0
jne continue_10_string

mov rax, string_10

final_10_string:

pop rdx
add rdx, 48
mov [rax], rdx

add rax, 1

sub cx, 1

cmp cx, 0
jne final_10_string

mov [rax], byte 0

mov rax, string_10

pop r11
pop r9
pop r10
pop r13
pop r14
pop r15
pop rdx
pop rcx
pop r12
pop rbx

ret

string_10 db "00000000000000000000",0

format_string_with_32_int: ; r15d - int32 to paste, r14 - string to format, r13 - where start format

push rbx
push rdx
push rcx
push rax
push r15
push r14
push r13

mov ebx, r15d
call get_string_from_int_32

mov rbx, r14
add rbx, r13
mov edx, 0

copy_efer:
mov cl, [rax]
mov [rbx], cl
add rbx, 1
add rax, 1
add edx, 1

cmp edx, 10
jne copy_efer

pop r13
pop r14
pop r15
pop rax
pop rcx
pop rdx
pop rbx

ret

format_string_with_64_int: ; r15 - int32 to paste, r14 - string to format, r13 - where start format

push rbx
push rdx
push rcx
push rax
push r15
push r14
push r13

mov rbx, r15
call get_string_from_int_64

mov rbx, r14
add rbx, r13
mov rdx, 0

copy_efer_64:
mov cl, [rax]
mov [rbx], cl
add rbx, 1
add rax, 1
add rdx, 1

cmp rdx, 18
jne copy_efer_64

pop r13
pop r14
pop r15
pop rax
pop rcx
pop rdx
pop rbx

ret

format_10_string_with_64_int: ; r15 - int32 to paste, r14 - string to format, r13 - where start format

push rbx
push rdx
push rcx
push rax
push r15
push r14
push r13
push r9
push r10

mov rbx, r15
call get_10_string_from_int_64

mov rbx, r14
add rbx, r13
mov rdx, 0

copy_10_efer_64:
mov cl, [rax]

cmp cl, 0
je format_10_finish

mov [rbx], cl
add rbx, 1
add rax, 1
add rdx, 1

; rdx счётчик
jmp copy_10_efer_64

format_10_finish:

mov r10, r14
add r10, r13
add r10, 20

format_10_finish_cycle:

mov cl, [r10]
mov [rbx], cl
add rbx, 1
add rdx, 1

add r10, 1

cmp cl, 0
je format_10_final
jmp format_10_finish_cycle

format_10_final:

pop r10
pop r9
pop r13
pop r14
pop r15
pop rax
pop rcx
pop rdx
pop rbx

ret

print_format_string_with_32_int: ; r15d - int32 to paste, r14 - string to format, r13 - where start format, r12d - x, r11d - y, cl - R, bh - G, bl - B, ch - alpha

push r15
push r14
push r13

call format_string_with_32_int

mov r15d, r12d
mov r13, r14
mov r14d, r11d
call draw_text

pop r13
pop r14
pop r15

ret

print_format_string_with_64_int: ; r15 - int32 to paste, r14 - string to format, r13 - where start format, r12d - x, r11d - y, cl - R, bh - G, bl - B, ch - alpha

push r15
push r14
push r13

call format_string_with_64_int

mov r15d, r12d
mov r13, r14
mov r14d, r11d
call draw_text

pop r13
pop r14
pop r15

ret

sleep_ticks: ; rcx - tick count
    push rax
    push rbx
    
    mov rbx, [timer_ticks]
    add rbx, rcx

.wait_loop:
    hlt
    mov rax, [timer_ticks]
    cmp rax, rbx
    jb .wait_loop

    pop rbx
    pop rax
ret

sleep_ms: ; RDI - time in ms
    push rax
    push rbx
    push rcx
    push rdx

    test rdi, rdi
    jz .done

    mov rax, rdi
    xor rdx, rdx
    mov rcx, 5
    div rcx
    test rax, rax
    jnz .set_sleep
    mov rax, 1

.set_sleep:
    mov rcx, rax

    mov eax, [current_task_index]
    imul rax, struct_pcb_size
    mov rdx, [process_table_ptr]
    add rdx, rax

    mov rax, [timer_ticks]
    add rax, rcx
    mov [rdx + PCB_ticks], rax

    mov dword [rdx + PCB_state], TASK_STATE_SLEEP            

    .wait_for_hardware_tick:
    
    cmp dword [rdx + PCB_state], TASK_STATE_SLEEP
    je .wait_for_hardware_tick 

.done:
    pop rdx
    pop rcx
    pop rbx
    pop rax
ret

outb: ; RDI - port, rsi - value
    push rdx
    mov dx, di
    mov al, sil
    out dx, al
    pop rdx
ret

outw: ; RDI - port, rsi - value
    push rdx
    mov dx, di
    mov ax, si
    out dx, ax
    pop rdx
ret

outl: ; RDI - port, rsi - value
    push rdx
    mov dx, di
    mov eax, esi
    out dx, eax
    pop rdx
ret

inb: ; RDI - port
    push rdx
    mov dx, di
    in al, dx
    movzx rax, al
    pop rdx
ret

inw: ; RDI - port
    push rdx
    mov dx, di
    in ax, dx
    movzx rax, ax
    pop rdx
ret

inl: ; RDI - port
    push rdx
    mov dx, di
    in eax, dx
    mov eax, eax
    pop rdx
ret

memcpy:
    mov rax, rdi
    mov rcx, rdx

    cld
    rep movsb
ret

memset:
    mov r8, rdi
    mov rcx, rdx
    mov al, sil
    cld
    rep stosb
    mov rax, r8
ret

memcmp:
    test rdx, rdx
    jz .equal
    mov rcx, rdx
    cld
.loop_memcmp:
    mov al, [rdi]
    mov bl, [rsi]
    cmp al, bl
    jne .diff
    inc rdi
    inc rsi
    loop .loop_memcmp
.equal:
    xor rax, rax
    ret
.diff:
    movzx rax, al
    movzx rcx, bl
    sub rax, rcx
    ret

strlen:
    xor rax, rax
.loop_strlen:
    cmp byte [rdi + rax], 0
    je .done_strlen
    inc rax
    jmp .loop_strlen
.done_strlen:
    ret

strcmp:
    cld
.loop_strcmp:
    movzx rax, byte [rdi]
    movzx rcx, byte [rsi]
    cmp al, cl
    jne .diff_strcmp
    test al, al
    jz .equal_strcmp
    inc rdi
    inc rsi
    jmp .loop_strcmp
.diff_strcmp:
    sub rax, rcx
    ret
.equal_strcmp:
    xor rax, rax
    ret

strncmp:
    cld
    test rdx, rdx
    jz .equal_strncmp
.loop_strncmp:
    movzx rax, byte [rdi]
    movzx rcx, byte [rsi]
    cmp al, cl
    jne .diff_strncmp
    test al, al
    jz .equal_strncmp
    inc rdi
    inc rsi
    dec rdx
    jz .equal_strncmp
    jmp .loop_strncmp
.diff_strncmp:
    sub rax, rcx
    ret
.equal_strncmp:
    xor rax, rax
    ret

strcpy:
    mov rax, rdi
.loop_strcpy:
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .done_strcpy
    inc rdi
    inc rsi
    jmp .loop_strcpy
.done_strcpy:
    ret

strcat:
    mov rax, rdi
.find_end_strcat:
    cmp byte [rdi], 0
    jz .copy_strcat
    inc rdi
    jmp .find_end_strcat
.copy_strcat:
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .done_strcat
    inc rdi
    inc rsi
    jmp .copy_strcat
.done_strcat:
    ret

strncpy:
    mov rax, rdi
.loop_strncpy:
    test rdx, rdx
    jz .done_strncpy
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .fill_zeros
    inc rdi
    inc rsi
    dec rdx
    jmp .loop_strncpy
.fill_zeros:
    test rdx, rdx
    jz .done_strncpy
    mov byte [rdi], 0
    inc rdi
    dec rdx
    jmp .fill_zeros
.done_strncpy:
    ret

strncat:
    mov rax, rdi
.find_end_strncat:
    cmp byte [rdi], 0
    jz .copy_strncat
    inc rdi
    jmp .find_end_strncat
.copy_strncat:
    test rdx, rdx
    jz .done_strncat
    mov cl, [rsi]
    mov [rdi], cl
    test cl, cl
    jz .done_strncat_null
    inc rdi
    inc rsi
    dec rdx
    jmp .copy_strncat
.done_strncat_null:
    ret
.done_strncat:
    mov byte [rdi], 0
    ret

print_text:
    ; rdi = str, rsi = x, rdx = y, rcx = r, r8 = g, r9 = b, [rsp + 16] = alpha
    push rbx

    mov r13, rdi         ; r13 - pointer to string
    mov r15d, esi       ; r15d - x
    mov r14d, edx        ; r14d - y
    
    movzx eax, r8b
    mov bh, al           ; bh - G

    movzx eax, r9b
    mov bl, al           ; bl - B
    
    mov ch, [rsp + 16]   ; ch - alpha

    call draw_text

    pop rbx
    ret

cpu_get_cpuid: ; RDI - CPUID function, RSI - 32-bit array(4 elements)

    push rbx
    push rbp
    mov rbp, rsi

    mov eax, edi
    xor ecx, ecx
    cpuid

    mov [rbp + 0], eax
    mov [rbp + 4], ebx
    mov [rbp + 8], ecx
    mov [rbp + 12], edx

    pop rbp
    pop rbx
ret

draw_status_bar_long_mode: ; RSI - string
    push rsi
    push rdi
    push rcx
    push rax

    mov rdi, 0xB8F00    
    
    mov rcx, 80
    mov ah, 70h

.text_loop:
    mov al, [rsi]
    inc rsi
    cmp al, 0
    je .fill_loop

    mov [rdi], ax
    add rdi, 2
    dec rcx
    jz .done
    jmp .text_loop

.fill_loop:
    mov al, ' '
.fill_loop_run:
    cmp rcx, 0
    je .done
    mov [rdi], ax
    add rdi, 2
    dec rcx
    jmp .fill_loop_run

.done:
    pop rax
    pop rcx
    pop rdi
    pop rsi
ret

update_status_bar:
    mov rsi, rdi
    call draw_status_bar_long_mode
ret

getch: ; RAX - key
    push rbx
    push rsi

.wait_data:
    mov rax, [kbd_buf_head]
    mov rbx, [kbd_buf_tail]

    cmp rax, rbx
    jne .read_byte

    hlt
    jmp .wait_data

.read_byte:
    lea rsi, [kbd_buffer]
    movzx rax, byte [rsi + rbx]

    inc rbx
    and rbx, 0xFF
    mov [kbd_buf_tail], rbx

    pop rsi
    pop rbx
ret

init_syscall_interface:
    push rax
    push rdx
    push rcx

    mov ecx, MSR_STAR
    rdmsr
    mov edx, 0x001B0008
    wrmsr

    mov ecx, MSR_LSTAR
    lea rax, [syscall_dispatcher]
    mov rdx, rax
    shr rdx, 32
    wrmsr

    mov ecx, MSR_SFMASK
    rdmsr
    mov eax, 0x200
    wrmsr

    pop rcx
    pop rdx
    pop rax
ret

align 8
syscall_dispatcher:
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

    cmp rax, MAX_SYSCALLS
    jae .invalid_syscall

    lea rbx, [syscall_table]
    call qword [rbx + rax * 8]

.exit_syscall:
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

sysretq

.invalid_syscall:
    mov rax, -1
jmp .exit_syscall

; rdi = dest_front
; rsi = src_back
; rdx = dwords_count

flip_buffers:
    cld
    mov rcx, rdx
    rep movsd
ret

; RDI = lock addr
spin_lock:

push rax

    mov eax, 1

.loop:
    xchg [rdi], eax
    test eax, eax
    jz .done

    pause
    jmp .loop

.done:

pop rax

    ret

; RDI = lock addr
spin_unlock:
    mov dword [rdi], 0
ret

get_string_from_int_32_string db "0x00000000",0
get_string_from_int_64_string db "0x0000000000000000",0
codes_16_sc db "0123456789ABCDEF"

section '.data' align 8

syscall_table:
    dq getch
    dq strcmp
    ; dq sys_read_file ; Сделать потом типо как в Линуксе да
MAX_SYSCALLS = 2