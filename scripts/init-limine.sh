#!/bin/sh
cd limine
./bootstrap
./configure --prefix=$PWD/../build/bootloader/ --enable-uefi-x86-64
make
make install
cp limine.h ../build/bootloader/include/
cd ..