#include "file.h"

OwnPtr<OpenFileDescription> File::open() {
    return OwnPtr<OpenFileDescription>(new OpenFileDescription{*this});
}
