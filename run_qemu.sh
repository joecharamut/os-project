#!/usr/bin/env bash

cd cmake-build-debug || exit
qemu-system-x86_64 \
            -s \
            -no-reboot \
            -no-shutdown \
            -d guest_errors,cpu_reset \
            -serial stdio \
            -m 256M \
            -vga std \
            -soundhw pcspk \
            -drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on \
            -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
            -drive file=disk.img,format=raw
