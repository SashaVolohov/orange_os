macro align length { 
    db length-1 - ($ + length-1) mod (length) dup 0
}
HEADS = 1
SPT = 7
Diskette_Size equ 1474560 
Begin:
    file "obj\boot.bin";
    file "obj\kernel.bin";
    align 512;
    align HEADS*SPT*512;

db (Diskette_Size - ($ - 0)) dup (0)