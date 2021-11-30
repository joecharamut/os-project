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

add_custom_command(OUTPUT boot_fs.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS test64/test64.bin
        VERBATIM
        COMMAND dd if=/dev/zero of=boot_fs.bin bs=1M count=64 &&
        mkfs.vfat -F32 -i BEEFCAFE -n "SYSTEM" boot_fs.bin &&
        mmd -i boot_fs.bin ::/dir1 &&
        mcopy -i boot_fs.bin test64/test64.bin :: &&
        mattrib -i boot_fs.bin -a -h +r +s test64.bin
        )

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS test64/test64.bin
        DEPENDS efiloader/loader.efi
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=512 &&
        parted -s disk.img
            mklabel gpt
            mkpart primary fat32 1MiB 129MiB
            set 1 esp on
            print; &&
        sudo -A -- losetup --partscan /dev/loop99 disk.img &&
        sudo -A -- mkfs.vfat -F32 -n "EFI" /dev/loop99p1 &&
        mkdir -p mnt &&
        sudo -A -- mount -o defaults,umask=000 /dev/loop99p1 mnt &&
        mkdir -p mnt/EFI/BOOT &&
        cp efiloader/loader.efi mnt/EFI/BOOT/BOOTX64.EFI &&
        cp test64/test64.bin mnt/KERNEL.BIN &&
        sudo -A -- umount mnt &&
        sudo -A -- losetup -D /dev/loop99 &&
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
