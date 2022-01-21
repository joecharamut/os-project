cmake_minimum_required(VERSION 3.18)

find_program(X86_64_PE_COMPILER x86_64-w64-mingw32-gcc REQUIRED)
message(STATUS "x86_64_PE compiler is ${X86_64_PE_COMPILER}")

enable_language(C)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE SHARED_LIBRARY)

set(CMAKE_C_COMPILER ${X86_64_PE_COMPILER})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WARNING_FLAGS -Wall -Wextra -pedantic -Wformat)
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

ExternalProject_Add(scalable-font2
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/scalable-font2
        URL "https://gitlab.com/bztsrc/scalable-font2/-/archive/master/scalable-font2-master.zip"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND
            cd ${CMAKE_CURRENT_BINARY_DIR}/scalable-font2/src/scalable-font2/sfnconv && make && cd - &&
            cd ${CMAKE_CURRENT_BINARY_DIR}/scalable-font2/src/scalable-font2/sfnedit && make && cd -
        BUILD_IN_SOURCE TRUE
        USES_TERMINAL_BUILD TRUE
        INSTALL_COMMAND ""
        )

add_compile_options(
        -ffreestanding
        -fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.
        -std=c11
        -fshort-wchar
        $<IF:$<CONFIG:Debug>,-O0,-O2>
        ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS}
)

add_link_options(
        -nostdlib
        -Wl,-dll
        -shared
        -Wl,--subsystem,10
        -e efi_main
)

include_directories(SYSTEM PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/inc
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/inc/x86_64
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/inc/protocol
        ${CMAKE_CURRENT_BINARY_DIR}/scalable-font2/src/scalable-font2
        )
link_directories(PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/gnu-efi/src/gnu-efi/x86_64/lib
        )

set(LOADER_SOURCES
        main.c
        elf.c elf.h
        file.c file.h
        video.c video.h
        mem.c mem.h
        debug.c debug.h)

add_executable(loader.efi ${LOADER_SOURCES})
add_dependencies(loader.efi gnu-efi scalable-font2)
target_link_libraries(loader.efi efi gcc)

add_executable(stub.efi stub.c)
add_dependencies(stub.efi gnu-efi)
target_link_libraries(stub.efi efi)
add_custom_command(TARGET stub.efi POST_BUILD COMMAND strip stub.efi)
