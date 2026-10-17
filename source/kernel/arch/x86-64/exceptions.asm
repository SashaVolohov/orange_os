MASTER_PIC_CMD  equ 0x20
MASTER_PIC_DATA equ 0x21
SLAVE_PIC_CMD   equ 0xA0
SLAVE_PIC_DATA  equ 0xA1

use64

section '.data' writeable align 16

public idt_pointer
public idt_table
public KeBugMemoryManagement
public KeBugACPIError
public KeBugInaccesibleBootDevice

align 16
idt_pointer:
    dw (256 * 16) - 1
    db 0,0,0,0,0,0,0,0

align 16
idt_table:
    rb 256 * 16

section '.text' executable

align 8
handlers_jump_table:

rept 256 i:0 {
    dq vector_handler_#i
}

save_reg:

mov [rax_bsod], rax
mov [rbx_bsod], rbx
mov [rcx_bsod], rcx
mov [rdx_bsod], rdx
mov [rsp_bsod], rsp
mov [rbp_bsod], rbp
mov [rsi_bsod], rsi
mov [rdi_bsod], rdi
mov [r8_bsod], r8
mov [r9_bsod], r9
mov [r10_bsod], r10
mov [r11_bsod], r11
mov [r12_bsod], r12
mov [r13_bsod], r13
mov [r14_bsod], r14
mov [r15_bsod], r15

mov rax, 0
mov [interrupt_number], rax

pushfq
pop rax

mov [rflags_bsod], rax

mov ecx, 0C0000080h
rdmsr
mov [efer_bsod], eax

ret

clear_screen:

cmp [lfb_buffer_addr], 0
je text_bsod

mov rsi, 0
mov esi, [lfb_buffer_addr]

mov eax, 480000

.gui_screen:

mov [rsi], byte 255
mov [rsi+1], byte 0
mov [rsi+2], byte 0
mov [rsi+3], byte 255

dec eax
add rsi, 4
cmp eax, 00h
jne .gui_screen

ret

vector_handler_1:

cli

call save_reg

mov [interrupt_number], 12

call clear_screen

mov r13, bsod_01_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_2:

cli

call save_reg

mov [interrupt_number], 2

call clear_screen

mov r13, bsod_02_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_3:

cli

call save_reg

mov [interrupt_number], 3

call clear_screen

mov r13, bsod_03_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_4:

cli

call save_reg

mov [interrupt_number], 4

call clear_screen

mov r13, bsod_04_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_5:

cli

call save_reg

mov [interrupt_number], 5

call clear_screen

mov r13, bsod_05_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_6:

cli

call save_reg

mov [interrupt_number], 6

call clear_screen

mov r13, bsod_06_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_7:

cli

call save_reg

mov [interrupt_number], 7
call clear_screen

mov r13, bsod_07_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_8:

mov [interrupt_number], 8

cli

call save_reg
call clear_screen

mov r13, bsod_08_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_9:

cli

call save_reg

mov [interrupt_number], 9

call clear_screen

mov r13, bsod_09_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_10:

cli

call save_reg

mov [interrupt_number], 10

call clear_screen

mov r13, bsod_0A_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_11:

cli

call save_reg

mov [interrupt_number], 11

call clear_screen

mov r13, bsod_0B_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_12:

cli

call save_reg

mov [interrupt_number], 12

call clear_screen

mov r13, bsod_0C_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_0:

cli

call save_reg

mov [interrupt_number], 0

call clear_screen

mov r13, bsod_00_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_13:

cli

call save_reg

mov [interrupt_number], 13

call clear_screen

mov r13, bsod_0D_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

memory_management:
KeBugMemoryManagement:

cli

call save_reg

mov [interrupt_number], 32

call clear_screen

mov r13, bsod_20_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_14:

cli
call save_reg

mov [interrupt_number], 14

mov rax, [rsp + 0]
mov [page_error_code], rax

mov rax, [rsp + 8]
mov [page_rip_code], rax

mov rax, [rsp + 32]
mov [rsp_bsod], rax

call clear_screen

mov r15, [page_error_code]
mov r14, bsod_text
mov r13, 71
call format_string_with_64_int

