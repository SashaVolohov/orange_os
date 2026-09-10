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
mov rbx, 1048576
div rbx

add rax, 1

mov [mb_memory], rax
mov [fake_mb_memory], rax

mov rax, [mb_memory]
cmp rax, 128
ja fake_memory
jmp make_entries

fake_memory:
mov [fake_mb_memory], 128

make_entries:

mov rax, [fake_mb_memory]
shr rax, 1
add rax, 3


imul rax, 512          
mov rcx, rax            

mov rdi, 10000h         
xor rax, rax            

rep stosq              
                        

mov dword [10000h], 11000h + 111b
mov dword [11000h], 12000h + 111b

mov rax, [fake_mb_memory]
shr rax, 1            
mov ecx, eax            

mov edi, 12000h         
mov rbx, 13000h + 111b  

make_pd_entries:
mov [rdi], rbx       
add rdi, 8             
add rbx, 1000h         
loop make_pd_entries

mov rax, [fake_mb_memory]
shr rax, 1             
imul rax, 512          
mov ecx, eax          

mov edi, 13000h        
mov rbx, 0 + 111b       

make_pt_entries:
mov [rdi], rbx         
add rdi, 8            
add rbx, 1000h          
loop make_pt_entries

mov [pt_end], rdi

load_new_page_table:
mov rax, 10000h
mov cr3, rax

mov rax, [mb_memory]
cmp rax, 128
ja make_new_128_entries
ret

make_new_128_entries:

mov [pt_begin], 100000h

mov rax, [mb_memory]
shr rax, 1
add rax, 3

imul rax, 512
mov rcx, rax

mov rdi, 100000h
xor rax, rax

rep stosq

mov dword [100000h], 101000h + 111b
mov dword [101000h], 102000h + 111b

mov rax, [mb_memory]
shr rax, 1
mov ecx, eax

mov edi, 102000h
mov rbx, 103000h + 111b

make_pd_entries_128:
mov [rdi], rbx
add rdi, 8
add rbx, 1000h
loop make_pd_entries_128

mov rax, [mb_memory]
shr rax, 1
imul rax, 512
mov ecx, eax

mov edi, 103000h
mov rbx, 0 + 111b

make_pt_entries_128:
mov [rdi], rbx
add rdi, 8
add rbx, 1000h
loop make_pt_entries_128

mov [pt_end], rdi

load_new_page_table_128:
mov rax, 100000h
mov cr3, rax

ret

videomem_init:

mov rax, [mb_memory]
cmp rax, 128
ja videomem_more_128

videomem_less_128:

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

mov [ptv_end], rdi

ret

videomem_more_128:


mov rsi, 0
mov esi, [lfb_buffer_addr]
shr esi, 30

mov ebx, 101000h
mov eax, esi
mov ecx, 8

mul ecx

add ebx, eax

mov r10, [pt_end]

mov qword [ebx], r10
add qword [ebx], 111b

mov esi, [lfb_buffer_addr]
and esi, 3FFFFFFFh
shr esi, 21

mov eax, esi
shl eax, 3

mov rbx, r10
add rbx, rax

mov rdi, r10
add rdi, 1000h

mov qword [rbx], rdi
add qword [rbx], 111b

mov eax, [lfb_buffer_addr]
add eax, 111b

mov ecx, 512

make_video_entries_128:

stosd

add edi, 4
add eax, 1000h
loop make_video_entries_128

mov [ptv_end], rdi



ret

allocator_init:

mov rax, [mb_memory]
mov rbx, 4
mul rbx
mov rcx, rax

mov rdi, [ptv_end]
mov rax, 0xFFFFFFFFFFFFFFFF
rep stosq

mov [bitmap_end], rdi

ret

get_used_memory_mb: ; rax - count of used memory in MB
    push rbx
    push rcx
    push rdx
    push r10

    mov rsi, [ptv_end]

    mov r10, [bitmap_end]
    sub r10, [ptv_end]
    mov rcx, r10

    shr rcx, 3
    jz .empty_bitmap

    xor rax, rax

.loop_quadwords:
    mov rdx, [rsi]
    popcnt rdx, rdx
    add rax, rdx
    add rsi, 8
    loop .loop_quadwords
    
    shr rax, 8

.exit:
    pop r10
    pop rdx
    pop rcx
    pop rbx
    ret

.empty_bitmap:
    xor rax, rax
    jmp .exit

free_page: ; rdi - page addr(4 KB)

    push rbx

    shr rdi, 12
    
    mov rbx, [ptv_end]
    btr [rbx], rdi

    pop rbx

ret

alloc_page: ; RAX - addr of allocated page. If 0 - out of memory, error

push rbx
push rdx
push rcx
push rsi

    mov rbx, [ptv_end]
    mov rdx, [bitmap_end]
    xor rcx, rcx

