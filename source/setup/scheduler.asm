use64

TASK_STACK_LIMIT equ 0xFFFF800000000000

TSS64_rsp0       equ 4
TSS64_rsp1       equ 12
TSS64_rsp2       equ 20
TSS64_ist1       equ 36
TSS64_ist2       equ 44
TSS64_iomap_base equ 102

align 16
tss0:
    times 104 db 0

install_tss_descriptor:
    push rax
    push rbx
    push rdx

    lea rax, [tss0]
    
    lea rbx, [GDT_64 + 40] 

    mov word [rbx], 103
    
    mov [rbx + 2], ax
    shr rax, 16
    
    mov [rbx + 4], al
    shr rax, 8
    
    mov byte [rbx + 5], 0x89
    
    mov byte [rbx + 6], 0x00
    
    mov [rbx + 7], al
    shr rax, 8
    
    mov [rbx + 8], eax
    
    mov dword [rbx + 12], 0

    pop rdx
    pop rbx
    pop rax
ret

align 8
tmp_ret_addr dq 0
tmp_rcx      dq 0
tmp_rbx      dq 0
tmp_rdi      dq 0

tasking_init:

    mov qword [tmp_ret_addr], rbp
    mov [tmp_rcx], rcx
    mov [tmp_rbx], rbx
    mov [tmp_rdi], rdi

    mov rax, [rsp]
    mov [tmp_ret_addr], rax

    mov rdi, MAX_PROCESSES * struct_pcb_size
    call kmalloc

    test rax, rax
    jz memory_management
    mov [process_table_ptr], rax

    mov rdi, [process_table_ptr]
    mov rcx, (MAX_PROCESSES * struct_pcb_size) / 8
    xor rax, rax
    rep stosq

    ; PROCESS 0: System Idle Process
    mov rbx, [process_table_ptr]
    mov dword [rbx + PCB_pid], 0
    mov dword [rbx + PCB_state], TASK_STATE_READY
    
    mov rax, [pt_begin]
    mov [rbx + PCB_cr3], rax
    mov qword [rbx + PCB_rflags], 0x200
    
    mov rcx, 4
    mov r12, TASK_STACK_LIMIT
.map_idle_stack_loop:
    push rcx
    call alloc_page
    mov rsi, rax
    mov rdi, r12
    mov rdx, 111b
    call map_page
    pop rcx
    add r12, 4096
    loop .map_idle_stack_loop

    mov rdi, r12
    sub rdi, 8
    and rdi, not 0xF
    mov rdx, rdi

    mov qword [rdi], 0x10   ; SS
    sub rdi, 8
    mov [rdi], rdx          ; RSP
    sub rdi, 8
    mov qword [rdi], 0x200  ; RFLAGS
    sub rdi, 8
    mov qword [rdi], 0x08   ; CS
    sub rdi, 8
    lea rax, [kernel_idle_task]
    mov [rdi], rax          ; RIP

    mov rdx, 15
.push_fake_idle_regs:
    sub rdi, 8
    mov qword [rdi], 0
    dec rdx
    jnz .push_fake_idle_regs
    mov [rbx + PCB_rsp], rdi

    ; PID 1: Kernel Main Task
    mov rbx, [process_table_ptr]
    add rbx, struct_pcb_size
    
    mov dword [rbx + PCB_pid], 1
    mov dword [rbx + PCB_state], TASK_STATE_RUNNING
    
    mov rax, [pt_begin]
    mov [rbx + PCB_cr3], rax
    mov qword [rbx + PCB_rflags], 0x200

    mov rcx, 4
    mov r12, TASK_STACK_LIMIT + (16 * 4096)

.map_kernel_stack_loop:
    push rcx
    call alloc_page
    mov rsi, rax
    mov rdi, r12
    mov rdx, 111b
    call map_page
    pop rcx
    add r12, 4096
    loop .map_kernel_stack_loop

    mov rdi, r12
    sub rdi, 8
    and rdi, not 0xF

    mov rdx, [tmp_ret_addr]
    mov [rdi], rdx
    
    mov rdx, [tmp_rdi]
    sub rdi, 8
    mov [rdi], rdx

    mov rdx, [tmp_rbx]
    sub rdi, 8
    mov [rdi], rdx

    mov rdx, [tmp_rcx]
    sub rdi, 8
    mov [rdi], rdx

    mov rsp, rdi

    mov dword [current_task_index], 1
    mov dword [total_processes], 2

    pop rcx
    pop rbx
    pop rdi
ret

create_task: ; Input - RDI(addr of entry point)
             ; Output - RAX(PID)
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r12
    push r13

    mov r12, rdi

    mov rbx, [process_table_ptr]
    xor rcx, rcx

.find_slot_loop:
    cmp rcx, MAX_PROCESSES
    jae .no_free_slots

    mov rax, rcx
    imul rax, struct_pcb_size
    lea rsi, [rbx + rax]

    mov edx, [rsi + PCB_state]
    cmp edx, TASK_STATE_ZOMBIE
    je .slot_found
    
    test edx, edx
    jnz .next_slot
    cmp dword [rsi + PCB_pid], 0
    je .slot_found

.next_slot:
    inc rcx
    jmp .find_slot_loop

.slot_found:
    mov r13, rsi

    push rcx
    call alloc_page
    pop rcx
    test rax, rax
    jz .no_free_slots

    mov rbx, rcx
    shl rbx, 12
    mov rdi, TASK_STACK_LIMIT
    add rdi, rbx

    mov rsi, rax
    mov rdx, 111b
    push rcx
    call map_page
    pop rcx
    
    add rdi, 4096
    sub rdi, 8

    push rcx
    push rdi
    mov rdi, r13
    mov rcx, struct_pcb_size / 8
    xor rax, rax
    pop rdi
    pop rcx

    mov [r13 + PCB_pid], ecx
    mov dword [r13 + PCB_state], TASK_STATE_READY
    
    mov rax, [pt_begin]
    mov [r13 + PCB_cr3], rax

    mov [r13 + PCB_rip], r12 
    
    mov eax, ecx

    mov qword [rdi], 0x10
    
    sub rdi, 8
    mov [rdi], rdi 
    add qword [rdi], 8
    
    sub rdi, 8
    mov qword [rdi], 0x200
    
    sub rdi, 8
    mov qword [rdi], 0x08
    
    sub rdi, 8
    mov [rdi], r12

    mov rbx, 15
.push_fake_regs_fixed:
    sub rdi, 8
    mov qword [rdi], 0
    dec rbx
    jnz .push_fake_regs_fixed

    mov [r13 + PCB_rsp], rdi
    mov qword [r13 + PCB_rflags], 0x200

    inc dword [total_processes]
    jmp .done

.no_free_slots:
    mov rax, -1

.done:
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
ret

process_table_ptr dq 0
current_task_index dd 0
total_processes    dd 0