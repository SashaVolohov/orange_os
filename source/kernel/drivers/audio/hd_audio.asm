use64

HDA_REG_GCTL equ 0x08
HDA_REG_STATESTS equ 0x0E

HDA_REG_CORBLBASE  equ 0x40
HDA_REG_CORBUBASE  equ 0x44
HDA_REG_CORBWP     equ 0x48
HDA_REG_CORBRP     equ 0x4A
HDA_REG_CORBCTL    equ 0x4C
HDA_REG_CORBSIZE   equ 0x4E

HDA_REG_RIRBLBASE  equ 0x50
HDA_REG_RIRBUBASE  equ 0x54
HDA_REG_RIRBWP     equ 0x58
HDA_REG_RIRBCTL    equ 0x5C
HDA_REG_RIRBSIZE   equ 0x5E

HDA_REG_CORBWP     equ 0x48
HDA_REG_RIRBWP     equ 0x58

HDA_WIDGET_TYPE_AUDIO_OUTPUT equ 0x0
HDA_WIDGET_TYPE_AUDIO_INPUT  equ 0x1
HDA_WIDGET_TYPE_PIN_COMPLEX  equ 0x4

HDA_REG_SD0CTL     equ 0x160
HDA_REG_SD0CBL     equ 0x168
HDA_REG_SD0LVI     equ 0x16C
HDA_REG_SD0FMT     equ 0x172
HDA_REG_SD0BDPL    equ 0x178
HDA_REG_SD0BDPU    equ 0x17C

check_ics_support: ; CF 0 - success, 1 - error
    push rdx
    push rax

    mov edx, 0x000F0002

    call hda_send_verb_immediate
    jc .ics_failed

    test eax, eax
    jz .ics_failed

    clc
    jmp .exit

.ics_failed:
    stc

.exit:
    pop rax
    pop rdx
ret

hda_init_audio_system:

    mov r15, [hda_addr]
    call hda_init_controller
    
    call check_ics_support
    jc .use_corb_rirb_backend

.use_immediate_backend:

    mov rax, hda_send_verb_immediate
    mov [hda_send_verb_ptr], rax

    jmp .continue_init

.use_corb_rirb_backend:
    
    call alloc_page
    test rax, rax
    jz memory_management

    mov [corb_phys_addr], rax

    call alloc_page
    test rax, rax
    jz memory_management

    mov [rirb_phys_addr], rax

    mov r15, [hda_addr]
    mov rdi, [corb_phys_addr]
    mov rsi, [rirb_phys_addr]
    call hda_init_rings

    mov rax, hda_send_verb_dma_corb
    mov [hda_send_verb_ptr], rax

.continue_init:

ret

hda_init_controller: ; Input - R15 - MMIO Base Address
                     ; Output - CF - 0(success), CF - 1(error)
    push rax
    push rcx
    push rdx

    mov eax, [r15 + HDA_REG_GCTL]
    and eax, not 1
    mov [r15 + HDA_REG_GCTL], eax

    mov ecx, 10000
.wait_reset_clear:
    mov eax, [r15 + HDA_REG_GCTL]
    test eax, 1
    jz .reset_cleared
    dec ecx
    jnz .wait_reset_clear
    jmp .error

.reset_cleared:
    mov rdi, 2
    call sleep_ms

    mov eax, [r15 + HDA_REG_GCTL]
    or eax, 1
    mov [r15 + HDA_REG_GCTL], eax

.wait_crst:
    pause
    mov eax, [r15 + HDA_REG_GCTL]
    test eax, 1
    jz .wait_crst

    mov rdi, 10
    call sleep_ms

    mov word [r15 + 0x0E], 0xFFFF   

    mov rdi, 2
    call sleep_ms


.delay_done:
    movzx eax, word [r15 + 0x0E]
    test ax, ax
    jz .error

    mov byte [r15 + 0x83], 0xFF
    mov byte [r15 + 0xA3], 0xFF
    mov byte [r15 + 0xC3], 0xFF
    mov byte [r15 + 0xE3], 0xFF
    mov byte [r15 + 0x103], 0xFF
    mov byte [r15 + 0x123], 0xFF

    pause

    clc
    jmp .exit

