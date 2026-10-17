ENTRY(efi_main)

SECTIONS
{
    . = 0x00400000;

    .text :
    {
        *(.text)
        *(.text.*)
    }

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
        *(.data.*)
    }

    .bss :
    {
        *(.bss)
        *(.common)
    }

    .reloc :
    {
        *(.reloc)
    }
}