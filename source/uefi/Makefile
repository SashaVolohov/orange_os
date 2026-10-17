CC = clang
CFLAGS = -target x86_64-unknown-windows -fuse-ld=lld -ffreestanding -fno-stack-protector -mno-red-zone -fshort-wchar -nostdlib -Wl,-subsystem:efi_application -Wl,-entry:efi_main

TARGET = BOOTX64.EFI

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
	rm -f $(TARGET)