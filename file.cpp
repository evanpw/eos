#include "file.h"

OpenFileDescription* File::open() { return new OpenFileDescription{*this}; }
