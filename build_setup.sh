BUILD_FILE="build_number.txt"

if [ ! -f "$BUILD_FILE" ]; then
    echo 1 > "$BUILD_FILE"
fi

BUILD_NUM=$(($(cat "$BUILD_FILE") + 1))
echo "$BUILD_NUM" > "$BUILD_FILE"
BUILD_DATE=$(date '+%Y-%m-%d %H:%M')

echo "Сборка OrangeOS (Build #$BUILD_NUM)..."

cat <<EOF > source/kernel/include/version.h
#ifndef VERSION_H
#define VERSION_H

#define ORANGEOS_BUILD $BUILD_NUM
#define ORANGEOS_BUILD_DATE "$BUILD_DATE"

#endif
EOF

fasm source/boot.asm obj/boot.bin
fasm source/boot_cd.asm obj/boot_cd.bin
fasm source/boot_usb.asm obj/boot_usb.bin
fasm source/vbr_sector0.asm obj/vbr_sector0.bin
fasm source/vbr_stage2.asm obj/vbr_stage2.bin

fasm source/boot/bootmgr.asm obj/bootmgr.bin
fasm source/setup/bootmgr.asm obj/bootmgr_setup.bin
fasm source/kernel/kernel.asm obj/kernel.o

cd source/acpica
./build_acpica.sh
cd ../..

cd source
./build_kernel.sh
cd ../

#cd source/uefi
#make
#cd ../..

cd source/apps
./build_apps.sh
cd ../..

ld -n -T linker.ld obj/kernel.o source/acpica/build_obj/*.o source/build_obj/*.o source/apps/build_obj/*.o -o obj/kernel.elf --no-warn-rwx-segments

mkdir -p iso_root
#mkdir -p iso_root/EFI
#mkdir -p iso_root/EFI/BOOT
mkdir -p iso_root/boot

cp obj/boot_cd.bin iso_root/boot/boot_cd.bin 
cp obj/bootmgr_setup.bin iso_root/BOOTMGR
cp obj/kernel.elf iso_root/OROSKRNL.ELF

cp obj/bootmgr.bin iso_root/_BOOTMGR
cp obj/boot.bin iso_root/_BOOTMBR
cp obj/vbr_sector0.bin iso_root/_VBRSEC0
cp obj/vbr_stage2.bin iso_root/_VBRSEC1

#cp source/uefi/BOOTX64.EFI iso_root/EFI/BOOT/

cp images/BACK_S.BMP iso_root/BACK_S.BMP

cp fonts/Inter.ttf iso_root/INTER.TTF

#dd if=/dev/zero of=iso_root/boot/efiboot.img bs=1k count=1440
#mformat -i iso_root/boot/efiboot.img -f 1440 ::
#mmd -i iso_root/boot/efiboot.img ::/EFI
#mmd -i iso_root/boot/efiboot.img ::/EFI/BOOT

#mcopy -i iso_root/boot/efiboot.img source/uefi/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI

#xorriso -as mkisofs -R -J -V "ORANGEOS" \
#  -b boot/boot_cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
#  -isohybrid-mbr obj/boot_usb.bin \
#  -eltorito-alt-boot \
#  -e boot/efiboot.img -no-emul-boot \
#  -isohybrid-gpt-basdat \
#  -o orangeos.iso iso_root

xorriso -as mkisofs -R -J -V "ORANGEOS" \
  -b boot/boot_cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table \
  -isohybrid-mbr obj/boot_usb.bin \
  -o orangeos.iso iso_root

rm -rf iso_root