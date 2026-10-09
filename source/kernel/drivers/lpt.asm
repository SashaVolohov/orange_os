use64

vector_handler_39:

push rsi

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

pop rsi

iretq