.loop_qwords:
    mov rsi, rbx
    add rsi, rcx 
    
    cmp rsi, rdx
    jae .out_of_memory
    
    mov rax, [rsi]
    not rax
    
    test rax, rax
    jz .next_qword
    
    bsf rdx, rax

    shl rcx, 3
    add rdx, rcx
    
    bts [rbx], rdx
    
    mov rax, rdx
    shl rax, 12
    jmp .exit

.next_qword:
    add rcx, 8
    jmp .loop_qwords

.out_of_memory:
    xor rax, rax
    jmp .exit

.exit:
    pop rsi
    pop rcx
    pop rdx
    pop rbx
    ret

extract_paging_indices: ; Input: RDI - virtual addr
                        ; R9 - PML4
                        ; R10 - PDPT
                        ; R11 - PD
                        ; R12 - PT
    mov rax, rdi
    shr rax, 39
    and rax, 0x1FF
    mov r9, rax

    mov rax, rdi
    shr rax, 30
    and rax, 0x1FF
    mov r10, rax

    mov rax, rdi
    shr rax, 21
    and rax, 0x1FF
    mov r11, rax

    mov rax, rdi
    shr rax, 12
    and rax, 0x1FF
    mov r12, rax
    ret

map_page: ; Input: rdi - virtual addr, rsi - physical addr, rdx - access flags
          ; rax - 1(success), 0(error)
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r13, rdi
    mov r14, rsi
    mov r15, rdx

    mov rax, r13
    shr rax, 39
    and rax, 0x1FF
    mov r9, rax

    mov rax, r13
    shr rax, 30
    and rax, 0x1FF
    mov r10, rax

    mov rax, r13
    shr rax, 21
    and rax, 0x1FF
    mov r11, rax

    mov rax, r13
    shr rax, 12
    and rax, 0x1FF
    mov r12, rax

    mov rax, [pt_begin]
    shl r9, 3
    add rax, r9
    
    mov rdi, rax
    call get_or_create_table
    test rax, rax
    jz .error

    shl r10, 3
    add rax, r10
    
    mov rdi, rax
    call get_or_create_table
    test rax, rax
    jz .error

    shl r11, 3
    add rax, r11

    mov rdi, rax
    call get_or_create_table
    test rax, rax
    jz .error

    shl r12, 3
    add rax, r12

    mov rbx, r14
    and rbx, not 0xFFF
    or rbx, r15

    mov [rax], rbx

    invlpg [r13]

    mov rax, 1
    jmp .done

.error:
    xor rax, rax

.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

get_or_create_table:
    push rbx
    push rdi

    mov rax, [rdi]
    test rax, 1
    jnz .table_exists

    push rdi
    call alloc_page
    pop rdi
    test rax, rax
    jz .out_of_mem

    push rax
    push rdi
    mov rdi, rax
    mov rcx, 512
    xor rax, rax
    rep stosq
    pop rdi
    pop rax

    mov rbx, rax

    or rbx, 111b
    mov [rdi], rbx

    jmp .table_exists

.out_of_mem:
    xor rax, rax
    pop rdi
    pop rbx
    ret

.table_exists:
    and rax, not 0xFFF      
    pop rdi
    pop rbx
    ret

unmap_page: ;rdi - virtual addr
    push rbx
    push r12
    push r13
    push rdi

    mov r13, rdi

    mov rax, r13
    shr rax, 39
    and rax, 0x1FF
    mov r9, rax

    mov rax, r13
    shr rax, 30
    and rax, 0x1FF
    mov r10, rax

    mov rax, r13
    shr rax, 21
    and rax, 0x1FF
    mov r11, rax

    mov rax, r13
    shr rax, 12
    and rax, 0x1FF
    mov r12, rax

    mov rax, [pt_begin]
    shl r9, 3
    add rax, r9
    
    mov rax, [rax]
    test rax, 1
    jz .not_mapped
    and rax, not 0xFFF

    shl r10, 3
    add rax, r10
    
    mov rax, [rax]
    test rax, 1
    jz .not_mapped
    and rax, not 0xFFF

    shl r11, 3
    add rax, r11
    
    mov rax, [rax]
    test rax, 1
    jz .not_mapped
    
    test rax, 0x80          
    jnz .not_mapped
    
    and rax, not 0xFFF

    shl r12, 3
    add rax, r12

    mov rbx, [rax]
    test rbx, 1
    jz .not_mapped

    and rbx, not 0xFFF
    
    push rax
    mov rdi, rbx
    call free_page
    pop rax

    mov qword [rax], 0

    invlpg [r13]

    mov rax, 1
    jmp .done

.not_mapped:
    xor rax, rax

.done:
    pop rdi
    pop r13
    pop r12
    pop rbx
    ret

mb_memory dq 0
fake_mb_memory dq 0
pt_begin dq 10000h
pt_end dq 100000h
ptv_end dq 100000h
bitmap_end dq 100000h