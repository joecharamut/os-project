cmake_minimum_required(VERSION 3.18)
project(os C ASM_NASM)

add_subdirectory(bootloader)
add_subdirectory(test64)
add_subdirectory(bootstrap)
add_subdirectory(efiloader)

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
        DEPENDS test64/test64.bin
        DEPENDS efiloader/loader.efi
        DEPENDS efiloader/unifont.sfn
        VERBATIM
        COMMAND dd if=/dev/zero of=esp.bin bs=1M count=128 &&
        mkfs.vfat -F32 -n "SYSTEM" esp.bin &&
        mmd -i esp.bin ::/EFI &&
        mmd -i esp.bin ::/EFI/BOOT &&
        mcopy -i esp.bin efiloader/loader.efi ::/EFI/BOOT/BOOTX64.EFI &&
        mmd -i esp.bin ::/QOS &&
        mcopy -i esp.bin ${CMAKE_SOURCE_DIR}/efiloader/unifont.sfn ::/QOS/unifont.sfn &&
        mcopy -i esp.bin test64/test64.bin ::/KERNEL.BIN
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

add_custom_command(OUTPUT OVMF_CODE.fd
        COMMAND cp /usr/share/ovmf/x64/OVMF_CODE.fd .
        )

add_custom_command(OUTPUT OVMF_VARS.fd
        COMMAND cp /usr/share/ovmf/x64/OVMF_VARS.fd .
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS disk.img
        DEPENDS OVMF_CODE.fd
        DEPENDS OVMF_VARS.fd
        DEPENDS run_qemu.sh
        )
