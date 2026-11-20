#!/bin/bash
set -e

INCLUDES="-I./kernel/include -I./apps/include -I./lwip/src/include -I./BearSSL/inc -I./BearSSL/src"

BASE_CFLAGS="-c -m64 -ffreestanding -Wall -Wextra -Werror -mno-red-zone -fno-pic -fno-stack-protector -fno-pie -O2"
BASE_CFLAGS="$BASE_CFLAGS -D_KERNEL -mcmodel=kernel -fno-builtin -fno-common -fno-asynchronous-unwind-tables -fno-strict-aliasing -nostdlib"
BASE_CFLAGS="$BASE_CFLAGS -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D_OSI_MONOLITHIC"

KERNEL_CFLAGS="$BASE_CFLAGS -mno-mmx -mno-sse -mpreferred-stack-boundary=4"

SSE_CFLAGS="$BASE_CFLAGS -msse -msse2 -mmmx -mpreferred-stack-boundary=4"

echo "Компиляция ядра OrangeOS..."

mkdir -p build_obj

for file in kernel/core/*.c kernel/setup/*.c lwip/src/core/*.c lwip/src/core/ipv4/*.c lwip/src/netif/ethernet.c BearSSL/src/*.c BearSSL/src/aead/*.c BearSSL/src/x509/*.c BearSSL/src/codec/*.c BearSSL/src/ec/*.c BearSSL/src/hash/*.c BearSSL/src/int/*.c BearSSL/src/kdf/*.c BearSSL/src/mac/*.c BearSSL/src/rand/*.c BearSSL/src/rsa/*.c BearSSL/src/ssl/*.c BearSSL/src/symcipher/*.c; do
    if [ -f "$file" ]; then
        dir_name=$(dirname "$file")
        file_name=$(basename "$file" .c)
        
        safe_dir_name=$(echo "$dir_name" | tr '/' '_')
        obj_name="build_obj/${safe_dir_name}_${file_name}.o"
        
        if [[ "$file_name" == "font" ]]; then
            CURRENT_CFLAGS="$SSE_CFLAGS"
            echo "Компиляция (с SSE/FPU) $file -> $obj_name..."
        else
            CURRENT_CFLAGS="$KERNEL_CFLAGS"
            echo "Компиляция $file -> $obj_name..."
        fi
        
        gcc $CURRENT_CFLAGS $INCLUDES "$file" -o "$obj_name"
    fi
done

echo "Все файлы ядра OrangeOS успешно скомпилированы в папку build_obj/"