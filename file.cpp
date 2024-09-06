#include "file.h"

OwnPtr<OpenFileDescription> OpenFileDescription::create(const SharedPtr<File>& file) {
    return OwnPtr<OpenFileDescription>(new OpenFileDescription{file});
}
