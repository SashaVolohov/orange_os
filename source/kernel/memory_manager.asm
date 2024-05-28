align 8;

GDT_64:

dq 0;

dw 0FFFFh, 0, 9A00h, 0AFh;
dw 0FFFFh, 0, 9200h, 0AFh;

label GDT_64_SIZE at $-GDT_64

GDTR_64:

dw GDT_64_SIZE-1
dq GDT_64

memory_init:

mov rax, 1A00h
mov rbx, 0
mov rcx, 0
mov bx, [1900h]
mov rdx, 0
mov dx, [1800h]

detect_memory_length:

add rcx, [rax+8h]
add rax, rdx

cmp rax, rbx
jne detect_memory_length

; rcx - amount of RAM

mov rax, rcx
mov rdx, 0
mov ebx, 1024
div ebx

mov edx, eax
mov rax, 0
mov eax, edx
mov rdx, 0
mov ebx, 1024
div ebx
mov edx, eax
mov rax, 0
mov eax, edx
mov rdx, 0

add rax, 1

mov [mb_memory], rax

; rax - amount of RAM in MB


mov dword [10000h], 11000h + 111b
mov dword [11000h], 12000h + 111b

mov rax, [mb_memory]
mov edx, 0
mov ebx, 2
div ebx

mov edi, 12000h
mov ecx, eax
mov eax, 13000h + 111b

make_page_2_entries:

stosd

add edi, 4
add eax, 1000h
loop make_page_2_entries

mov rax, [mb_memory]
mov edx, 0
mov ebx, 2
div ebx

mov ebx, 512
mul ebx

mov ecx, eax
mov edi, 13000h
mov eax, 0 + 111b

make_page_entries_64:

stosd

add edi, 4
add eax, 1000h
loop make_page_entries_64

load_new_page_table:

mov rax, 10000h
mov cr3, rax

ret

videomem_init:

mov rsi, 0
mov esi, [lfb_buffer_addr]
shr esi, 30

mov ebx, 11000h
mov eax, esi
mov ecx, 8

mul ecx

add ebx, eax

mov qword [ebx], 100000h + 111b

mov esi, [lfb_buffer_addr]
and esi, 3FFFFFFFh
shr esi, 21

mov ebx, 100000h
mov eax, esi
mov ecx, 8

mul ecx

add ebx, eax

mov qword [ebx], 101000h + 111b

mov eax, [lfb_buffer_addr]
add eax, 111b
mov edi, 101000h
mov ecx, 512

make_video_entries:

stosd

add edi, 4
add eax, 1000h
loop make_video_entries

ret

mb_memory dq 0