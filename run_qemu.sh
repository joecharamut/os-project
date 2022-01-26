#!/usr/bin/env bash

qemu-system-x86_64 \
            -s \
            -enable-kvm \
            -no-shutdown \
            -no-reboot \
            -d cpu_reset,int,mmu,pcall \
            -serial file:/dev/stdout \
            -m 2G \
            -vga std \
            -net none \
            -audiodev id=pa,driver=pa \
            -drive if=pflash,format=raw,unit=0,readonly=on,file=OVMF_CODE.fd \
            -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
            -drive file=disk.img,format=raw
