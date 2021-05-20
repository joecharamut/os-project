#!/usr/bin/env bash

cd cmake-build-debug
qemu-system-i386 \
            -s -S \
            -no-reboot \
            -no-shutdown \
            -d guest_errors,cpu_reset \
            -serial stdio \
            -m 128M \
            -vga std \
            -drive file=disk.img,format=raw
