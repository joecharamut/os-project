set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER /home/joseph/opt/cross/prefix/bin/i686-elf-gcc)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_COMPILER nasm)

project(os C ASM_NASM)
cmake_minimum_required(VERSION 3.14)

set(WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wformat")
set(SUPPRESS_FLAGS "")
set(ERROR_FLAGS "-Werror=implicit-function-declaration")

set(CMAKE_C_FLAGS "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -O0 -g -std=c11 ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS} -ffreestanding -I${CMAKE_SOURCE_DIR}/bootstrap")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -T linker.ld")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT  "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(BOOTSTRAP_SOURCES
        bootstrap/std/types.h bootstrap/debug/assert.h
        bootstrap/boot/gdt.asm bootstrap/boot/gdt.c bootstrap/boot/gdt.h
        bootstrap/boot/interrupts.asm bootstrap/boot/interrupts.c bootstrap/boot/interrupts.h
        bootstrap/boot/boot.asm bootstrap/boot/boot.c bootstrap/boot/multiboot.h
        bootstrap/debug/serial.c bootstrap/debug/serial.h
        bootstrap/debug/term.c bootstrap/debug/term.h
        bootstrap/dev/pci.c bootstrap/dev/pci.h
        bootstrap/io/port.c bootstrap/io/port.h
        bootstrap/debug/debug.c bootstrap/debug/debug.h
        bootstrap/fs/ide.c bootstrap/fs/ide.h
        bootstrap/mm/kmem.c bootstrap/mm/kmem.h
        bootstrap/mm/paging.c bootstrap/mm/paging.h
        bootstrap/io/timer.c bootstrap/io/timer.h
        bootstrap/debug/panic.c bootstrap/debug/panic.h
        bootstrap/std/stdlib.c bootstrap/std/stdlib.h
        bootstrap/std/string.c bootstrap/std/string.h
        bootstrap/std/math.c bootstrap/std/math.h
        bootstrap/kernel/kernel.c bootstrap/kernel/kernel.h
        bootstrap/std/bitset.c bootstrap/std/bitset.h
        bootstrap/io/ps2.c bootstrap/io/ps2.h
        bootstrap/std/ordered_array.c bootstrap/std/ordered_array.h
        bootstrap/mm/kheap.c bootstrap/mm/kheap.h
        bootstrap/io/acpi.c bootstrap/io/acpi.h
        bootstrap/std/list.c bootstrap/std/list.h
        bootstrap/fs/ext2.c bootstrap/fs/ext2.h)

add_custom_target(linker.ld
        BYPRODUCTS linker.ld
        DEPENDS bootstrap/linker.ld.in
        VERBATIM
        COMMAND ${CMAKE_C_COMPILER} -P -CC -E -x c ${CMAKE_C_FLAGS} ${CMAKE_SOURCE_DIR}/bootstrap/linker.ld.in > linker.ld
        )

add_executable(kernel.bin ${BOOTSTRAP_SOURCES})
add_dependencies(kernel.bin linker.ld)
target_link_libraries(kernel.bin gcc)

set(GRUB_MODULES acpi biosdisk boot configfile echo elf ext2 hello help ls multiboot normal part_msdos sleep terminal vbe vga video)
add_custom_command(OUTPUT grub.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS grub_img.cfg
        VERBATIM
        COMMAND grub-mkimage -o grub.img -p \(hd0,msdos1\)/boot -c ${CMAKE_SOURCE_DIR}/grub_img.cfg --format=i386-pc ${GRUB_MODULES}
        )

set(PYTHON_SOURCES
        python/test.py
        )
list(TRANSFORM PYTHON_SOURCES PREPEND ${CMAKE_SOURCE_DIR}/)
add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${PYTHON_SOURCES}
        DEPENDS grub.img
        DEPENDS grub.cfg
        DEPENDS kernel.bin
        BYPRODUCTS ext2.img
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=8 &&
        parted -s disk.img mklabel msdos &&
        parted -s disk.img mkpart primary ext2 1MiB 100% &&
        dd conv=notrunc if=/usr/lib/grub/i386-pc/boot.img of=disk.img bs=440 count=1 &&
        dd conv=notrunc if=grub.img of=disk.img bs=512 seek=1 count=1024 &&
        dd if=/dev/zero of=ext2.img bs=1M count=7 &&
        mkfs.ext2 -L "RAGU" -M "/" -q ext2.img 7M &&
        e2cp ${CMAKE_SOURCE_DIR}/HELLO.TXT ext2.img: &&
        e2mkdir ext2.img:/boot &&
        e2cp ${CMAKE_BINARY_DIR}/kernel.bin ext2.img:/boot &&
        e2cp ${CMAKE_SOURCE_DIR}/grub.cfg ext2.img:/boot &&
        e2mkdir ext2.img:/python &&
        e2cp ${PYTHON_SOURCES} ext2.img:/python &&
        dd conv=notrunc if=ext2.img of=disk.img bs=1M seek=1 count=7
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS disk.img
        DEPENDS run_qemu.sh
        )
