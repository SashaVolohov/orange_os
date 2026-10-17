#!/bin/bash
set -e

INCLUDES="-I./include -I../apps/include"

CFLAGS="-c -m64 -ffreestanding -mno-red-zone -mno-mmx -mno-sse -fno-pic -fno-stack-protector -fno-pie -O2"
CFLAGS="$CFLAGS -D_KERNEL -mcmodel=large -fno-builtin -fno-asynchronous-unwind-tables"
CFLAGS="$CFLAGS -mpreferred-stack-boundary=3"
CFLAGS="$CFLAGS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D_OSI_MONOLITHIC"

echo "Компиляция ядра OrangeOS..."

mkdir -p build_obj

for file in core/*.c setup/*.c; do
    if [ -f "$file" ]; then
        dir_name=$(dirname "$file")
        file_name=$(basename "$file" .c)
        
        obj_name="build_obj/${dir_name}_${file_name}.o"
        
        echo "Компиляция $file -> $obj_name..."
        gcc $CFLAGS $INCLUDES "$file" -o "$obj_name"
    fi
done

echo "Все файлы ядра OrangeOS успешно скомпилированы в папку build_obj/"