.error:
    stc

.exit:
    pop rdx
    pop rcx
    pop rax
ret

hda_init_rings: ; Input: R15 = MMIO Base, RDI = CORB Phys Addr, RSI = RIRB Phys Addr
    push rax

    mov byte [r15 + HDA_REG_CORBCTL], 0
    mov byte [r15 + HDA_REG_RIRBCTL], 0

    mov rax, rdi
    mov [r15 + HDA_REG_CORBLBASE], eax
    shr rax, 32
    mov [r15 + HDA_REG_CORBUBASE], eax

    mov al, [r15 + HDA_REG_CORBSIZE]
    test al, 0x40
    jz .fallback_corb_size
    mov byte [r15 + HDA_REG_CORBSIZE], 0x02
.fallback_corb_size:

    mov word [r15 + HDA_REG_CORBRP], 0x8000

    mov word [r15 + HDA_REG_CORBWP], 0

    mov rax, rsi
    mov [r15 + HDA_REG_RIRBLBASE], eax
    shr rax, 32
    mov [r15 + HDA_REG_RIRBUBASE], eax

    mov byte [r15 + HDA_REG_RIRBSIZE], 0x02

    mov word [r15 + HDA_REG_RIRBWP], 0x8000

    mov byte [r15 + HDA_REG_RIRBCTL], 0x02

    mov byte [r15 + HDA_REG_CORBCTL], 0x02

    pop rax
ret

hda_send_verb_dma_corb: ; Input: R15 = MMIO Base Address (BAR0)
               ;        RDI = CORB buffer
               ;        RSI = RIRB buffer
               ;   EDX = 32-bit Verb(command)
               ; Output: RAX = 64-bit answer from codec (RIRB entry). Lower 32-bit — answer.
               ;   Carry Flag (CF) = 0 if success, CF = 1 if error
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi

    mov ecx, [corb_write_index]
    inc ecx
    and ecx, 0xFF
    mov [corb_write_index], ecx

    mov rbx, rcx
    shl rbx, 2
    mov [rdi + rbx], edx

    mov [r15 + HDA_REG_CORBWP], cx

    mov rbx, 100000

.wait_response:
    movzx eax, word [r15 + HDA_REG_RIRBWP]
    and eax, 0xFF

    cmp eax, [rirb_read_index]
    jne .response_received

    dec rbx
    jnz .wait_response
    jmp .timeout_error

.response_received:
    mov [rirb_read_index], eax

    shl eax, 3
    mov rax, [rsi + rax]

    clc
    jmp .exit

.timeout_error:
    stc
    xor rax, rax

.exit:
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
ret

hda_send_verb_immediate: 

    push rbx
    push rcx
    push rdx

    mov ecx, 10000000
.wait_ready:
    pause
    movzx eax, word [r15 + 0x68]
    test ax, 1
    jz .interface_free
    dec ecx
    jnz .wait_ready
    jmp .timeout_error

.interface_free:
    mov dword [r15 + 0x60], edx

    mov dword [r15 + 0x64], 0

    mov word [r15 + 0x68], 2

    mov word [r15 + 0x68], 3

    mov ecx, 3000000000
.wait_response:
    pause
    movzx eax, word [r15 + 0x68]
    
    test ax, 1
    jnz .next_tick
    
    test ax, 2
    jnz .success

.next_tick:
    dec ecx
    jnz .wait_response
    jmp .timeout_error

.success:
    mov eax, [r15 + 0x64]
    clc
    jmp .exit

.timeout_error:
    xor eax, eax
    stc

.exit:
    pop rdx
    pop rcx
    pop rbx
    ret

