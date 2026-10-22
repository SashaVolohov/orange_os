ENTRY(kernel_start)

SECTIONS
{
    . = 0xFFFFFFFF80000000; 

    .text : ALIGN(4096)
    {
        _code_start = .;
        
        *(.text)
        *(.text.*)
        
        _code_end = .;
    }

    .rodata : ALIGN(4096)
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        _rodata_end = .;
    }

    .data : ALIGN(4096)
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        _data_end = .;
    }

        .bss : ALIGN(4096)
    {
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        . = ALIGN(16);
        *(COMMON)
        _bss_end = .;
    }

    _kernel_end = .;
}
