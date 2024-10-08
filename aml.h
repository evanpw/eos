// Parses AML bytecode read from ACPI tables
#pragma once
#include <stddef.h>
#include <stdint.h>

#include "estd/stddef.h"

void parseAML(byte* code, size_t length);
