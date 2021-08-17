#!/usr/bin/env bash

cd cmake-build-debug
qemu-system-x86_64 \
            -s \
            -no-reboot \
            -no-shutdown \
            -d guest_errors,cpu_reset \
            -serial stdio \
            -m 128M \
            -vga std \
            -drive file=disk.img,format=raw
