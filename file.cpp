#include "file.h"

estd::unique_ptr<OpenFileDescription> OpenFileDescription::create(
    const estd::shared_ptr<File>& file) {
    return estd::unique_ptr<OpenFileDescription>(new OpenFileDescription{file});
}
