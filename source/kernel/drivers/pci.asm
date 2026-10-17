use64

public total_devices

pci_read_config_dword: ; Input - RDI = Bus, RSI = Slot, RDX = Function, RCX = Offset
                       ; Output - EAX = data (32-bit). 0xFFFFFFFF, if device is missing
    push rbx

    movzx eax, dil
    shl eax, 16

    movzx ebx, sil
    shl ebx, 11
    or eax, ebx

    movzx ebx, dl
    shl ebx, 8
    or eax, ebx

    and ecx, 0xFC
    or eax, ecx

    or eax, 0x80000000

    mov edx, 0x0CF8
    out dx, eax

    mov edx, 0x0CFC
    in eax, dx

    pop rbx
ret

pci_scan_bus_linear:
    push r12
    push r13
    push r14

    xor r12, r12

.loop_bus:
    xor r13, r13

.loop_device:
    mov rdi, r12
    mov rsi, r13
    xor rdx, rdx
    xor rcx, rcx
    call pci_read_config_dword

    cmp ax, 0xFFFF
    je .next_device

    mov rdi, r12
    mov rsi, r13
    xor rdx, rdx
    mov rcx, 0x0C
    call pci_read_config_dword

    shr eax, 16
    test al, 0x80
    jnz .multifunc

    xor r14, r14
    call .on_device_found
    jmp .next_device

.multifunc:
    xor r14, r14

.loop_function:
    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    xor rcx, rcx
    call pci_read_config_dword

    cmp ax, 0xFFFF
    je .next_function

    call .on_device_found

.next_function:
    inc r14
    cmp r14, 8
    jl .loop_function

.next_device:
    inc r13
    cmp r13, 32
    jl .loop_device

.next_bus:
    inc r12
    cmp r12, 256
    jl .loop_bus

    pop r14
    pop r13
    pop r12
    ret
.on_device_found:

    push rax
    push rbx
    push r9
    push r10
    push rdi
    push rsi
    push rdx
    push rcx
    push r8

    mov rax, [total_devices]
    mov rbx, 16
    mul rbx

    mov rbx, [pci_devices]
    add rbx, rax

    mov r9, rbx

    mov r10, [total_devices]

    inc [total_devices]

    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    xor rcx, rcx
    call pci_read_config_dword

    mov rbx, rax
    and ax, 0xFFFF
    shr ebx, 16

    mov [r9], ax ; Vendor ID
    mov [r9+2], bx; Device ID

    mov rdi, r12
    mov rsi, r13
    mov rdx, r14
    mov rcx, 0x08
    call pci_read_config_dword

    push rax

    shr eax, 24
    mov r8b, al

    mov [r9+4], r8b; Class
    mov [r9+5], r12b; Bus
    mov [r9+6], r13b; Device
    mov [r9+7], r14b; Function

    pop rax
    push rax

    shr rax, 16
    mov r8b, al

    mov [r9+8], r8b; Subclass

    pop rax
    shr eax, 8
    mov r8b, al
    mov [r9+9], r8b ; Prog IF

    pop r8
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r10
    pop r9
    pop rbx
    pop rax

ret

pci_get_bar: ; Input: RDI = Bus, RSI = Slot, RDX = Function, RCX = BAR Index (0-5)
             ; Output: RAX = addr or port. R9 = Type (0 - Mem, 1 - I/O)
    push rbx
    push rcx

    mov rax, rcx
    shl rax, 2
    add rax, 0x10
    
    mov rcx, rax            
    call pci_read_config_dword

    test eax, 1
    jnz .io_space

.memory_space:
    mov r9, 0
    
    mov ebx, eax
    and ebx, 0x06
    cmp ebx, 0x04
    je .mem64

    and eax, 0xFFFFFFF0
    jmp .done

.mem64:
    and eax, 0xFFFFFFF0
    mov rbx, rax

    pop rcx
    push rcx
    inc rcx
    
    mov rax, rcx
    shl rax, 2
    add rax, 0x10
    mov rcx, rax
    
    push rbx                
    call pci_read_config_dword
    pop rbx

    shl rax, 32
    or rax, rbx
    jmp .done

.io_space:
    mov r9, 1
    and eax, 0xFFFFFFFC

.done:
    pop rcx
    pop rbx
ret

total_devices dq 0

align 16

pci_devices_list_text:

db 0
dq pci_class_00
db 0,0,0,0,0,0,0

db 1
dq pci_class_01
db 0,0,0,0,0,0,0

db 2
dq pci_class_02
db 0,0,0,0,0,0,0

db 3
dq pci_class_03
db 0,0,0,0,0,0,0

db 4
dq pci_class_04
db 0,0,0,0,0,0,0

db 5
dq pci_class_05
db 0,0,0,0,0,0,0

db 6
dq pci_class_06
db 0,0,0,0,0,0,0

db 7
dq pci_class_07
db 0,0,0,0,0,0,0

db 8
dq pci_class_08
db 0,0,0,0,0,0,0

db 9
dq pci_class_09
db 0,0,0,0,0,0,0

db 0Ah
dq pci_class_0A
db 0,0,0,0,0,0,0

db 0Bh
dq pci_class_0B
db 0,0,0,0,0,0,0

db 0Ch
dq pci_class_0C
db 0,0,0,0,0,0,0

db 0Dh
dq pci_class_0D
db 0,0,0,0,0,0,0

db 0Eh
dq pci_class_0E
db 0,0,0,0,0,0,0

db 0Fh
dq pci_class_0F
db 0,0,0,0,0,0,0

db 10h
dq pci_class_10
db 0,0,0,0,0,0,0

db 11h
dq pci_class_11
db 0,0,0,0,0,0,0

db 12h
dq pci_class_12
db 0,0,0,0,0,0,0

db 13h
dq pci_class_13
db 0,0,0,0,0,0,0

db 0FFh
dq pci_class_FF
db 0,0,0,0,0,0,0

pci_devices_list_text_end:

pci_class_00 db 'Undefined Device',0
pci_class_01 db 'Mass Storage Controller',0
pci_class_02 db 'Network Controller',0
pci_class_03 db 'Display Controller',0
pci_class_04 db 'Multimedia Controller',0
pci_class_05 db 'Memory Controller',0
pci_class_06 db 'Bridge Device',0
pci_class_07 db 'Simple Communication Controller',0
pci_class_08 db 'Base System Peripheral',0
pci_class_09 db 'Input Device Controller',0
pci_class_0A db 'Docking Station',0
pci_class_0B db 'Processor',0
pci_class_0C db 'Serial Bus Controller',0
pci_class_0D db 'Wireless Controller',0
pci_class_0E db 'Intelligent I/O Controller',0
pci_class_0F db 'Satellite Communication Controller',0
pci_class_10 db 'Encryption Controller',0
pci_class_11 db 'Signal Processing Controller',0
pci_class_12 db 'Processing Accelerator',0
pci_class_13 db 'Non-Essential Instrumentation',0
pci_class_FF db 'Undefined Device',0