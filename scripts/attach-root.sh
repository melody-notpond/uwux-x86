#!/bin/sh

losetup -f build/root.iso
LOOPBACK_DEVICE=`losetup -a | grep build/root.iso | awk -F: '{print $1;}'`
partprobe $LOOPBACK_DEVICE
ls $LOOPBACK_DEVICE*
