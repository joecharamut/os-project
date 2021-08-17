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
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT  "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(BOOTSTRAP_SOURCES
        bootstrap/std/types.h bootstrap/debug/assert.h bootstrap/std/registers.h
        bootstrap/boot/gdt.asm bootstrap/boot/gdt.c bootstrap/boot/gdt.h
        bootstrap/boot/interrupts.asm bootstrap/boot/interrupts.c bootstrap/boot/interrupts.h
        bootstrap/boot/boot.asm bootstrap/boot/boot.c bootstrap/boot/multiboot.h
        bootstrap/debug/serial.c bootstrap/debug/serial.h
        bootstrap/debug/term.c bootstrap/debug/term.h
        bootstrap/io/pci.c bootstrap/io/pci.h
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
target_link_options(kernel.bin PRIVATE -T "${CMAKE_BINARY_DIR}/linker.ld")

add_custom_command(TARGET kernel.bin POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS kernel.bin
        VERBATIM
        COMMAND objdump -M intel -d kernel.bin > kernel.asm
        )

set(IMAGE_PYTHON_SOURCES
        image/python/test.py)
set(IMAGE_BOOT_SOURCES
        image/boot/grub.cfg
        image/boot/config.ini)
list(TRANSFORM IMAGE_PYTHON_SOURCES PREPEND ${CMAKE_SOURCE_DIR}/)
list(TRANSFORM IMAGE_BOOT_SOURCES PREPEND ${CMAKE_SOURCE_DIR}/)

add_custom_command(OUTPUT mbr.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS bootloader/mbr.asm
        VERBATIM
        COMMAND ${CMAKE_ASM_NASM_COMPILER} ${INCLUDES} ${FLAGS} -f bin -o mbr.bin ${CMAKE_SOURCE_DIR}/bootloader/mbr.asm
        )

set(STAGE2_SOURCES
        bootloader/stage2.c
        bootloader/cpuid.h bootloader/cpuid.asm
        bootloader/print.c bootloader/print.h)
add_executable(stage2.bin ${STAGE2_SOURCES})
target_compile_options(stage2.bin PRIVATE $<$<COMPILE_LANGUAGE:C>:-m16>)
target_link_options(stage2.bin PRIVATE -T "${CMAKE_SOURCE_DIR}/bootloader/linker.ld")

add_custom_command(OUTPUT fs.bin
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${IMAGE_PYTHON_SOURCES}
        DEPENDS ${IMAGE_BOOT_SOURCES}
        DEPENDS kernel.bin
        COMMAND dd if=/dev/zero of=fs.bin bs=1M count=1 &&
        mkfs.ext2 -L "RAGU" -M "/" -q fs.bin 7M &&
        e2mkdir fs.bin:/boot &&
        e2cp ${CMAKE_BINARY_DIR}/kernel.bin fs.bin:/boot &&
        e2cp ${IMAGE_BOOT_SOURCES} fs.bin:/boot &&
        e2mkdir fs.bin:/python &&
        e2cp ${IMAGE_PYTHON_SOURCES} fs.bin:/python
        )

add_custom_command(OUTPUT disk.img
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS mbr.bin
        DEPENDS fs.bin
        DEPENDS stage2.bin
        VERBATIM
        COMMAND dd if=/dev/zero of=disk.img bs=1M count=8 &&
        dd conv=notrunc if=mbr.bin of=disk.img bs=512 count=1 &&
        dd conv=notrunc if=stage2.bin of=disk.img bs=512 seek=1 count=1024 &&
        dd conv=notrunc if=fs.bin of=disk.img bs=1M seek=1 count=1 &&
        fdisk -l disk.img
        )

add_custom_target(qemu
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS disk.img
        DEPENDS run_qemu.sh
        )
