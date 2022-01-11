#!/usr/bin/env bash

qemu-system-x86_64 \
            -s \
            -enable-kvm \
            -no-reboot \
            -no-shutdown \
            -d guest_errors,cpu_reset \
            -serial stdio \
            -m 128M \
            -vga std \
            -net none \
            -audiodev id=pa,driver=pa \
            -bios OVMF.fd \
            -drive file=disk.img,format=raw
