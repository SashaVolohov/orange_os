MASTER_PIC_CMD  equ 0x20
MASTER_PIC_DATA equ 0x21
SLAVE_PIC_CMD   equ 0xA0
SLAVE_PIC_DATA  equ 0xA1

use64

align 8;

IDT:

dw divide_zero_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (divide_zero_fault shr 16) and 0xFFFF ;
dd divide_zero_fault shr 32              
dd 00000000h                        

dw breakpoint_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (breakpoint_fault shr 16) and 0xFFFF ;
dd breakpoint_fault shr 32              
dd 00000000h                        

dw nmi_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (nmi_fault shr 16) and 0xFFFF ;
dd nmi_fault shr 32              
dd 00000000h                        

dw status_breakpoint and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (status_breakpoint shr 16) and 0xFFFF ;
dd status_breakpoint shr 32              
dd 00000000h                        

dw overflow_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (overflow_fault shr 16) and 0xFFFF ;
dd overflow_fault shr 32              
dd 00000000h                        

dw bound_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (bound_fault shr 16) and 0xFFFF ;
dd bound_fault shr 32              
dd 00000000h                        

dw illegal_instruction_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (illegal_instruction_fault shr 16) and 0xFFFF ;
dd illegal_instruction_fault shr 32              
dd 00000000h                        

dw device_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (device_fault shr 16) and 0xFFFF ;
dd device_fault shr 32              
dd 00000000h                        

dw double_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (double_fault shr 16) and 0xFFFF ;
dd double_fault shr 32              
dd 00000000h                        

dw coprocessor_overrun_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (coprocessor_overrun_fault shr 16) and 0xFFFF ;
dd coprocessor_overrun_fault shr 32              
dd 00000000h                        

dw invalid_tss_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (invalid_tss_fault shr 16) and 0xFFFF ;
dd invalid_tss_fault shr 32              
dd 00000000h

dw segment_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (segment_fault shr 16) and 0xFFFF ;
dd segment_fault shr 32              
dd 00000000h

dw stack_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (stack_fault shr 16) and 0xFFFF ;
dd stack_fault shr 32              
dd 00000000h

dw general_protection_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (general_protection_fault shr 16) and 0xFFFF ;
dd general_protection_fault shr 32              
dd 00000000h                        

dw page_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (page_fault shr 16) and 0xFFFF ;
dd page_fault shr 32              
dd 00000000h                        

dw reserved_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (reserved_fault shr 16) and 0xFFFF ;
dd reserved_fault shr 32              
dd 00000000h                        

dw x87_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (x87_fault shr 16) and 0xFFFF ;
dd x87_fault shr 32              
dd 00000000h                        

dw aligment_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (aligment_fault shr 16) and 0xFFFF ;
dd aligment_fault shr 32              
dd 00000000h                        

dw mce_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (mce_fault shr 16) and 0xFFFF ;
dd mce_fault shr 32              
dd 00000000h                        

dw simd_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (simd_fault shr 16) and 0xFFFF ;
dd simd_fault shr 32              
dd 00000000h                        

dw virtualization_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (virtualization_fault shr 16) and 0xFFFF ;
dd virtualization_fault shr 32              
dd 00000000h                        

dw control_protection_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (control_protection_fault shr 16) and 0xFFFF ;
dd control_protection_fault shr 32              
dd 00000000h                        

dw x87_floating_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (x87_floating_fault shr 16) and 0xFFFF ;
dd x87_floating_fault shr 32              
dd 00000000h                        

dw reserved_17_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (reserved_17_fault shr 16) and 0xFFFF ;
dd reserved_17_fault shr 32              
dd 00000000h                        

dw mce_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (mce_fault shr 16) and 0xFFFF ;
dd mce_fault shr 32              
dd 00000000h                        

dw virtualization_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (virtualization_fault shr 16) and 0xFFFF ;
dd virtualization_fault shr 32              
dd 00000000h                        

dw control_protection_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (control_protection_fault shr 16) and 0xFFFF ;
dd control_protection_fault shr 32              
dd 00000000h                        

