cmake_minimum_required(VERSION 3.18)
project(os)

add_subdirectory(bootloader)
add_subdirectory(bootstrap)
add_subdirectory(efiloader)
add_subdirectory(rkernel)

add_custom_command(OUTPUT fs.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND dd if=/dev/zero of=fs.bin bs=1M count=64 &&
        mkfs.ext2 -L "RAGU" -M "/" -q fs.bin &&
        e2mkdir fs.bin:/boot &&
        e2cp ${CMAKE_BINARY_DIR}/Makefile fs.bin:/boot
        )

add_custom_command(OUTPUT esp.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS efiloader/stub.efi
        DEPENDS efiloader/loader.efi
        DEPENDS common/unifont.sfn
        DEPENDS rkernel/rkernel.bin
        DEPENDS shellx64.efi
        VERBATIM
        COMMAND dd if=/dev/zero of=esp.bin bs=1M count=128 &&
        mkfs.vfat -F32 -n "SYSTEM" esp.bin &&
        mmd -i esp.bin ::/qOS &&
        mmd -i esp.bin ::/EFI &&
        mmd -i esp.bin ::/EFI/Boot &&
        mcopy -i esp.bin shellx64.efi ::/shellx64.efi &&
        mcopy -i esp.bin efiloader/stub.efi ::/EFI/Boot/bootx64.efi &&
        mcopy -i esp.bin efiloader/loader.efi ::/qOS/loader.efi &&
        mcopy -i esp.bin ${CMAKE_SOURCE_DIR}/common/unifont.sfn ::/qOS/font.sfn &&
        mcopy -i esp.bin rkernel/rkernel.bin ::/RKERNEL.BIN
        )

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS esp.bin
        DEPENDS fs.bin
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=512 &&
        parted -s disk.img -a minimal
            unit MiB
            mklabel gpt
            mkpart ESP fat32 1MiB 129MiB # Part 1 - EFI system partition - 128MiB
            set 1 esp on
            mkpart root ext2 129MiB 193MiB # Part 2 - root - 64MiB
            print &&
        dd if=esp.bin of=disk.img bs=1M seek=1 count=128 conv=notrunc &&
        dd if=fs.bin of=disk.img bs=1M seek=129 count=64 conv=notrunc &&
        echo
        )

find_file(OVMF_CODE OVMF_CODE.fd
        PATHS /usr/share
        PATH_SUFFIXES ovmf/x64
        REQUIRED
        )
add_custom_command(OUTPUT OVMF_CODE.fd
        COMMAND cp ${OVMF_CODE} .
        )
find_file(OVMF_VARS OVMF_VARS.fd
        PATHS /usr/share
        PATH_SUFFIXES ovmf/x64
        REQUIRED
        )
add_custom_command(OUTPUT OVMF_VARS.fd
        COMMAND cp ${OVMF_VARS} .
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS OVMF_CODE.fd
        DEPENDS OVMF_VARS.fd
        DEPENDS disk.img
        DEPENDS run_qemu.sh
        )
