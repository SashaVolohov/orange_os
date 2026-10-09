PIT_COMMAND equ 0x43
PIT_CHANNEL0 equ 0x40

timer_ticks dq 0

use64

irq0_timer_handler:

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    inc qword [timer_ticks]

    xor ecx, ecx
    mov rbx, [process_table_ptr]

.wakeup_loop:
    cmp ecx, MAX_PROCESSES
    jae .wakeup_done

    mov rax, rcx
    imul rax, struct_pcb_size
    lea rdx, [rbx + rax]

    cmp dword [rdx + PCB_state], TASK_STATE_SLEEP
    jne .next_wakeup_slot

    mov rax, [timer_ticks]
    cmp rax, qword [rdx + PCB_ticks]
    jb .next_wakeup_slot

    mov dword [rdx + PCB_state], TASK_STATE_READY

.next_wakeup_slot:
    inc ecx
    jmp .wakeup_loop

.wakeup_done:


    mov eax, [current_task_index]
    imul rax, struct_pcb_size
    mov rbx, [process_table_ptr]
    add rbx, rax

    mov [rbx + PCB_rsp], rsp

    cmp dword [rbx + PCB_state], TASK_STATE_RUNNING
    jne .skip_state_change
    mov dword [rbx + PCB_state], TASK_STATE_READY
.skip_state_change:

    mov ecx, [current_task_index]

.search_next_task:
    inc ecx
    cmp ecx, MAX_PROCESSES
    jb .check_slot
    xor ecx, ecx

.check_slot:
    mov rax, rcx
    imul rax, struct_pcb_size
    mov rdx, [process_table_ptr]
    add rdx, rax

    mov eax, [rdx + PCB_state]
    cmp eax, TASK_STATE_READY
    je .task_found
    cmp eax, TASK_STATE_RUNNING
    je .task_found

    cmp ecx, [current_task_index]
    jne .search_next_task

    mov ecx, [current_task_index]
    mov rdx, rbx

.task_found:
    mov [current_task_index], ecx
    mov dword [rdx + PCB_state], TASK_STATE_RUNNING

    mov rax, qword [rdx + 80]
    mov rbx, cr3

    cmp rax, rbx
    je .skip_cr3_switch

    mov cr3, rax

.skip_cr3_switch:

    mov rsp, [rdx + PCB_rsp]

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

mov qword [rsp + 16], 0x200 

iretq