// Sets up the syscall interface, implements the syscall functions, and
// handles the user -> kernel entry point
#pragma once

extern "C" [[gnu::naked]] void syscallEntry();

void initSyscalls();
