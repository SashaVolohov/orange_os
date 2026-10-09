fasm source/boot.asm obj/boot.bin
fasm source/boot_cd.asm obj/boot_cd.bin
fasm source/vbr_sector0.asm obj/vbr_sector0.bin
fasm source/vbr_stage2.asm obj/vbr_stage2.bin

fasm source/boot/bootmgr.asm obj/bootmgr.bin
fasm source/setup/bootmgr.asm obj/bootmgr_setup.bin
fasm source/kernel/kernel.asm obj/kernel.o

cd source/acpica
./build_acpica.sh
cd ../..

cd source/kernel
./build_kernel.sh
cd ../..

ld -n -T linker.ld obj/kernel.o source/acpica/build_obj/*.o source/kernel/build_obj/*.o -o obj/kernel.elf --no-warn-rwx-segments

objdump -d -M intel obj/kernel.elf > full_kernel.txt

objcopy -O binary obj/kernel.elf obj/kernel.bin

mkdir -p iso_root
mkdir -p iso_root/boot

cp obj/boot_cd.bin iso_root/boot/boot_cd.bin 
cp obj/bootmgr_setup.bin iso_root/BOOTMGR
cp obj/kernel.bin iso_root/OROSKRNL.OEX

cp obj/bootmgr.bin iso_root/_BOOTMGR
cp obj/boot.bin iso_root/_BOOTMBR
cp obj/vbr_sector0.bin iso_root/_VBRSEC0
cp obj/vbr_stage2.bin iso_root/_VBRSEC1

xorriso -as mkisofs \
  -R -J \
  -V "ORANGEOS" \
  -b boot/boot_cd.bin \
  -no-emul-boot \
  -boot-load-size 4 \
  -boot-info-table \
  -o orangeos.iso \
  iso_root

rm -rf iso_root