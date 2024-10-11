#include "file.h"

estd::shared_ptr<OpenFileDescription> OpenFileDescription::create(
    const estd::shared_ptr<File>& file) {
    return estd::make_shared<OpenFileDescription>(file);
}