mov r15, [page_rip_code]
mov r14, bsod_text
mov r13, 51
call format_string_with_64_int

mov r13, bsod_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_15:

cli

call save_reg
call clear_screen

mov r13, bsod_0F_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_23:

cli

call save_reg
call clear_screen

mov r13, bsod_17_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_31:

cli

call save_reg
call clear_screen

mov r13, bsod_1F_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_16:

cli

call save_reg
call clear_screen

mov r13, bsod_10_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_17:

cli

call save_reg
call clear_screen

mov r13, bsod_11_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_18:
vector_handler_24:

cli

call save_reg
call clear_screen

mov r13, bsod_12_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_19:

cli

call save_reg
call clear_screen

mov r13, bsod_13_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_20:
vector_handler_25:
vector_handler_30:

cli

call save_reg
call clear_screen

mov r13, bsod_14_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_21:
vector_handler_26:
vector_handler_27:

cli

call save_reg
call clear_screen

mov r13, bsod_15_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_22:

cli

call save_reg
call clear_screen

mov r13, bsod_16_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_28:

cli

call save_reg
call clear_screen

mov r13, bsod_1C_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

jmp draw_dump

vector_handler_29:

cli

call save_reg
call clear_screen

mov r13, bsod_1D_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text

acpi_error:
KeBugACPIError:

mov [interrupt_number], 21

cli

call save_reg
call clear_screen

mov r13, bsod_21_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text


jmp draw_dump

KeBugInaccesibleBootDevice:

mov [interrupt_number], 22

cli

call save_reg
call clear_screen

mov r13, bsod_22_text
mov r15d, 0
mov r14d, 0
mov cl, 0FFh
mov bh, 0FFh
mov bl, 0FFh
mov ch, 0FFh
call draw_text


jmp draw_dump

draw_dump:

mov r15d, [efer_bsod]
mov r14, bsod_efer
mov r13, 6
mov r12d, 0
mov r11d, 24
;call print_format_string_with_32_int

mov r15, [rax_bsod]
mov r14, bsod_rax
mov r13, 5
call format_string_with_64_int

mov r15, [rbx_bsod]
mov r14, bsod_rax
mov r13, 30
call format_string_with_64_int

mov r13, bsod_rax
mov r15d, 0
mov r14d, 48
call draw_text

mov r15, [rcx_bsod]
mov r14, bsod_rcx
mov r13, 5
call format_string_with_64_int

mov r15, [rdx_bsod]
mov r14, bsod_rcx
mov r13, 30
call format_string_with_64_int

mov r13, bsod_rcx
mov r15d, 0
mov r14d, 60
call draw_text

mov r15, [rsp_bsod]
mov r14, bsod_sp
mov r13, 5
call format_string_with_64_int

mov r15, [rbp_bsod]
mov r14, bsod_sp
mov r13, 30
call format_string_with_64_int

mov r13, bsod_sp
mov r15d, 0
mov r14d, 72
call draw_text

mov r15, [rsi_bsod]
mov r14, bsod_rsi
mov r13, 5
call format_string_with_64_int

mov r15, [rdi_bsod]
mov r14, bsod_rsi
mov r13, 30
call format_string_with_64_int

mov r13, bsod_rsi
mov r15d, 0
mov r14d, 84
call draw_text

mov r15, [r8_bsod]
mov r14, bsod_r8
mov r13, 4
call format_string_with_64_int

mov r15, [r9_bsod]
mov r14, bsod_r8
mov r13, 28
call format_string_with_64_int

mov r13, bsod_r8
mov r15d, 0
mov r14d, 96
call draw_text

mov r15, [r10_bsod]
mov r14, bsod_r10
mov r13, 5
call format_string_with_64_int

mov r15, [r11_bsod]
mov r14, bsod_r10
mov r13, 30
call format_string_with_64_int

mov r13, bsod_r10
mov r15d, 0
mov r14d, 108
call draw_text

mov r15, [r12_bsod]
mov r14, bsod_r12
mov r13, 5
call format_string_with_64_int

mov r15, [r13_bsod]
mov r14, bsod_r12
mov r13, 30
call format_string_with_64_int

mov r13, bsod_r12
mov r15d, 0
mov r14d, 120
call draw_text

