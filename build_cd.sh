fasm source/boot_cd.asm obj/boot_cd.bin
fasm source/boot/bootmgr.asm obj/bootmgr.bin
fasm source/kernel/kernel.asm obj/kernel.o

cd source/acpica
./build_acpica.sh
cd ../..

cd source/kernel
./build_kernel.sh
cd ../..

ld -n -T linker.ld obj/kernel.o source/acpica/build_obj/*.o source/kernel/build_obj/*.o -o obj/kernel.elf --no-warn-rwx-segments
objcopy -O binary obj/kernel.elf obj/kernel.bin

mkdir -p iso_root
mkdir -p iso_root/boot

cp obj/boot_cd.bin iso_root/boot/boot_cd.bin 
cp obj/bootmgr.bin iso_root/BOOTMGR
cp obj/kernel.bin iso_root/OROSKRNL.OEX
cp concord.pcm iso_root/CONCORD.PCM

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