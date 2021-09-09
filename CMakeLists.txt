cmake_minimum_required(VERSION 3.18)
project(os C ASM_NASM)

add_subdirectory(bootloader)
add_subdirectory(test64)
add_subdirectory(bootstrap)

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
        mcopy -i boot_fs.bin test64/test64.bin :: &&
        mattrib -i boot_fs.bin -a -h +r +s test64.bin
        )

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS mbr.bin
        DEPENDS stage2.bin
        DEPENDS boot_fs.bin
        DEPENDS fs.bin
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=129 &&
        dd conv=notrunc if=bootloader/mbr.bin of=disk.img bs=512 count=1 &&
        dd conv=notrunc if=bootloader/stage2.bin of=disk.img bs=512 seek=1 count=1024 &&
        dd conv=notrunc if=boot_fs.bin of=disk.img bs=1M seek=1 count=64 &&
        dd conv=notrunc if=fs.bin of=disk.img bs=1M seek=65 count=64 &&
        fdisk -l disk.img
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS disk.img
        DEPENDS run_qemu.sh
        )
