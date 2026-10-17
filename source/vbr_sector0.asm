org 0x7C00

start:
    jmp short main
    nop

brOEMName           db 'ORANGEOS'
bpbBytesPerSector   dw 512
bpbSectorsPerCluster db 8
bpbReservedSectors  dw 32
bpbNumFATs          db 2
bpbRootEntries      dw 0        
bpbTotalSectors     dw 0        
bpbMedia            db 0xF8
bpbFATSize16        dw 0
bpbSectorsPerTrack  dw 63
bpbHeads            dw 255
bpbHiddenSectors    dd 2048
bpbTotalSectors32   dd 0
; Расширение FAT32
bpbFATSize32        dd 0
bpbExtFlags         dw 0
bpbFSVersion        dw 0
bpbRootCluster      dd 2
bpbFSInfo           dw 1
bpbBackupBoot       dw 6
times 12 db 0
bsDriveNumber       db 0x80
bsUnused            db 0
bsBootSignature     db 0x29
bsVolumeID          dd 0x12345678
bsVolumeLabel       db 'ORANGE DISK'
bsFileSystemType    db 'FAT32   '

main:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [drive_number], dl

    mov eax, [bpbHiddenSectors]
    add eax, 2
    
    mov di, 0x0500
    mov byte [di + 0], 0x10
    mov byte [di + 1], 0x00
    mov word [di + 2], 4
    mov word [di + 4], 0x3000
    mov word [di + 6], 0x0000
    mov [di + 8], eax
    mov dword [di + 12], 0

    mov si, 0x0500
    mov ah, 0x42
    int 13h
    jc .disk_error

    mov dl, [drive_number]

    jmp 0x0000:0x3000

.disk_error:
    mov si, msg_vbr_err
    mov ah, 0x0E
.loop:
    lodsb
    or al, al
    jz .halt
    int 0x10
    jmp .loop
.halt:
    jmp $

msg_vbr_err db "VBR Read Error", 13, 10, "Press CTRL+ALT+DEL to restart.", 13, 10, 0

drive_number db 0

times 510-($-$$) db 0
dw 0xAA55