#pragma once

extern "C" [[gnu::naked]] void syscallEntry();

void initSyscalls();
