use64

                         ; Input - RDI - buffer
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

    mov byte [r12], 0
    inc r12

    add rsi, 21
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

    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
ret