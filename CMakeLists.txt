set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER /home/joseph/opt/cross/prefix/bin/i686-elf-gcc)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_COMPILER nasm)

project(os C ASM_NASM)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_FLAGS "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -O0 -g -std=gnu11 -Wall -Wextra -pedantic -ffreestanding -I${CMAKE_SOURCE_DIR}/bootstrap")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -T linker.ld")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT  "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(BOOTSTRAP_SOURCES
        bootstrap/std/types.h bootstrap/std/attributes.h bootstrap/std/attributes.h
        bootstrap/boot/gdt.asm bootstrap/boot/gdt.c bootstrap/boot/gdt.h
        bootstrap/boot/interrupts.asm bootstrap/boot/interrupts.c bootstrap/boot/interrupts.h
        bootstrap/boot/boot.asm bootstrap/boot/boot.c bootstrap/boot/multiboot.h
        bootstrap/debug/serial.c bootstrap/debug/serial.h
        bootstrap/debug/term.c bootstrap/debug/term.h
        bootstrap/dev/pci.c bootstrap/dev/pci.h
        bootstrap/io/port.c bootstrap/io/port.h
        bootstrap/debug/debug.c bootstrap/debug/debug.h
        bootstrap/dev/ide.c bootstrap/dev/ide.h
        bootstrap/mm/kmem.c bootstrap/mm/kmem.h
        bootstrap/mm/paging.c bootstrap/mm/paging.h
        bootstrap/io/timer.c bootstrap/io/timer.h
        bootstrap/debug/panic.c bootstrap/debug/panic.h
        bootstrap/std/stdlib.c bootstrap/std/stdlib.h
        bootstrap/std/string.c bootstrap/std/string.h
        bootstrap/std/math.c bootstrap/std/math.h
        bootstrap/kernel/kernel.c bootstrap/kernel/kernel.h
        bootstrap/std/bitset.c bootstrap/std/bitset.h
        bootstrap/io/ps2.c bootstrap/io/ps2.h)

add_custom_target(linker.ld
        BYPRODUCTS linker.ld
        DEPENDS bootstrap/linker.ld.in
        VERBATIM
        COMMAND ${CMAKE_C_COMPILER} -P -CC -E -x c ${CMAKE_C_FLAGS} ${CMAKE_SOURCE_DIR}/bootstrap/linker.ld.in > linker.ld
        )

add_executable(kernel.bin ${BOOTSTRAP_SOURCES})
add_dependencies(kernel.bin linker.ld)
target_link_libraries(kernel.bin gcc)

set(GRUB_MODULES acpi biosdisk boot cat configfile cpuid datetime drivemap echo elf ext2 fat fshelp halt hdparm hello help loadenv ls msdospart multiboot normal part_msdos pci probe read reboot scsi search serial sleep terminal usb vbe vga video)
add_custom_command(OUTPUT grub.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS grub.cfg
        VERBATIM
        COMMAND grub-mkimage -o grub.img -p \(hd0,msdos1\)/boot -c ${CMAKE_SOURCE_DIR}/grub.cfg --format=i386-pc ${GRUB_MODULES}
        )

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS HELLO.TXT
        DEPENDS grub.img
        DEPENDS grub.cfg
        DEPENDS kernel.bin
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=8 &&
        parted disk.img mklabel msdos &&
        parted disk.img mkpart primary fat32 1MiB 100% &&
        dd conv=notrunc if=/usr/lib/grub/i386-pc/boot.img of=disk.img bs=440 count=1 &&
        dd conv=notrunc if=grub.img of=disk.img bs=512 seek=1 count=1024 &&
        mkfs.vfat --offset=2048 -F32 disk.img &&
        mmd -i disk.img@@2048S ::/boot &&
        mcopy -i disk.img@@2048S ${CMAKE_BINARY_DIR}/kernel.bin ::/boot &&
        mcopy -i disk.img@@2048S ${CMAKE_SOURCE_DIR}/grub.cfg ::/boot &&
        mcopy -i disk.img@@2048S ${CMAKE_SOURCE_DIR}/HELLO.TXT ::
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS disk.img
        DEPENDS run_qemu.sh
        )
