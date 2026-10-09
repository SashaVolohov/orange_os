#!/bin/bash
set -e

INCLUDES="-I./include"

CFLAGS="-c -m64 -ffreestanding -mno-red-zone -mno-mmx -mno-sse -fno-pic -fno-stack-protector -fno-pie -O2"
CFLAGS="$CFLAGS -D_KERNEL -DACPI_LIBRARY -mcmodel=large -fno-builtin -fno-asynchronous-unwind-tables"
CFLAGS="$CFLAGS -DACPI_DEBUGGER_OFF -DACPI_DISASSEMBLER_OFF -mpreferred-stack-boundary=3"
CFLAGS="$CFLAGS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -DACPI_MACHINE_WIDTH=64 -DACPI_USE_SYSTEM_CLIB -DACPI_SINGLE_THREADED -DACPI_MISALIGNMENT_NOT_SUPPORTED -DACPI_DATA_ALIGNMENT=8 -D_OSI_MONOLITHIC"

echo "Компиляция ядра ACPICA..."

mkdir -p build_obj

for file in dispatcher/*.c events/*.c executer/*.c hardware/*.c interpreter/*.c namespace/*.c parser/*.c tables/*.c utilities/*.c; do
    if [ -f "$file" ]; then
        dir_name=$(dirname "$file")
        file_name=$(basename "$file" .c)
        
        obj_name="build_obj/${dir_name}_${file_name}.o"
        
        echo "Компиляция $file -> $obj_name..."
        gcc $CFLAGS $INCLUDES "$file" -o "$obj_name"
    fi
done

if [ -f "acpi_osl.c" ]; then
    echo "Компиляция acpi_osl.c..."
    gcc $CFLAGS $INCLUDES acpi_osl.c -o build_obj/acpi_osl.o
fi

echo "Все файлы ACPICA успешно скомпилированы в папку build_obj/"