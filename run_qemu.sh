#!/usr/bin/env bash

cd cmake-build-debug
qemu-system-i386 \
            -no-reboot \
            -no-shutdown \
            -d guest_errors \
            -d cpu_reset \
            -serial stdio \
            -m 128M \
            -vga std \
            -drive id=disk,if=ide,file=disk.img,format=raw \
            -kernel kernel.bin
