cmake_minimum_required(VERSION 3.18)

find_program(X86_64_PE_COMPILER x86_64-w64-mingw32-gcc REQUIRED)

message(STATUS "x86_64_PE compiler is ${X86_64_PE_COMPILER}")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE SHARED_LIBRARY)

set(CMAKE_C_COMPILER ${X86_64_PE_COMPILER})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WARNING_FLAGS -Wall -Wextra -Wpedantic -Wformat)
set(SUPPRESS_FLAGS "")
set(ERROR_FLAGS -Werror=implicit-function-declaration)

include(ExternalProject)
ExternalProject_Add(gnu-efi
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi
        URL "https://downloads.sourceforge.net/project/gnu-efi/gnu-efi-3.0.14.tar.bz2"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E env
            ARCH=x86_64
            CROSS_COMPILE=x86_64-w64-mingw32-
            make -C <SOURCE_DIR> lib
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        )

#include_directories(/usr/include/efi)
#include_directories(/usr/include/efi/x86_64)
#include_directories(/usr/include/efi/protocol)
add_compile_options(
        -ffreestanding
        -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
        -std=c11
        ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS}
)

add_link_options(
        -nostdlib
        -Wl,-dll
        -shared
        -Wl,--subsystem,10
        -e efi_main
)
add_executable(loader.efi
        elf.h
        main.c)
target_include_directories(loader.efi PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/inc
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/inc/x86_64
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/inc/protocol
        )
target_link_directories(loader.efi PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/x86_64/lib)
target_link_libraries(loader.efi efi)
add_dependencies(loader.efi gnu-efi)

