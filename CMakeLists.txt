cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/x86_64-elf.cmake)

project(eos LANGUAGES CXX ASM_NASM)


## Kernel binary ##

set(KERNEL_SOURCES
    kmain.cpp
    screen.cpp
    assertions.cpp
    print.cpp
    mem.cpp
    stdlib.cpp
    new.cpp
    system.cpp
    interrupts.cpp
    user.cpp
    keyboard.cpp
    terminal.cpp
    page_map.cpp
)

add_executable(kernel.elf ${KERNEL_SOURCES})

target_compile_options(
    kernel.elf
    PRIVATE
    -ffreestanding
    -mno-red-zone
    -fno-rtti
    -fno-exceptions
    -mgeneral-regs-only
)

target_link_options(kernel.elf PRIVATE -Ttext=0x7E00 -ekmain -nostdlib -lgcc)

# Convert kernel.elf to flat binary
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel.bin
    DEPENDS kernel.elf
    COMMAND ${CMAKE_OBJCOPY}
        -O binary ${CMAKE_BINARY_DIR}/kernel.elf
        ${CMAKE_BINARY_DIR}/kernel.bin
        --set-section-flags .bss=alloc,load,contents
)


## Boot Sector ##

set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g -F dwarf")
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_LINK_EXECUTABLE "${CMAKE_LINKER} <CMAKE_ASM_NASM_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/boot.inc
    MAIN_DEPENDENCY ${CMAKE_BINARY_DIR}/kernel.bin
    COMMAND python
        ${CMAKE_SOURCE_DIR}/get_kernel_size.py
        ${CMAKE_BINARY_DIR}/kernel.bin
        ${CMAKE_BINARY_DIR}/boot.inc
)

set(BOOT_SOURCES
    boot.asm
    ${CMAKE_BINARY_DIR}/boot.inc
)

set_source_files_properties(
    ${BOOT_SOURCES}
    PROPERTIES
    LANGUAGE ASM_NASM
)

set_source_files_properties(
    ${CMAKE_BINARY_DIR}/boot.inc
    PROPERTIES
    HEADER_FILE_ONLY ON
)

add_executable(boot.elf ${BOOT_SOURCES})
target_include_directories(boot.elf PRIVATE ${CMAKE_BINARY_DIR})
target_link_options(boot.elf PRIVATE -Ttext=0x7C00 -melf_x86_64 -e0x7C00)

# Convert boot.elf to flat binary
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/boot.bin
    DEPENDS boot.elf
    COMMAND ${CMAKE_OBJCOPY}
        -O binary ${CMAKE_BINARY_DIR}/boot.elf
        ${CMAKE_BINARY_DIR}/boot.bin
)


## Disk Image ##

add_custom_target(build-disk-image ALL DEPENDS ${CMAKE_BINARY_DIR}/diskimg)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/diskimg
    DEPENDS ${CMAKE_BINARY_DIR}/boot.bin ${CMAKE_BINARY_DIR}/kernel.bin
    COMMAND
        cat
        ${CMAKE_BINARY_DIR}/boot.bin
        ${CMAKE_BINARY_DIR}/kernel.bin
        > ${CMAKE_BINARY_DIR}/diskimg
)
