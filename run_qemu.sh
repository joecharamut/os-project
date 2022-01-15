#!/usr/bin/env bash

qemu-system-x86_64 \
            -s \
            -enable-kvm \
            -no-shutdown \
            -d cpu_reset \
            -serial stdio \
            -m 128M \
            -vga std \
            -net none \
            -audiodev id=pa,driver=pa \
            -drive if=pflash,format=raw,unit=0,readonly=on,file=OVMF_CODE.fd \
            -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
            -drive file=disk.img,format=raw
