use64

public iso9660_init
public iso9660_find_file

public root_lba

section '.text' executable align 4096

align 4096
pvd_buffer rb 2048

root_lba    dd 0
root_size   dd 0

iso9660_init: ; Output - RAX 0(success), -1(error)
    push rdi
    push rsi
    push rdx

    mov rdi, pvd_buffer
    mov rsi, 16
    mov rdx, 1
    call read_atapi_sector

    cmp byte [pvd_buffer], 1
    jne .error
    
    cmp dword [pvd_buffer + 1], 'CD00'
    jne .error
    cmp byte [pvd_buffer + 5], '1'
    jne .error

    
    mov eax, dword [pvd_buffer + 156 + 2]
    mov [root_lba], eax
    
    mov eax, dword [pvd_buffer + 156 + 10]
    mov [root_size], eax

    mov rax, 0
    jmp .done

.error:
    mov rax, -1

.done:
    pop rdx
    pop rsi
    pop rdi
ret

iso9660_find_entry: ; RDI - pvd_buffer, RSI - filename, RDX - length
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14

    mov r12, rdi
    mov r13, rsi
    mov r14, rdx

    mov rbx, 0

.loop_records:
    cmp rbx, 2048
    jae .not_found

    lea rdi, [r12 + rbx]

    movzx eax, byte [rdi + 0]
    test al, al
    jz .next_sector_or_end

    mov r8, rax

    movzx ecx, byte [rdi + 32]
    
    ; --- НАЧАЛО ИЗМЕНЕНИЙ ---
    cmp rcx, r14
    jb .skip_entry
    
    mov rcx, r14
    mov rsi, r13
    lea rdx, [rdi + 33]

.compare_strings:
    mov al, [rsi]
    cmp al, [rdx]
    jne .skip_entry
    inc rsi
    inc rdx
    loop .compare_strings
    
    mov eax, [rdi + 2]
    mov ecx, [rdi + 10]
    jmp .done

.skip_entry:
    add rbx, r8
    jmp .loop_records

.next_sector_or_end:
    jmp .not_found

.not_found:
    xor rax, rax
    xor rcx, rcx
    
.done:
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
ret

iso9660_find_file:
    mov r8, rcx
    push r8

    call iso9660_find_entry

    pop r8

    test r8, r8
    jz .skip_size

    mov [r8], rcx

.skip_size:
    ret