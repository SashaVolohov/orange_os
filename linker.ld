OUTPUT_FORMAT("elf64-x86-64")
ENTRY(kernel_start)

SECTIONS
{
    . = 0xFFFF900000000000; 

    .text : AT(ADDR(.text))
    {
        obj/kernel.o(.text.header)
        obj/kernel.o(.text)
        
        *(.text)
        *(.rodata*)
        *(.data)
        *(.bss)
    }
}