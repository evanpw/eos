cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/x86_64-elf.cmake)

# Custom build type
set(CMAKE_CXX_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_STANDARD "-O -g" CACHE STRING "" FORCE)
MARK_AS_ADVANCED(CMAKE_CXX_FLAGS_STANDARD CMAKE_C_FLAGS_STANDARD CMAKE_EXE_LINKER_FLAGS_STANDARD)

project(eos LANGUAGES CXX ASM_NASM)


## Kernel binary ##

set(KERNEL_SOURCES
    assertions.cpp
    file.cpp
    interrupts.cpp
    keyboard.cpp
    kmain.cpp
    mem.cpp
    new.cpp
    page_map.cpp
    print.cpp
    process.cpp
    screen.cpp
    stdlib.cpp
    syscalls.cpp
    system.cpp
    terminal.cpp
    thread.cpp
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

target_link_options(kernel.elf PRIVATE -T ${CMAKE_SOURCE_DIR}/kernel.ld -nostdlib -lgcc LINKER:--no-warn-rwx-segments)
target_include_directories(kernel.elf PRIVATE ${CMAKE_SOURCE_DIR}/libc/include)

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

set(USER_SOURCES
    user.cpp
    libc/unistd.cpp
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

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/boot.inc
    DEPENDS ${CMAKE_BINARY_DIR}/kernel.bin ${CMAKE_BINARY_DIR}/user.bin
    COMMAND python
        ${CMAKE_SOURCE_DIR}/scripts/get_kernel_size.py
        ${CMAKE_BINARY_DIR}/kernel.bin
        ${CMAKE_BINARY_DIR}/user.bin
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
    DEPENDS
        ${CMAKE_BINARY_DIR}/boot.bin
        ${CMAKE_BINARY_DIR}/kernel.bin
        ${CMAKE_BINARY_DIR}/user.bin
    COMMAND
        cat
        ${CMAKE_BINARY_DIR}/boot.bin
        ${CMAKE_BINARY_DIR}/kernel.bin
        ${CMAKE_BINARY_DIR}/user.bin
        > ${CMAKE_BINARY_DIR}/diskimg
    COMMAND
        python
        ${CMAKE_SOURCE_DIR}/scripts/pad_file.py
        ${CMAKE_BINARY_DIR}/diskimg
)
