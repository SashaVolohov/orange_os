use64

MADT_TYPE_LOCAL_APIC           = 0
MADT_TYPE_IO_APIC              = 1
MADT_TYPE_INT_SRC_OVERRIDE     = 2
MADT_TYPE_LOCAL_APIC_NMI       = 4

MADT_LAPIC_ADDRESS_OFFSET      = 36
MADT_FLAGS_OFFSET              = 40
MADT_FIRST_ENTRY_OFFSET        = 44

virtual at 0
    acpi.signature    dd ?
    acpi.length       dd ?
    acpi.revision     db ?
    acpi.checksum     db ?
    acpi.oem_id       db 6 dup (?)
    acpi.oem_table_id db 8 dup (?)
    acpi.oem_revision dd ?
    acpi.creator_id   dd ?
    acpi.creator_rev  dd ?
end virtual

init_acpi:

    mov rax, 0x000E0000

    .main_loop:

    mov rbx, [rax]
    mov rcx, "RSD PTR "

    cmp rbx, rcx
    je .validate_checksum

    add rax, 16
    
    mov rdx, 0x000FFFFF
    cmp rax, rdx
    jae .acpi_not_found

    jmp .main_loop

.validate_checksum:
    xor ecx, ecx
    xor rsi, rsi

.checksum_loop:
    movzx rbx, byte [rax + rsi]
    add ecx, ebx
    inc rsi
    cmp rsi, 20
    jl .checksum_loop

    test cl, cl
    jnz .main_loop 

.address_found:

mov [acpi_rsdp_addr], rax

call parse_rsdt
call parse_madt

ret

.acpi_not_found:

jmp acpi_error

parse_rsdt:
    mov rax, [acpi_rsdp_addr]
    test rax, rax
    jz .rsdt_error

    mov ecx, dword [rax + 16]
    mov [acpi_rsdt_addr], rcx

    mov rdi, rcx
    mov rsi, rcx
    mov rdx, 0x13
    call map_page
    test rax, rax
    jz .rsdt_error

    mov rsi, [acpi_rsdt_addr]
    mov ecx, [rsi + acpi.length]
    
    mov rbx, [acpi_rsdt_addr]
    mov r8, rbx
    add r8, rcx 
    
    and rbx, 0xFFFFFFFFFFFFF000
    add rbx, 4096

.map_remaining_pages:
    cmp rbx, r8
    jae .mapping_done

    mov rdi, rbx
    mov rsi, rbx
    mov rdx, 0x13
    push r8
    push rcx
    push rsi
    call map_page
    pop rsi
    pop rcx
    pop r8
    test rax, rax
    jz .rsdt_error

    add rbx, 4096
    jmp .map_remaining_pages

.mapping_done:
    xor eax, eax
    xor rbx, rbx

.checksum_loop:
    movzx rdx, byte [rsi + rbx]
    add eax, edx
    inc rbx
    cmp rbx, rcx
    jl .checksum_loop

    test al, al
    jnz .rsdt_error

    mov ecx, [rsi + acpi.length]
    sub ecx, 36
    shr ecx, 2
    mov [rsdt_entries_count], ecx

    ret

.rsdt_error:
    jmp acpi_error

find_table_in_rsdt: ; Input: EAX - signature
                    ; Output: RAX - addr of table(0 if error)
    mov rsi, [acpi_rsdt_addr]
    add rsi, 36
    mov ecx, [rsdt_entries_count]
    test ecx, ecx
    jz .not_found

.search_loop:
    mov edi, dword [rsi]
    mov rbx, qword [rdi]
    cmp ebx, eax
    je .found

    add rsi, 4
    dec ecx
    jnz .search_loop

.not_found:
    xor rax, rax
    ret

.found:
    mov rax, rdi
    ret

parse_madt:
    mov eax, 'APIC'
    call find_table_in_rsdt
    test rax, rax
    jz .madt_not_found

    mov [madt_addr], rax

    mov ecx, dword [rax + MADT_LAPIC_ADDRESS_OFFSET]
    mov [lapic_base_addr], rcx

    mov rdi, rcx
    mov rsi, rcx
    mov rdx, 0x13
    push rax
    call map_page
    pop rax
    test rax, rax
    jz acpi_error

.start_parsing_entries:
    mov ecx, dword [rax + 4]
    add rcx, rax
    
    lea rsi, [rax + MADT_FIRST_ENTRY_OFFSET]

.entry_loop:
    cmp rsi, rcx
    jae .parsing_done

    movzx rbx, byte [rsi + 0]
    movzx rdx, byte [rsi + 1]

    test rdx, rdx
    jz .parsing_done 

    cmp rbx, MADT_TYPE_LOCAL_APIC
    je .handle_local_apic

    cmp rbx, MADT_TYPE_IO_APIC
    je .handle_io_apic

    cmp rbx, MADT_TYPE_INT_SRC_OVERRIDE
    je .handle_interrupt_override

.next_entry:
    add rsi, rdx
    jmp .entry_loop

.handle_local_apic:
    mov r8d, dword [rsi + 4]
    test r8d, 1
    jz .next_entry

    movzx r9d, byte [rsi + 3]
    
    mov r10d, dword [cpu_count]
    mov byte [cpus_temp_array + r10], r9b

    inc dword [cpu_count]
    jmp .next_entry

.handle_io_apic:

    mov r8d, dword [rsi + 4]
    mov [io_apic_base_addr], r8

    mov r9d, dword [rsi + 8]
    mov [io_apic_gsi_base], r9d

    push rcx
    push rsi
    push rdx

    mov r8d, dword [rsi + 4]
    
    mov rdi, r8
    mov rsi, r8
    mov rdx, 0x13
    call map_page
    
    pop rdx
    pop rsi
    pop rcx
    
    test rax, rax
    jz acpi_error

    ; I/O APIC can be not one, but two and etc...
    jmp .next_entry

.handle_interrupt_override:
    movzx r8d, byte [rsi + 2]
    movzx r9d, byte [rsi + 3]
    mov r10d, dword [rsi + 4]
    
    jmp .next_entry

.parsing_done:
    ret

.madt_not_found:
    jmp acpi_error


madt_addr          dq 0
lapic_base_addr    dq 0
io_apic_base_addr  dq 0
io_apic_gsi_base   dd 0

acpi_rsdp_addr dq 0

acpi_rsdt_addr dq 0
rsdt_entries_count dd 0