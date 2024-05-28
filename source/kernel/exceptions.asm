use64

align 8;

IDT:

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw general_protection_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

dw page_fault, 08h, 8E00h, 0000h, 0000h, 0000h, 0000h, 0000h;

label IDT_SIZE at $-IDT

IDTR:

dw IDT_SIZE-1

dq IDT

general_protection_fault:

jmp $

page_fault:

mov ecx, 0C0000080h
rdmsr
mov [efer_bsod], eax

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov eax, 480000

gui_screen_page_fault:

mov [rsi], byte 255
mov [rsi+1], byte 0
mov [rsi+2], byte 0
mov [rsi+3], byte 255

dec eax
add rsi, 4
cmp eax, 00h
jne gui_screen_page_fault
je continue_page_fault

continue_page_fault:

mov r13, bsod_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

mov r15d, [efer_bsod]
mov r14, bsod_efer
mov r13, 6
mov r12d, 0
mov r11d, 24
call print_format_string_with_32_int

mov r13, bsod_rax
mov r15d, 0
mov r14d, 48
call draw_text

mov r13, bsod_rcx
mov r14d, 60
call draw_text

mov r13, bsod_sp
mov r14d, 72
call draw_text

mov r13, bsod_rsi
mov r14d, 84
call draw_text

mov r13, bsod_r8
mov r14d, 96
call draw_text

mov r13, bsod_r10
mov r14d, 108
call draw_text

mov r13, bsod_r12
mov r14d, 120
call draw_text

mov r13, bsod_r14
mov r14d, 132
call draw_text

mov r13, bsod_rflags
mov r14d, 158
call draw_text

jmp $

bsod_text db '*** STOP: 0x0000000E (PAGE_FAULT_IN_NONPAGED_AREA)',0
bsod_efer db 'EFER: 0x00000000',0
bsod_rax db 'RAX: 0x0000000000000000, RBX: 0x0000000000000000',0
bsod_rcx db 'RCX: 0x0000000000000000, RDX: 0x0000000000000000',0
bsod_sp db 'RSP: 0x0000000000000000, RBP: 0x0000000000000000',0
bsod_rsi db 'RSI: 0x0000000000000000, RDI: 0x0000000000000000',0
bsod_r8 db 'R8: 0x0000000000000000, R9: 0x0000000000000000',0
bsod_r10 db 'R10: 0x0000000000000000, R11: 0x0000000000000000',0
bsod_r12 db 'R12: 0x0000000000000000, R13: 0x0000000000000000',0
bsod_r14 db 'R14: 0x0000000000000000, R15: 0x0000000000000000',0
bsod_rflags db 'RFLAGS: 0x0000000000000000',0