dw control_protection_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (control_protection_fault shr 16) and 0xFFFF ;
dd control_protection_fault shr 32              
dd 00000000h                        

dw hypervisor_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (hypervisor_fault shr 16) and 0xFFFF ;
dd hypervisor_fault shr 32              
dd 00000000h                        

dw vmm_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (vmm_fault shr 16) and 0xFFFF ;
dd vmm_fault shr 32              
dd 00000000h                        

dw virtualization_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (virtualization_fault shr 16) and 0xFFFF ;
dd virtualization_fault shr 32              
dd 00000000h                        

dw reserved_1F_fault and 0xFFFF          
dw 0008h                            
dw 8E01h                            
dw (reserved_1F_fault shr 16) and 0xFFFF ;
dd reserved_1F_fault shr 32              
dd 00000000h                        

dw irq0_timer_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq0_timer_handler shr 16) and 0xFFFF ;
dd irq0_timer_handler shr 32              
dd 00000000h                        

dw irq1_keyboard_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq1_keyboard_handler shr 16) and 0xFFFF ;
dd irq1_keyboard_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw irq3_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq3_handler shr 16) and 0xFFFF ;
dd irq3_handler shr 32              
dd 00000000h                        

dw irq4_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq4_handler shr 16) and 0xFFFF ;
dd irq4_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw fdc_irq_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (fdc_irq_handler shr 16) and 0xFFFF ;
dd fdc_irq_handler shr 32              
dd 00000000h                        

dw irq7_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq7_handler shr 16) and 0xFFFF ;
dd irq7_handler shr 32              
dd 00000000h                        

dw cmos_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (cmos_handler shr 16) and 0xFFFF ;
dd cmos_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw mouse_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (mouse_handler shr 16) and 0xFFFF ;
dd mouse_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw irq14_ata_primary_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq14_ata_primary_handler shr 16) and 0xFFFF ;
dd irq14_ata_primary_handler shr 32              
dd 00000000h                        

dw irq15_ata_secondary_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (irq15_ata_secondary_handler shr 16) and 0xFFFF ;
dd irq15_ata_secondary_handler shr 32              
dd 00000000h                        

; 30

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

; 40

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

; A0

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw default_handler and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (default_handler shr 16) and 0xFFFF ;
dd default_handler shr 32              
dd 00000000h                        

dw false_interrupt and 0xFFFF          
dw 0008h                            
dw 8E00h                            
dw (false_interrupt shr 16) and 0xFFFF ;
dd false_interrupt shr 32              
dd 00000000h                        

label IDT_SIZE at $-IDT

IDTR:

dw IDT_SIZE-1

dq IDT

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

pushfq
pop rax

mov [rflags_bsod], rax

mov ecx, 0C0000080h
rdmsr
mov [efer_bsod], eax

ret

clear_screen:

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

breakpoint_fault:

cli

call save_reg
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

nmi_fault:

cli

call save_reg
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

status_breakpoint:

cli

call save_reg
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

overflow_fault:

cli

call save_reg
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

bound_fault:

cli

call save_reg
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

illegal_instruction_fault:

cli

call save_reg
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

device_fault:

cli

call save_reg
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

double_fault:

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

coprocessor_overrun_fault:

cli

call save_reg
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

invalid_tss_fault:

cli

call save_reg
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

segment_fault:

cli

call save_reg
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

stack_fault:

cli

call save_reg
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

divide_zero_fault:

cli

call save_reg
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

general_protection_fault:

cli

call save_reg
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

cli

call save_reg
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

page_fault:

cli

call save_reg

pop rax
mov [page_error_code], rax

pop rax
mov [page_rip_code], rax

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

reserved_fault:

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

reserved_17_fault:

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

reserved_1F_fault:

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

x87_fault:

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

aligment_fault:

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

mce_fault:

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

simd_fault:

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

virtualization_fault:

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

control_protection_fault:

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

x87_floating_fault:

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

hypervisor_fault:

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

vmm_fault:

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

default_handler:

push rsi

    mov rsi, [lapic_base_addr]
    mov dword [rsi + 0xB0], 0

pop rsi

iretq

false_interrupt:

iretq