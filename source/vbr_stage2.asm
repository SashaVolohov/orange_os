org 0x3000

vbr_stage2:

    mov [boot_drive], dl

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov eax, [0x7C00 + 36]
    movzx edx, byte [0x7C00 + 16]
    mul edx
    movzx ecx, word [0x7C00 + 14]
    add eax, ecx
    add eax, [0x7C00 + 28]
    mov [data_lba_start], eax

    mov eax, [0x7C00 + 28]
    movzx ecx, word [0x7C00 + 14]
    add eax, ecx
    mov [fat_lba_start], eax

    mov eax, [0x7C2C]

.next_root_cluster:
    push eax
    call cluster_to_lba
    
    mov bx, 0x9000
    mov cx, 1
    call read_cluster

    movzx cx, byte [0x7C0D]
    shl cx, 4
    mov si, 0x9000

.search_loop:
    push cx
    mov di, filename
    mov cx, 11
    push si
    repe cmpsb
    pop si
    pop cx
    je .found_file

    cmp byte [si], 0x00
    je .mgr_missing_pop

    add si, 32
    loop .search_loop
    pop eax
    call get_next_cluster
    cmp edx, 0x0FFFFFF8
    jae msg_no_mgr
    mov eax, edx
    jmp .next_root_cluster

.mgr_missing_pop:
    pop eax

.print_missing:
    mov si, msg_no_mgr
    call print_string
    hlt
    jmp $

.found_file:
    pop eax
    
    mov dx, [si + 20]
    mov ax, [si + 26]
    shl edx, 16
    mov dx, ax

    mov bx, 0x8000
    xor cx, cx
    mov es, cx

.load_file_loop:
    push edx

    mov eax, edx
    call cluster_to_lba
    mov cx, 1
    call read_cluster

    movzx ax, byte [0x7C0D]
    shl ax, 5
    mov cx, es
    add cx, ax
    mov es, cx

    pop edx

    call get_next_cluster
    cmp edx, 0x0FFFFFF8
    jb .load_file_loop

    mov dl, [boot_drive]

    jmp 0x0000:0x8000

cluster_to_lba:
    sub eax, 2 
    movzx ecx, byte [0x7C00 + 13] ; bpbSectorsPerCluster
    mul ecx 
    add eax, [data_lba_start] 
    ret

read_cluster:
    push eax
    push cx
    mov di, 0x0500
    mov byte [di + 0], 0x10
    mov byte [di + 1], 0x00
    
    movzx ax, byte [0x7C0D]
    mul cl
    mov [di + 2], ax
    
    mov [di + 4], bx
    mov [di + 6], es
    pop cx
    pop eax
    mov [di + 8], eax
    mov dword [di + 12], 0

    push si
    mov si, 0x0500
    mov ah, 0x42
    mov dl, [boot_drive]        
    int 13h
    pop si
    jc .read_err
    ret
.read_err:
    mov si, msg_vbr_err
    call print_string
    hlt
    jmp $                      

get_next_cluster:
    mov eax, edx
    shl eax, 2
    
    xor edx, edx
    movzx ecx, word [0x7C0B]
    div ecx

    add eax, [fat_lba_start]    

    push edx
    mov di, 0x0500
    mov byte [di + 0], 0x10
    mov byte [di + 1], 0x00
    mov word [di + 2], 1        
    mov word [di + 4], 0x5000   
    mov word [di + 6], 0x0000
    mov [di + 8], eax
    mov dword [di + 12], 0
    
    mov si, 0x0500
    mov ah, 0x42
    mov dl, [boot_drive]
    int 13h
    pop edx
    jc .fat_err

    mov si, 0x5000
    add si, dx                  
    mov edx, [si]               
    and edx, 0x0FFFFFFF
    ret
.fat_err:
    mov si, msg_vbr_err
    call print_string
    hlt
    jmp $

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

fat_lba_start  dd 0
data_lba_start dd 0
boot_drive     db 0
filename       db 'BOOTMGR    '
msg_no_mgr     db "BOOTMGR is missing.", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0
msg_vbr_err    db "Disk error occured", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0

times 2048-($-$$) db 0