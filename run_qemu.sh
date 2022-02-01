#!/usr/bin/env bash

ARGS=(
  -s                        # enable gdb connection on 127.0.0.1:1234
  -enable-kvm               # enable kernel virtual machine support
  -no-shutdown              # pause on shutdown
  -no-reboot                # pause on reboot
  -d cpu_reset              # dump regs on reset
  -d int                    # interrupts
  -d mmu                    # mmu operations
  -d pcall                  # protected mode calls
  -serial file:/dev/stdout  # redirect serial to stdout
  -m 128M                   # 128mb ram
  -vga std                  # standard vga card
  -net none                 # no network yet
  -audiodev pa,id=pa1,server=/run/user/$UID/pulse/native            # pulseaudio support
  -drive if=pflash,format=raw,unit=0,readonly=on,file=OVMF_CODE.fd  # UEFI firmware
  -drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd              # UEFI variables
  -drive format=raw,file=disk.img                                   # hard drive
)

echo "Running QEMU: qemu-system-x86_64 ${ARGS[@]}"
qemu-system-x86_64 ${ARGS[@]}
