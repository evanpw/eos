cmake_minimum_required(VERSION 3.20)

## Userland ##

add_compile_options(
    -ffreestanding
    -mno-red-zone
    -fno-rtti
    -fno-exceptions
    -mgeneral-regs-only
    -mcmodel=large
)

add_link_options(
    -T ${CMAKE_CURRENT_SOURCE_DIR}/user.ld
    -nostdlib
    -lgcc
)

include_directories(
    ${CMAKE_SOURCE_DIR}
    libc/include
    libc/
)

add_library(
    libc
    libc/_start.cpp
    libc/arpa/inet.cpp
    libc/dirent.cpp
    libc/errno.cpp
    libc/fcntl.cpp
    libc/heap.cpp
    libc/stdio.cpp
    libc/stdlib.cpp
    libc/string.cpp
    libc/sys/wait.cpp
    libc/syscall.cpp
    libc/unistd.cpp
)

add_library(estd_user ${ESTD_SOURCES})
target_link_libraries(estd_user PRIVATE libc)

macro(make_user_target name)
    add_executable(${name}.elf ${ARGN})
    target_link_libraries(${name}.elf PRIVATE estd_user)

    # Convert to flat binary format
    add_custom_command(
        TARGET ${name}.elf
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY}
            -O binary ${CMAKE_CURRENT_BINARY_DIR}/${name}.elf
            ${CMAKE_CURRENT_BINARY_DIR}/${name}
            --set-section-flags .bss=alloc,load,contents
    )
endmacro()

make_user_target(shell shell.cpp)
make_user_target(spam spam.cpp)
make_user_target(cat cat.cpp)

add_custom_target(
    userland
    DEPENDS
    shell.elf
    spam.elf
    cat.elf
)

set(USERLAND_BINARIES
    ${CMAKE_CURRENT_BINARY_DIR}/shell
    ${CMAKE_CURRENT_BINARY_DIR}/spam
    ${CMAKE_CURRENT_BINARY_DIR}/cat
    PARENT_SCOPE
)
