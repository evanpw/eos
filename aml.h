// Parses AML bytecode read from ACPI tables
#pragma once
#include <stddef.h>
#include <stdint.h>

void parseAML(uint8_t* code, size_t length);
