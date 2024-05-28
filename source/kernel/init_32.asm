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
mov esp, 10000h
mov ax, 10h
mov gs, ax

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

add edi, 4
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