use64

align 16;

main_unicode_system_font:

; Codepoint, addr, distanceX, distanceY;

dd 0x420
dq symbol_420
dw 7
dw 0;

main_unicode_system_font_end:

symbol_420:

; Р
db 1, 1, 1, 1, 1, 0, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 1, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 1, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 1, 0, 0, 0, 0;
db 1, 1, 1, 1, 1, 0, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 0, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 0, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 0, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 0, 0, 0, 0, 0;
db 1, 0, 0, 0, 0, 0, 0, 0, 0, 0;