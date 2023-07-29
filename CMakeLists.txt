cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/x86_64-elf.cmake)

# Use C++20 with GNU extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# Custom build type
set(CMAKE_CXX_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
MARK_AS_ADVANCED(CMAKE_CXX_FLAGS_STANDARD CMAKE_C_FLAGS_STANDARD CMAKE_EXE_LINKER_FLAGS_STANDARD)

project(eos LANGUAGES CXX ASM_NASM ASM)


# Share between kernel and userland
set(ESTD_SOURCES
    ${CMAKE_SOURCE_DIR}/estd/assertions.cpp
    ${CMAKE_SOURCE_DIR}/estd/new.cpp
    ${CMAKE_SOURCE_DIR}/estd/print.cpp
)


## Kernel binary ##

set(KERNEL_SOURCES
    acpi.cpp
    aml.cpp
    file.cpp
    fs/ext2.cpp
    ide.cpp
    interrupts.cpp
    keyboard.cpp
    klibc.cpp
    kmain.cpp
    mem.cpp
    page_map.cpp
    panic.cpp
    pci.cpp
    process.cpp
    processor.cpp
    screen.cpp
    syscalls.cpp
    system.cpp
    terminal.cpp
    thread.cpp
    timer.cpp
    entry.S
    ${ESTD_SOURCES}
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
    -Wall
    -Wextra
    -DKERNEL
)

target_link_options(kernel.elf PRIVATE -T ${CMAKE_SOURCE_DIR}/kernel.ld -nostdlib -lgcc LINKER:--no-warn-rwx-segments)
target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/user/libc/include)

# Convert kernel.elf to flat binary
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/kernel.bin
    DEPENDS kernel.elf
    COMMAND ${CMAKE_OBJCOPY}
        -O binary ${CMAKE_BINARY_DIR}/kernel.elf
        ${CMAKE_BINARY_DIR}/kernel.bin
        --set-section-flags .bss=alloc,load,contents
)


## Userland ##
add_subdirectory(user)


## Boot Sector ##

set(CMAKE_ASM_NASM_FLAGS_DEBUG "-g -F dwarf")
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_LINK_EXECUTABLE "${CMAKE_LINKER} <CMAKE_ASM_NASM_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(BOOT_SOURCES
    boot.asm
)

set_source_files_properties(
    ${BOOT_SOURCES}
    PROPERTIES
    LANGUAGE ASM_NASM
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
    DEPENDS
        ${CMAKE_BINARY_DIR}/boot.bin
        ${CMAKE_BINARY_DIR}/boot.elf
        ${CMAKE_BINARY_DIR}/kernel.bin
        userland
    COMMAND
        sudo python
        ${CMAKE_SOURCE_DIR}/scripts/create_disk_image.py
        ${CMAKE_BINARY_DIR}
)
