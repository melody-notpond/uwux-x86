#!/bin/sh

make -C kernel/
./scripts/attach-root.sh
LOOPBACK_DEVICE=`losetup -a | grep build/root.iso | awk -F: '{print $1;}'`
mkdir -p mnt
mount $LOOPBACK_DEVICE'p1' mnt/
mkdir -p mnt/{EFI/uwux/,limine/}
cp kernel/uwux.elf mnt/EFI/uwux/
cp limine.cfg mnt/limine/
umount mnt/
losetup -d $LOOPBACK_DEVICE
