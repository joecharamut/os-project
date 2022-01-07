#!/usr/bin/env bash

cd cmake-build-debug || exit
qemu-system-x86_64 \
            -s \
            -no-reboot \
            -no-shutdown \
            -d guest_errors,cpu_reset \
            -serial stdio \
            -debugcon file:debug.log \
            -global isa-debugcon.iobase=0x402 \
            -m 1G \
            -vga std \
            -soundhw pcspk \
            -bios OVMF.fd \
            -drive file=disk.img,format=raw

            #-drive if=pflash,format=raw,unit=0,file=OVMF_CODE.fd,readonly=on \
            #-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \