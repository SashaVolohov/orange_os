use64

                         ; Input - RDI - buffer(12 bytes - filename, 4 bytes - size)
                         ; Output - RAX - count of files
get_file_list:
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi
    xor r13, r13
    mov r14, 19

.sector_loop:
    mov rax, r14
    mov rdi, 0x4000
    call read_floppy_sector

    mov rsi, 0x4000
    mov rcx, 16

.entry_loop:
    push rcx

    mov al, [rsi]
    cmp al, 0x00
    je .catalog_ended
    cmp al, 0xE5
    je .skip_entry

    mov al, [rsi + 11]
    test al, 0x08
    jnz .skip_entry

    inc r13

    mov rcx, 8
.copy_name:
    lodsb 
    mov [r12], al
    inc r12
    loop .copy_name

    mov byte [r12], '.'
    inc r12

    mov rcx, 3
.copy_ext:
    lodsb
    mov [r12], al
    inc r12
    loop .copy_ext

    add rsi, 17 ; To file size
    mov r15d, [rsi]
    mov [r12], r15d

    add r12, 4

    add rsi, 4 ; To the next entry
    pop rcx
    loop .entry_loop
    jmp .next_sector

.skip_entry:
    add rsi, 32
    pop rcx
    loop .entry_loop

.next_sector:
    inc r14
    cmp r14, 33
    jl .sector_loop
    jmp .done

.catalog_ended:
    pop rcx

.done:
    mov rax, r13

    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
ret

read_file: ; Input: RSI(filename 11 byte), RDI(buffer addr)
           ; Output: RAX - bytes read succesfully, 0 if file not found/error
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    xor rbx, rbx
    mov [current_progress], rbx

    mov r12, rsi
    mov r13, rdi
    xor r15, r15

    mov r14, 19

.root_sector_loop:
    mov rax, r14
    mov rdi, 0x4000
    call read_floppy_sector

    mov rsi, 0x4000
    mov rcx, 16

.entry_loop:
    mov al, [rsi]
    cmp al, 0x00
    je .file_not_found
    cmp al, 0xE5
    je .skip_entry

    push rcx
    push rsi
    mov rdi, r12
    mov rcx, 11
    repe cmpsb
    pop rsi
    pop rcx
    je .file_found

.skip_entry:
    add rsi, 32
    loop .entry_loop

    inc r14
    cmp r14, 33
    jl .root_sector_loop
    jmp .file_not_found

.file_found:
    movzx r14, word [rsi + 26]
    
    mov r15, qword [rsi + 28]
    mov r15d, r15d

    mov rdi, r13

.next_cluster:
    cmp r14, 0x0FF8
    jae .done_reading

    mov rax, r14
    add rax, 31

    call read_floppy_sector

    mov rbx, 1
    add [current_progress], rbx

    add rdi, 512

    mov rax, r14
    mov rbx, 3
    mul rbx
    shr rax, 1
    
    push rax
    shr rax, 9
    inc rax
    
    push rdi
    mov rdi, 0x4000
    call read_floppy_sector
    pop rdi
    pop rax

    and rax, 511            ; rax = rax % 512
    mov rbx, 0x4000
    add rbx, rax
    
    movzx rax, word [rbx]

    test r14, 1
    jnz .odd_cluster

.even_cluster:
    and rax, 0x0FFF
    jmp .cluster_resolved

.odd_cluster:
    shr rax, 4

.cluster_resolved:
    mov r14, rax
    jmp .next_cluster

.file_not_found:
    xor r15, r15

.done_reading:
    mov rax, r15

    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
ret