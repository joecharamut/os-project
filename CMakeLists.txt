set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER i686-elf-gcc)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_COMPILER nasm)

project(os C ASM_NASM)
cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_FLAGS "-O0 -g -std=gnu11 -Wall -Wextra -pedantic -ffreestanding -I${CMAKE_SOURCE_DIR}/bootstrap")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -T ${CMAKE_SOURCE_DIR}/bootstrap/linker.ld")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT  "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(BOOTSTRAP_SOURCES
        bootstrap/std/types.h
        bootstrap/boot/gdt.asm bootstrap/boot/gdt.c bootstrap/boot/gdt.h
        bootstrap/boot/interrupts.asm bootstrap/boot/interrupts.c bootstrap/boot/interrupts.h
        bootstrap/boot/boot.asm bootstrap/boot/boot.c
        bootstrap/debug/serial.c bootstrap/debug/serial.h
        bootstrap/debug/term.c bootstrap/debug/term.h
        bootstrap/dev/pci.c bootstrap/dev/pci.h
        bootstrap/io/port.c bootstrap/io/port.h
        bootstrap/debug/debug.c bootstrap/debug/debug.h
        bootstrap/dev/ide.c bootstrap/dev/ide.h
        bootstrap/mm/kmem.c bootstrap/mm/kmem.h
        bootstrap/mm/paging.c bootstrap/mm/paging.h bootstrap/io/timer.c bootstrap/io/timer.h bootstrap/debug/panic.c bootstrap/debug/panic.h bootstrap/std/stdio.c bootstrap/std/stdio.h)

add_executable(kernel.bin ${BOOTSTRAP_SOURCES})
target_link_libraries(kernel.bin gcc)

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=4 &&
                sh -c "(echo o; echo n; echo p; echo 1; echo 2048; echo; echo t; echo 0b; echo w) | fdisk disk.img" &&
                mkfs.vfat -F32 --offset=1 disk.img &&
                mcopy -i disk.img@@1S ${CMAKE_SOURCE_DIR}/HELLO.TXT ::
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS disk.img
        DEPENDS kernel.bin
        )
