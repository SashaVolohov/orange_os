use64

draw_pixel: ; r15d - x, r14d - y, cl - R, bh - G, bl - B, ch - alpha

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

mov [esi], bl
mov [esi+1], bh
mov [esi+2], cl
mov [esi+3], ch

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

mov eax, get_string_from_int_32_string

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
mov cl, [eax]
mov [rbx], cl
add rbx, 1
add eax, 1
add edx, 1

cmp edx, 8
jne copy_efer

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
mov r13, 0
mov r13d, bsod_efer
mov r14d, 24
call draw_text

pop r13
pop r14
pop r15

ret

get_string_from_int_32_string db "0x00000000",0
codes_16_sc db "0123456789ABCDEF"