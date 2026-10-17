use64

public acpi_rsdp_addr
public lapic_base_addr

init_acpi_system:
    push rbp
    mov rbp, rsp

    call AcpiInitializeSubsystem

    jmp $

    test rax, rax
    jnz acpi_error

    xor rdi, rdi
    xor rsi, rsi
    mov rdx, 1
    call AcpiInitializeTables

    test rax, rax
    jnz acpi_error

    call AcpiLoadTables
    test rax, rax
    jnz acpi_error

    pop rbp
ret


lapic_base_addr    dq 0
io_apic_base_addr  dq 0
acpi_rsdp_addr dq 0
