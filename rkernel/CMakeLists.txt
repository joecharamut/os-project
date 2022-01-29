cmake_minimum_required(VERSION 3.18)

find_program(X86_64_COMPILER x86_64-elf-gcc REQUIRED)
find_program(RUSTUP_EXECUTABLE rustup REQUIRED)

message(STATUS "x86_64 compiler is ${X86_64_COMPILER}")
message(STATUS "rustup executable is ${RUSTUP_EXECUTABLE}")

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM Generic)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER ${X86_64_COMPILER})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WARNING_FLAGS "-Wall -Wextra -Wpedantic -Wformat")
set(SUPPRESS_FLAGS "")
set(ERROR_FLAGS "-Werror=implicit-function-declaration")

set(CMAKE_C_FLAGS "-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=. -O0 -g -std=c11 ${WARNING_FLAGS} ${SUPPRESS_FLAGS} ${ERROR_FLAGS} -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -lgcc -zmax-page-size=0x1000")

set(RUST_TOOLCHAIN "nightly")

set(RUST_KERNEL_SOURCES
        Cargo.toml
        src/lib.rs

        src/acpi/mod.rs
        src/acpi/structs.rs

        src/boot/mod.rs
        src/boot/entry.rs

        src/io/mod.rs
        src/io/serial.rs

        src/mm/mod.rs
        src/mm/kheap.rs
        src/mm/pager.rs

        src/pci/mod.rs
        src/pci/pciconfig.rs

        src/x86_64/mod.rs
        src/x86_64/addr.rs
        )

set(RUST_SOURCES ${RUST_KERNEL_SOURCES})

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libkernel.a
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${RUSTUP_EXECUTABLE} run ${RUST_TOOLCHAIN} cargo build
            --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
            --target ${CMAKE_CURRENT_SOURCE_DIR}/target.json
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}
            --out-dir ${CMAKE_CURRENT_BINARY_DIR}
            -Z build-std=core,alloc,compiler_builtins
            -Z build-std-features=compiler-builtins-mem
            -Z unstable-options
        DEPENDS ${RUST_SOURCES}
        )
add_custom_target(libkernel.a DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libkernel.a)

add_executable(rkernel.bin bootstrap.c)
add_dependencies(rkernel.bin libkernel.a)
target_link_libraries(rkernel.bin ${CMAKE_CURRENT_BINARY_DIR}/libkernel.a)

set_target_properties(rkernel.bin PROPERTIES LINK_DEPENDS ${CMAKE_SOURCE_DIR}/rkernel/linker.ld)
target_link_options(rkernel.bin PRIVATE -T "${CMAKE_SOURCE_DIR}/rkernel/linker.ld")
set_source_files_properties(bootstrap.c PROPERTIES COMPILE_OPTIONS "-fno-asynchronous-unwind-tables")

add_custom_target(cargo-clean
        COMMAND ${RUSTUP_EXECUTABLE} run ${RUST_TOOLCHAIN} cargo clean
            --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
            --target ${CMAKE_CURRENT_SOURCE_DIR}/target.json
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}
        )

add_custom_command(TARGET rkernel.bin POST_BUILD
        COMMAND objdump -C -d -Mintel ${CMAKE_CURRENT_BINARY_DIR}/rkernel.bin > rkernel.lst
        )