mov r15, [r14_bsod]
mov r14, bsod_r14
mov r13, 5
call format_string_with_64_int

mov r15, [r15_bsod]
mov r14, bsod_r14
mov r13, 30
call format_string_with_64_int

mov r13, bsod_r14
mov r15d, 0
mov r14d, 132
call draw_text

mov r15, [rflags_bsod]
mov r14, bsod_rflags
mov r13, 8
call format_string_with_64_int

mov r13, bsod_rflags
mov r15d, 0
mov r14d, 158
call draw_text

mov rdi, 0
mov rsi, 0
mov edi, dword [lfb_mouse_addr]
mov esi, dword [lfb_buffer_addr]
mov rdx, 480000

call flip_buffers

jmp $

bsod_00_text db '*** STOP: 0x00000000 (INT_DIVIDE_BY_ZERO)',0
bsod_01_text db '*** STOP: 0x00000001 (KMODE_EXCEPTION_NOT_HANDLED)',0
bsod_02_text db '*** STOP: 0x00000002 (NMI_HARDWARE_FAILURE)',0
bsod_03_text db '*** STOP: 0x00000003 (STATUS_BREAKPOINT)',0
bsod_04_text db '*** STOP: 0x00000004 (KMODE_EXCEPTION_NOT_HANDLED)',0
bsod_05_text db '*** STOP: 0x00000005 (KMODE_EXCEPTION_NOT_HANDLED)',0
bsod_06_text db '*** STOP: 0x00000006 (STATUS_ILLEGAL_INSTRUCTION)',0
bsod_07_text db '*** STOP: 0x00000007 (KMODE_EXCEPTION_NOT_HANDLED)',0
bsod_08_text db '*** STOP: 0x00000008 (DOUBLE_FAULT)',0
bsod_09_text db '*** STOP: 0x00000009 (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_0A_text db '*** STOP: 0x0000000A (IRQL_NOT_LESS_OR_EQUAL)',0
bsod_0B_text db '*** STOP: 0x0000000B (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_0C_text db '*** STOP: 0x0000000C (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_0D_text db '*** STOP: 0x0000000D (KMODE_EXCEPTION_NOT_HANDLED)',0
bsod_text db '*** STOP: 0x0000000E (PAGE_FAULT_IN_NONPAGED_AREA, 0x0000000000000000, 0x0000000000000000)',0
bsod_0F_text db '*** STOP: 0x0000000F (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_10_text db '*** STOP: 0x00000010 (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_11_text db '*** STOP: 0x00000011 (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_12_text db '*** STOP: 0x00000012 (UNCORRECTABLE_ERROR)',0
bsod_13_text db '*** STOP: 0x00000013 (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_14_text db '*** STOP: 0x00000014 (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_15_text db '*** STOP: 0x00000015 (KERNEL_SECURITY_CHECK_FAILURE)',0
bsod_16_text db '*** STOP: 0x00000016 (INTERRUPT_EXCEPTION_NOT_HANDLED)',0
bsod_17_text db '*** STOP: 0x00000017 (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_1C_text db '*** STOP: 0x0000001C (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_1D_text db '*** STOP: 0x0000001D (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_1F_text db '*** STOP: 0x0000001F (UNEXPECTED_KERNEL_MODE_TRAP)',0
bsod_20_text db '*** STOP: 0x00000020 (MEMORY_MANAGEMENT)',0
bsod_21_text db '*** STOP: 0x00000021 (ACPI_BIOS_ERROR)',0
bsod_22_text db '*** STOP: 0x00000022 (INACCESSIBLE_BOOT_DEVICE)',0
bsod_efer db 'EFER: 0x00000000',0
bsod_rax db 'RAX: 0x0000000000000000, RBX: 0x0000000000000000',0
bsod_rcx db 'RCX: 0x0000000000000000, RDX: 0x0000000000000000',0
bsod_sp db 'RSP: 0x0000000000000000, RBP: 0x0000000000000000',0
bsod_rsi db 'RSI: 0x0000000000000000, RDI: 0x0000000000000000',0
bsod_r8 db 'R8: 0x0000000000000000, R9: 0x0000000000000000',0
bsod_r10 db 'R10: 0x0000000000000000, R11: 0x0000000000000000',0
bsod_r12 db 'R12: 0x0000000000000000, R13: 0x0000000000000000',0
bsod_r14 db 'R14: 0x0000000000000000, R15: 0x0000000000000000',0
bsod_rflags db 'RFLAGS: 0x0000000000000000',0

rax_bsod dq 0
rbx_bsod dq 0
rcx_bsod dq 0
rdx_bsod dq 0
rsp_bsod dq 0
rbp_bsod dq 0
rsi_bsod dq 0
rdi_bsod dq 0
r8_bsod dq 0
r9_bsod dq 0
r10_bsod dq 0
r11_bsod dq 0
r12_bsod dq 0
r13_bsod dq 0
r14_bsod dq 0
r15_bsod dq 0
rflags_bsod dq 0

page_error_code dq 0xFFFFFFFFFFFFFFFF
page_rip_code dq 0xFFFFFFFFFFFFFFFF

init_pic:
    mov al, 0x11
    out MASTER_PIC_CMD, al
    out SLAVE_PIC_CMD, al

    mov al, 0x20
    out MASTER_PIC_DATA, al
    mov al, 0x28
    out SLAVE_PIC_DATA, al

    mov al, 0x04
    out MASTER_PIC_DATA, al
    mov al, 0x02
    out SLAVE_PIC_DATA, al

    mov al, 0x01
    out MASTER_PIC_DATA, al
    out SLAVE_PIC_DATA, al

    mov al, 00000000b
    out MASTER_PIC_DATA, al
    mov al, 11101110b
    out SLAVE_PIC_DATA, al
ret

vector_handler_34:
vector_handler_37:
vector_handler_41:
vector_handler_42:
vector_handler_43:
vector_handler_45:
vector_handler_48:
vector_handler_49:
vector_handler_50:
vector_handler_51:
vector_handler_52:
vector_handler_53:
vector_handler_54:
vector_handler_55:
vector_handler_56:
vector_handler_57:
vector_handler_58:
vector_handler_59:
vector_handler_60:
vector_handler_61:
vector_handler_62:
vector_handler_63:
vector_handler_64:
vector_handler_65:
vector_handler_66:
vector_handler_67:
vector_handler_68:
vector_handler_69:
vector_handler_70:
vector_handler_71:
vector_handler_72:
vector_handler_73:
vector_handler_74:
vector_handler_75:
vector_handler_76:
vector_handler_77:
vector_handler_78:
vector_handler_79:
vector_handler_80:
vector_handler_81:
vector_handler_82:
vector_handler_83:
vector_handler_84:
vector_handler_85:
vector_handler_86:
vector_handler_87:
vector_handler_88:
vector_handler_89:
vector_handler_90:
vector_handler_91:
vector_handler_92:
vector_handler_93:
vector_handler_94:
vector_handler_95:
vector_handler_96:
vector_handler_97:
vector_handler_98:
vector_handler_99:
vector_handler_100:
vector_handler_101:
vector_handler_102:
vector_handler_103:
vector_handler_104:
vector_handler_105:
vector_handler_106:
vector_handler_107:
vector_handler_108:
vector_handler_109:
vector_handler_110:
vector_handler_111:
vector_handler_112:
vector_handler_113:
vector_handler_114:
vector_handler_115:
vector_handler_116:
vector_handler_117:
vector_handler_118:
vector_handler_119:
vector_handler_120:
vector_handler_121:
vector_handler_122:
vector_handler_123:
vector_handler_124:
vector_handler_125:
vector_handler_126:
vector_handler_127:
vector_handler_128:
vector_handler_129:
vector_handler_130:
vector_handler_131:
vector_handler_132:
vector_handler_133:
vector_handler_134:
vector_handler_135:
vector_handler_136:
vector_handler_137:
vector_handler_138:
vector_handler_139:
vector_handler_140:
vector_handler_141:
vector_handler_142:
vector_handler_143:
vector_handler_144:
vector_handler_145:
vector_handler_146:
vector_handler_147:
vector_handler_148:
vector_handler_149:
vector_handler_150:
vector_handler_151:
vector_handler_152:
vector_handler_153:
vector_handler_154:
vector_handler_155:
vector_handler_156:
vector_handler_157:
vector_handler_158:
vector_handler_159:
vector_handler_160:
vector_handler_161:
vector_handler_162:
vector_handler_163:
vector_handler_164:
vector_handler_165:
vector_handler_166:
vector_handler_167:
vector_handler_168:
vector_handler_169:
vector_handler_170:
vector_handler_171:
vector_handler_172:
vector_handler_173:
vector_handler_174:
vector_handler_175:
vector_handler_176:
vector_handler_177:
vector_handler_178:
vector_handler_179:
vector_handler_180:
vector_handler_181:
vector_handler_182:
vector_handler_183:
vector_handler_184:
vector_handler_185:
vector_handler_186:
vector_handler_187:
vector_handler_188:
vector_handler_189:
vector_handler_190:
vector_handler_191:
vector_handler_192:
vector_handler_193:
vector_handler_194:
vector_handler_195:
vector_handler_196:
vector_handler_197:
vector_handler_198:
vector_handler_199:
vector_handler_200:
vector_handler_201:
vector_handler_202:
vector_handler_203:
vector_handler_204:
vector_handler_205:
vector_handler_206:
vector_handler_207:
vector_handler_208:
vector_handler_209:
vector_handler_210:
vector_handler_211:
vector_handler_212:
vector_handler_213:
vector_handler_214:
vector_handler_215:
vector_handler_216:
vector_handler_217:
vector_handler_218:
vector_handler_219:
vector_handler_220:
vector_handler_221:
vector_handler_222:
vector_handler_223:
vector_handler_224:
vector_handler_225:
vector_handler_226:
vector_handler_227:
vector_handler_228:
vector_handler_229:
vector_handler_230:
vector_handler_231:
vector_handler_232:
vector_handler_233:
vector_handler_234:
vector_handler_235:
vector_handler_236:
vector_handler_237:
vector_handler_238:
vector_handler_239:
vector_handler_240:
vector_handler_241:
vector_handler_242:
vector_handler_243:
vector_handler_244:
vector_handler_245:
vector_handler_246:
vector_handler_247:
vector_handler_248:
vector_handler_249:
vector_handler_250:
vector_handler_251:
vector_handler_252:
vector_handler_253:
vector_handler_254:

push rsi

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

pop rsi

iretq

vector_handler_255:

iretq

init_dynamic_idt:
    push rbp
    mov rbp, rsp

    mov rsi, handlers_jump_table
    mov rdi, idt_table
    
    xor rdx, rdx

.idt_loop:
    mov rax, [rsi]

    mov [rdi], ax                 
    
    mov [rdi + 2], word 0x0008

    cmp rdx, 32
    jl .set_ist_1
    mov [rdi + 4], byte 0x00
    jmp .continue_flags
.set_ist_1:
    mov [rdi + 4], byte 0x01
.continue_flags:

    mov [rdi + 5], byte 0x8E      

    mov rbx, rax
    shr rbx, 16
    mov [rdi + 6], bx             

    shr rax, 32
    mov [rdi + 8], eax

    mov [rdi + 12], dword 0x00    

    add rsi, 8
    add rdi, 16
    
    inc rdx
    cmp rdx, 256
    jl .idt_loop

    mov rax, idt_table
    mov [idt_pointer + 2], rax

    lidt [idt_pointer]

    pop rbp
ret

text_bsod:

mov rdi, 0xB8000    
mov rax, 0x1F201F201F201F20
mov rcx, 500

.loop:
    mov [rdi], rax
    add rdi, 8
    dec rcx
    jnz .loop

    mov rsi, a_problem
    mov dh, 0
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, a_problem_hint
    mov dh, 5
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, a_problem_hint_2
    mov dh, 6
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, a_problem_hint_3
    mov dh, 8
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, a_problem_hint_4
    mov dh, 10
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, a_problem_hint_5
    mov dh, 11
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

    mov rsi, tech_info
    mov dh, 13
    mov dl, 0
    mov ah, 1Fh
    call print_string_64

mov r15, [rax_bsod]
mov r14, bsod_rax
mov r13, 5
call format_string_with_64_int

mov r15, [rbx_bsod]
mov r14, bsod_rax
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_rax
mov dh, 15
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [rcx_bsod]
mov r14, bsod_rcx
mov r13, 5
call format_string_with_64_int

mov r15, [rdx_bsod]
mov r14, bsod_rcx
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_rcx
mov dh, 16
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [rsp_bsod]
mov r14, bsod_sp
mov r13, 5
call format_string_with_64_int

mov r15, [rbp_bsod]
mov r14, bsod_sp
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_sp
mov dh, 17
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [rsi_bsod]
mov r14, bsod_rsi
mov r13, 5
call format_string_with_64_int

mov r15, [rdi_bsod]
mov r14, bsod_rsi
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_rsi
mov dh, 18
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [r8_bsod]
mov r14, bsod_r8
mov r13, 4
call format_string_with_64_int

mov r15, [r9_bsod]
mov r14, bsod_r8
mov r13, 28
call format_string_with_64_int

mov rsi, bsod_r8
mov dh, 19
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [r10_bsod]
mov r14, bsod_r10
mov r13, 5
call format_string_with_64_int

mov r15, [r11_bsod]
mov r14, bsod_r10
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_r10
mov dh, 20
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [r12_bsod]
mov r14, bsod_r12
mov r13, 5
call format_string_with_64_int

mov r15, [r13_bsod]
mov r14, bsod_r12
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_r12
mov dh, 21
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [r14_bsod]
mov r14, bsod_r14
mov r13, 5
call format_string_with_64_int

mov r15, [r15_bsod]
mov r14, bsod_r14
mov r13, 30
call format_string_with_64_int

mov rsi, bsod_r14
mov dh, 22
mov dl, 0
mov ah, 1Fh
call print_string_64

mov r15, [rflags_bsod]
mov r14, bsod_rflags
mov r13, 8
call format_string_with_64_int

mov rsi, bsod_rflags
mov dh, 23
mov dl, 0
mov ah, 1Fh
call print_string_64


mov rax, [interrupt_number]
cmp rax, 14
je .text_page_fault
jmp draw_text_bsod

.text_page_fault:
    mov r15, [page_error_code]
    mov r14, bsod_text
    mov r13, 71
    call format_string_with_64_int

    mov r15, [page_rip_code]
    mov r14, bsod_text
    mov r13, 51
    call format_string_with_64_int

draw_text_bsod:

mov rdi, bsod_text_0

mov rbx, 8
mov rax, [interrupt_number]
mul rbx

    add rdi, rax
    mov rsi, [rdi]

    mov dh, 3
    mov dl, 0
    mov ah, 1Fh
    call print_string_64


hlt
jmp $
interrupt_number dq 0

a_problem db 'A problem has been detected and OrangeOS has been shutdown to prevent damage to your computer.',0
a_problem_hint db "If this is first time you've seen this Stop error screen, restart your computer.",0
a_problem_hint_2 db "If this screen appears again, follow these steps:",0
a_problem_hint_3 db "Check to make sure any new hardware or software is properly installed.",0
a_problem_hint_4 db "If problems continue, disable or remove any newly installed hardware or software",0
a_problem_hint_5 db "Disable BIOS memory options such as caching and shadowing.",0

tech_info db "Technical information:",0

bsod_text_0 dq bsod_00_text
dq bsod_01_text
dq bsod_02_text
dq bsod_03_text
dq bsod_04_text
dq bsod_05_text
dq bsod_06_text
dq bsod_07_text
dq bsod_08_text
dq bsod_09_text
dq bsod_0A_text
dq bsod_0B_text
dq bsod_0C_text
dq bsod_0D_text
dq bsod_text
dq bsod_0F_text
dq bsod_10_text
dq bsod_11_text
dq bsod_12_text
dq bsod_13_text
dq bsod_14_text
dq bsod_15_text
dq bsod_16_text
dq bsod_17_text
dq bsod_17_text
dq bsod_17_text
dq bsod_17_text
dq bsod_17_text
dq bsod_1C_text
dq bsod_1D_text
dq bsod_1D_text
dq bsod_1F_text
dq bsod_20_text
dq bsod_21_text