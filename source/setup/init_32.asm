use32

align 8;

GDT:

dq 0;

db 0FFh, 0FFh, 0, 0, 0, 9Ah, 0CFh, 0;
db 0FFh, 0FFh, 0, 0, 0, 92h, 0CFh, 0;

label GDT_SIZE at $-GDT

GDTR:

dw GDT_SIZE-1

dd GDT

; Protected mode

startup32:

mov ax, 10h
mov es, ax
mov ds, ax
mov fs, ax
mov ss, ax
mov esp, 7000h
mov ax, 10h
mov gs, ax

mov esi, switch_to_long_mode
call draw_status_bar_pm_flat

mov eax, cr4
or eax, 1 shl 5
mov cr4, eax

mov edi, 70000h
mov ecx, 4000h shr 2
xor eax, eax
rep stosd

mov dword [70000h], 71000h + 111b
mov dword [71000h], 72000h + 111b
mov dword [72000h], 73000h + 111b

mov edi, 73000h
mov eax, 0 + 111b
mov ecx, 256

make_page_entries:

stosd
xor edx, edx
xchg eax, edx
stosd
xchg eax, edx

add eax, 1000h
loop make_page_entries

mov eax, 70000h
mov cr3, eax

mov ecx, 0C0000080h
rdmsr
or eax, 1 shl 8
wrmsr

mov eax, cr0
or eax, 1 shl 31
mov cr0, eax

lgdt [GDTR_64]

jmp 08h:long_start

draw_status_bar_pm_flat: ; ESI - text
    pushad

    mov edi, 0xB8000
    add edi, 3840
    
    mov ecx, 80
    mov ah, 70h

.text_loop:
    mov al, [esi]
    inc esi
    cmp al, 0
    je .fill_loop

    mov [edi], ax
    add edi, 2
    dec ecx
    jz .done
    jmp .text_loop

.fill_loop:
    mov al, ' '
.fill_loop_run:
    jecxz .done
    mov [edi], ax
    add edi, 2
    dec ecx
    jmp .fill_loop_run

.done:
    popad
ret

switch_to_long_mode db 'Setup is switching to long mode...',0