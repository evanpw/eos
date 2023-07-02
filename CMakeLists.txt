cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/x86_64-elf.cmake)

# Custom build type
set(CMAKE_CXX_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
MARK_AS_ADVANCED(CMAKE_CXX_FLAGS_STANDARD CMAKE_C_FLAGS_STANDARD CMAKE_EXE_LINKER_FLAGS_STANDARD)

project(eos LANGUAGES CXX ASM_NASM)


# Share between kernel and userland
set(ESTD_SOURCES
    estd/assertions.cpp
    estd/new.cpp
    estd/print.cpp
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
target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libc/include)

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

set(LIBC_SOURCES
    libc/stdio.cpp
    libc/stdlib.cpp
    libc/string.cpp
    libc/syscall.cpp
    libc/unistd.cpp
)

set(USER_SOURCES
    user.cpp
    ${LIBC_SOURCES}
    ${ESTD_SOURCES}
)

add_executable(user.elf ${USER_SOURCES})

target_compile_options(
    user.elf
    PRIVATE
    -ffreestanding
    -mno-red-zone
    -fno-rtti
    -fno-exceptions
    -mgeneral-regs-only
    -mcmodel=large
)

target_link_options(user.elf PRIVATE -T ${CMAKE_SOURCE_DIR}/user.ld -nostdlib -lgcc)
target_include_directories(user.elf PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/libc/include)

# Convert user.elf to flat binary
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/user.bin
    DEPENDS user.elf
    COMMAND ${CMAKE_OBJCOPY}
        -O binary ${CMAKE_BINARY_DIR}/user.elf
        ${CMAKE_BINARY_DIR}/user.bin
        --set-section-flags .bss=alloc,load,contents
)


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

# image 1: boot loader + kernel.bin
add_custom_target(build-disk-image ALL DEPENDS ${CMAKE_BINARY_DIR}/diskimg)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/diskimg
    DEPENDS
        ${CMAKE_BINARY_DIR}/boot.bin
        ${CMAKE_BINARY_DIR}/kernel.bin
        ${CMAKE_BINARY_DIR}/user.bin
    COMMAND
        python
        ${CMAKE_SOURCE_DIR}/scripts/create_disk_image.py
        ${CMAKE_BINARY_DIR}/boot.bin
        ${CMAKE_BINARY_DIR}/kernel.bin
        ${CMAKE_BINARY_DIR}/user.bin
        ${CMAKE_BINARY_DIR}/diskimg
)

# image 2: user.bin
add_custom_target(build-disk-image-2 ALL DEPENDS ${CMAKE_BINARY_DIR}/diskimg2)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/diskimg2
    DEPENDS
        ${CMAKE_BINARY_DIR}/user.bin
    COMMAND
        ${CMAKE_SOURCE_DIR}/make_image.sh ${CMAKE_BINARY_DIR}
)
