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

set(CMAKE_C_FLAGS "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -O0 -g -std=c11 ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS} -ffreestanding")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf32)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

add_custom_target(mbr.bin
        ${CMAKE_ASM_NASM_COMPILER} ${INCLUDES} ${FLAGS} -f bin -o "${CMAKE_CURRENT_BINARY_DIR}/mbr.bin" "${CMAKE_CURRENT_SOURCE_DIR}/mbr.asm"
        )

set(STAGE2_SOURCES
        entry.asm
        stage2.c
        debug.c debug.asm debug.h
        cpu.asm cpu.h
        disk.c disk.asm disk.h
        mem.asm mem.h
        elf.h)
add_executable(stage2.bin ${STAGE2_SOURCES})
set_target_properties(stage2.bin PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/bootloader/linker.ld)
target_compile_options(stage2.bin PRIVATE $<$<COMPILE_LANGUAGE:C>:-m16>)
target_link_options(stage2.bin PRIVATE -T "${CMAKE_SOURCE_DIR}/bootloader/linker.ld")
add_custom_command(TARGET stage2.bin POST_BUILD
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS stage2.bin
        BYPRODUCTS stage2.lst
        VERBATIM
        COMMAND objdump -m i8086 -b binary -D -M intel stage2.bin > stage2.lst
        )
