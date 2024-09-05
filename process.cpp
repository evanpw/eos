#include "process.h"

#include "errno.h"
#include "file.h"
#include "fs/ext2.h"
#include "klibc.h"
#include "page_map.h"
#include "panic.h"
#include "system.h"
#include "terminal.h"
#include "thread.h"

pid_t Process::s_nextPid = 1;

Process::Process(const char* filename) {
    pid = s_nextPid++;
    open(System::terminal());  // stdin
    open(System::terminal());  // stdout
    open(System::terminal());  // stderr

    // Look up the executable on disk
    auto inode = System::fs().lookup(filename);
    ASSERT(inode);

    // Allocate a fresh piece of page-aligned physical memory to store it
    uint64_t pagesNeeded = ceilDiv(inode->size(), PAGE_SIZE);
    PhysicalAddress userDest = System::mm().pageAlloc(pagesNeeded);
    uint8_t* ptr = System::mm().physicalToVirtual(userDest).ptr<uint8_t>();

    // Read the executable from disk
    if (!System::fs().readFullFile(*inode, ptr)) {
        panic("failed to read file");
    }

    // Create user address space and map the executable image into it
    addressSpace = System::mm().kaddressSpace().makeUserAddressSpace();
    VirtualAddress entryPoint = addressSpace->userMapBase();
    addressSpace->mapPages(entryPoint, userDest, pagesNeeded);

    thread = OwnPtr<Thread>(new Thread(this, entryPoint));
}

Process::~Process() = default;

int Process::open(File& file) {
    // Find next available fd
    for (size_t i = 0; i < RLIMIT_NOFILE; ++i) {
        if (!openFiles[i]) {
            openFiles[i] = file.open();
            return i;
        }
    }

    return -EMFILE;
}

int Process::close(int fd) {
    if (fd < 0 || fd >= RLIMIT_NOFILE || !openFiles[fd]) {
        return -EBADF;
    }

    openFiles[fd].clear();
    return 0;
}