hda_scan_widgets: ; R15 = MMIO Base, RDI = CORB Phys, RSI = RIRB Phys
    push r12
    push r13
    push r14
    push rbx

    mov edx, 0x000F0004
    call [hda_send_verb_ptr]
    jc .error

    mov r12, rax
    shr r12, 16
    and r12, 0xFF
    
    mov r13, rax
    and r13, 0xFF

    test r13, r13
    jz .error

    mov edx, r12d
    shl edx, 20
    or edx, 0x000F0004
    call [hda_send_verb_ptr]
    jc .error

    mov r12, rax
    shr r12, 16
    and r12, 0xFF
    
    mov r13, rax
    and r13, 0xFF

    add r13, r12

.widget_loop:
    cmp r12, r13
    jge .scan_done

    mov edx, r12d
    shl edx, 20
    or edx, 0x000F0009
    call [hda_send_verb_ptr]
    jc .next_widget

    mov ebx, eax
    shr ebx, 20
    and ebx, 0x0F

    cmp ebx, HDA_WIDGET_TYPE_AUDIO_OUTPUT
    je .found_dac

    cmp ebx, HDA_WIDGET_TYPE_PIN_COMPLEX
    je .found_pin

    jmp .next_widget

.found_dac:
    call .log_dac_found
    jmp .next_widget

.found_pin:
    call .log_pin_found
    jmp .next_widget

.next_widget:
    inc r12
    jmp .widget_loop

.scan_done:
    pop rbx
    pop r14
    pop r13
    pop r12
    clc
ret

.error:
    pop rbx
    pop r14
    pop r13
    pop r12
    stc
ret

.log_dac_found:

    cmp [dac_id], 0
    jne .log_dac_found_exit

    mov [dac_id], r12b

.log_dac_found_exit:
ret

.log_pin_found:

    cmp [pin_id], 0
    jne .log_dac_found_exit

    mov [pin_id], r12b

.log_pin_found_exit:
ret

hda_setup_output_stream:
    push rax
    push rcx
    push r15

    mov dword [r15 + 0x20], 0xFFFFFFFF
    
    mov eax, [r15 + 0x160]
    and eax, not 0x02
    mov [r15 + 0x160], eax
    
    mov dword [r15 + 0x168], 0
    mov word [r15 + 0x16C], 0

    mov eax, [r15 + 0x160]
    or eax, 0x01
    mov [r15 + 0x160], eax

.vbox_stream_reset_wait:
    mov eax, [r15 + 0x160]
    test eax, 0x01
    jz .vbox_stream_reset_wait

    mov rdi, 10
    call sleep_ms

    mov eax, [r15 + 0x160]
    and eax, not 0x01
    mov [r15 + 0x160], eax

.vbox_stream_ready_wait:
    mov eax, [r15 + 0x160]
    test eax, 0x01
    jnz .vbox_stream_ready_wait

    mov rdi, 10
    call sleep_ms

    mov rax, r12
    mov [r15 + 0x178], eax
    shr rax, 32
    mov [r15 + 0x17C], eax

    mov [r15 + 0x168], r13d
    mov [r15 + 0x16C], r14w

    mov word [r15 + 0x172], 0x4011  ; SDxFMT

    mov eax, [r15 + 0x160]
    and eax, 0xFF0FFFFF
    or eax, 0x00100004
    mov [r15 + 0x160], eax
    
    push r15
    
    mov edx, 0x00270610
    mov r15, [hda_addr]
    mov rdi, [corb_phys_addr]
    mov rsi, [rirb_phys_addr]
    call [hda_send_verb_ptr]

    mov edx, 0x002A4011
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]

    mov edx, 0x0023B07F
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]

    mov edx, 0x00A71C10
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]
    mov edx, 0x00A71D40
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]
    mov edx, 0x00A71E21
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]
    mov edx, 0x00A71F00
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]

    mov edx, 0x00A70540
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]

    mov edx, 0x00A7077F
    mov r15, [hda_addr]
    call [hda_send_verb_ptr]

    pop r15
    or byte [r15 + 0x160], 0x02

    pop r15
    pop rcx
    pop rax
    ret

hda_addr dq 0
corb_phys_addr dq 0
rirb_phys_addr dq 0
corb_write_index dd 0
rirb_read_index dd 0
hda_send_verb_ptr dq 0

dac_id db 0
pin_id db 0