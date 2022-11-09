#!/bin/sh
qemu-system-x86_64 \
    -machine q35 \
    -bios uefi/ovmf.fd \
    -drive if=ide,format=raw,file=build/root.iso \
    -boot order=c
