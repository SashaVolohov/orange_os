use64

mouse_logo:
db 2,2,0,0,0,0,0,0,0,0,0,0
db 2,1,2,0,0,0,0,0,0,0,0,0
db 2,1,1,2,0,0,0,0,0,0,0,0
db 2,1,1,1,2,0,0,0,0,0,0,0
db 2,1,1,1,1,2,0,0,0,0,0,0
db 2,1,1,1,1,1,2,0,0,0,0,0
db 2,1,1,1,1,1,1,2,0,0,0,0
db 2,1,1,1,1,1,1,1,2,0,0,0
db 2,1,1,1,1,1,1,1,1,2,0,0
db 2,1,1,1,1,1,1,1,1,1,2,0
db 2,1,1,1,1,1,2,2,2,2,2,2
db 2,1,1,2,1,1,2,0,0,0,0,0
db 2,1,2,0,2,1,1,2,0,0,0,0
db 2,2,0,0,2,1,1,2,0,0,0,0
db 0,0,0,0,0,2,1,1,2,0,0,0
db 0,0,0,0,0,2,1,1,2,0,0,0
db 0,0,0,0,0,0,2,1,1,2,0,0
db 0,0,0,0,0,0,2,1,1,2,0,0
db 0,0,0,0,0,0,0,2,2,0,0,0

mouse_background:
times 864 db 0

ps2_wait_read:

push rax

.wait:

in al, 0x64
test al, 0x01

jz .wait

pop rax

ret

ps2_wait_write:

push rax

.wait:

in al, 0x64
test al, 0x02

jnz .wait

pop rax

ret

ps2_write_command: ; al - command

call ps2_wait_write
out 0x64, al

ret

mouse_write: ; bl - command

push rax

mov al, 0xD4
call ps2_write_command

call ps2_wait_write

mov al, bl
out 0x60, al

pop rax

ret

mouse_read: ; Output - AL 

call ps2_wait_read

in al, 0x60

ret

mouse_init:

mov al, 0xA8
call ps2_write_command

mov al, 0x20
call ps2_write_command

call ps2_wait_read

in al, 0x60
mov bl, al

or bl, 2
and bl, 0xDF

mov al, 0x60
call ps2_write_command

call ps2_wait_write

mov al, bl
out 0x60, al

mov bl, 0xF6
call mouse_write

call mouse_read

mov bl, 0xF4
call mouse_write

call mouse_read

ret

mouse_handler:
    ret

mouse_packet db 0,0,0
mouse_cycle db 0
mouse_buttons db 0,0,0
mouse_x db 0
mouse_y db 0
global_mouse_x dd 400
global_mouse_y dd 300
old_mouse_x dd 1000
old_mouse_y dd 1000