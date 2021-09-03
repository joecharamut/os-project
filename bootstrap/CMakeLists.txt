cmake_minimum_required(VERSION 3.18)

find_program(I686_COMPILER i686-elf-gcc REQUIRED)
find_program(ASM_COMPILER nasm REQUIRED)

message(STATUS "i686 compiler is ${I686_COMPILER}")
message(STATUS "ASM compiler is ${ASM_COMPILER}")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${I686_COMPILER})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_COMPILER ${ASM_COMPILER})

set(WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wformat")
set(SUPPRESS_FLAGS "")
set(ERROR_FLAGS "-Werror=implicit-function-declaration")

set(CMAKE_C_FLAGS "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -O0 -g -std=c11 ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS} -ffreestanding -I${CMAKE_SOURCE_DIR}/bootstrap")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(BOOTSTRAP_SOURCES
        std/types.h debug/assert.h std/registers.h
        boot/gdt.asm boot/gdt.c boot/gdt.h
        boot/interrupts.asm boot/interrupts.c boot/interrupts.h
        boot/boot.asm boot/boot.c boot/multiboot.h
        debug/serial.c debug/serial.h
        debug/term.c debug/term.h
        io/pci.c io/pci.h
        io/port.c io/port.h
        debug/debug.c debug/debug.h
        fs/ide.c fs/ide.h
        mm/kmem.c mm/kmem.h
        mm/paging.c mm/paging.h
        io/timer.c io/timer.h
        debug/panic.c debug/panic.h
        std/stdlib.c std/stdlib.h
        std/string.c std/string.h
        std/math.c std/math.h
        kernel/kernel.c kernel/kernel.h
        std/bitset.c std/bitset.h
        io/ps2.c io/ps2.h
        std/ordered_array.c std/ordered_array.h
        mm/kheap.c mm/kheap.h
        io/acpi.c io/acpi.h
        std/list.c std/list.h
        fs/ext2.c fs/ext2.h)

add_custom_target(linker.ld
        BYPRODUCTS linker.ld
        DEPENDS linker.ld.in
        VERBATIM
        COMMAND ${CMAKE_C_COMPILER} -P -CC -E -x c ${CMAKE_C_FLAGS} ${CMAKE_SOURCE_DIR}/bootstrap/linker.ld.in > linker.ld
        )

add_executable(kernel.bin ${BOOTSTRAP_SOURCES})
add_dependencies(kernel.bin linker.ld)
target_link_libraries(kernel.bin gcc)
target_link_options(kernel.bin PRIVATE -T "${CMAKE_CURRENT_BINARY_DIR}/linker.ld")
add_custom_command(TARGET kernel.bin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/kernel.bin ${CMAKE_BINARY_DIR}
        )
add_custom_command(TARGET kernel.bin POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS kernel.bin
        BYPRODUCTS kernel.lst
        VERBATIM
        COMMAND objdump -M intel -d kernel.bin > kernel.lst
        )

