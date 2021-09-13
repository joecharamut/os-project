cmake_minimum_required(VERSION 3.18)

find_program(X86_64_COMPILER x86_64-elf-gcc REQUIRED)
find_program(ASM_COMPILER nasm REQUIRED)

message(STATUS "x86_64 compiler is ${X86_64_COMPILER}")
message(STATUS "ASM compiler is ${ASM_COMPILER}")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${X86_64_COMPILER})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_NASM_COMPILER ${ASM_COMPILER})

set(WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wformat")
set(SUPPRESS_FLAGS "")
set(ERROR_FLAGS "-Werror=implicit-function-declaration")

set(CMAKE_C_FLAGS "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -O0 -g -std=c11 ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS} -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -lgcc")

set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <INCLUDES> <FLAGS> -f ${CMAKE_ASM_NASM_OBJECT_FORMAT} -o <OBJECT> <SOURCE>")

set(TEST64_SOURCES
        main.c)
add_executable(test64.bin ${TEST64_SOURCES})
set_target_properties(test64.bin PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/test64/linker.ld)
target_link_options(test64.bin PRIVATE -T "${CMAKE_SOURCE_DIR}/test64/linker.ld")
add_custom_command(TARGET test64.bin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/test64.bin ${CMAKE_BINARY_DIR}
        )
