cmake_minimum_required(VERSION 3.18)
project(os)

add_subdirectory(bootloader)
add_subdirectory(bootstrap)
add_subdirectory(efiloader)
add_subdirectory(kernel2)
add_subdirectory(rkernel)

add_custom_command(OUTPUT fs.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS kernel.bin
        COMMAND dd if=/dev/zero of=fs.bin bs=1M count=64 &&
        mkfs.ext2 -L "RAGU" -M "/" -q fs.bin &&
        e2mkdir fs.bin:/boot &&
        e2cp ${CMAKE_BINARY_DIR}/kernel.bin fs.bin:/boot
        )

add_custom_command(OUTPUT esp.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS kernel2/kernel2.bin
        DEPENDS rkernel/rkernel.bin
        DEPENDS efiloader/loader.efi
        DEPENDS efiloader/unifont.sfn
        VERBATIM
        COMMAND dd if=/dev/zero of=esp.bin bs=1M count=128 &&
        mkfs.vfat -F32 -n "SYSTEM" esp.bin &&
        mmd -i esp.bin ::/QOS &&
        mmd -i esp.bin ::/EFI &&
        mmd -i esp.bin ::/EFI/BOOT &&
        mcopy -i esp.bin efiloader/loader.efi ::/EFI/BOOT/BOOTX64.EFI &&
        mcopy -i esp.bin ${CMAKE_SOURCE_DIR}/efiloader/unifont.sfn ::/QOS/FONT.SFN &&
        mcopy -i esp.bin kernel2/kernel2.bin ::/KERNEL2.BIN &&
        mcopy -i esp.bin rkernel/rkernel.bin ::/RKERNEL.BIN
        )

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS esp.bin
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=512 &&
        parted -s disk.img -a minimal
            unit MiB
            mklabel gpt
            mkpart ESP fat32 1MiB 129MiB
            set 1 esp on
            print &&
        dd if=esp.bin of=disk.img bs=1M seek=1 count=128 conv=notrunc &&
        fdisk -l disk.img
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
