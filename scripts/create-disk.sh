#!/bin/sh

dd if=/dev/zero of=build/root.iso bs=4M count=256
echo -ne 'g\nn\n\n\n+64M\nt\n1\nw\n' | fdisk build/root.iso
./scripts/attach-root.sh
LOOPBACK_DEVICE=`losetup -a | grep build/root.iso | awk -F: '{print $1;}'`
mkdir -p mnt
mkfs.fat -F 32 $LOOPBACK_DEVICE'p1'
mount $LOOPBACK_DEVICE'p1' mnt/
mkdir -p mnt/{EFI/BOOT/,limine/}
cp bootloader/share/limine/BOOTX64.EFI mnt/EFI/BOOT/
cp limine/test/limine.cfg mnt/limine/
umount mnt/
losetup -d $LOOPBACK_DEVICE
