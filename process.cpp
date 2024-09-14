#include "process.h"

#include "errno.h"
#include "estd/stdlib.h"
#include "file.h"
#include "fs/ext2.h"
#include "klibc.h"
#include "page_map.h"
#include "panic.h"
#include "scheduler.h"
#include "system.h"
#include "terminal.h"  // IWYU pragma: keep
#include "thread.h"

ProcessTable* ProcessTable::_instance = nullptr;

void ProcessTable::init() {
    ASSERT(_instance == nullptr);
    _instance = new ProcessTable;
}

Process* ProcessTable::create(const char* filename) {
    Process* process = new Process(_nextPid++, filename);
    // TODO: write an emplace_back function for Vector
    _processes.push_back(move(OwnPtr<Process>(process)));
    return process;
}

void ProcessTable::destroy(Process* process) {
    for (size_t i = 0; i < _processes.size(); ++i) {
        if (_processes[i].get() != process) continue;

        swap(_processes[i], _processes.back());
        _processes.pop_back();

        return;
    }

    panic("process not found");
}

Process::Process(pid_t pid, const char* filename) : pid(pid) {
    open(System::terminal());  // stdin
    open(System::terminal());  // stdout
    open(System::terminal());  // stderr

    // Look up the executable on disk
    auto inode = System::fs().lookup(filename);
    ASSERT(inode);

    // Allocate a fresh piece of page-aligned physical memory to store it
    uint64_t pagesNeeded = ceilDiv(inode->size(), PAGE_SIZE);
    imagePages = System::mm().pageAlloc(pagesNeeded);
    uint8_t* ptr = System::mm().physicalToVirtual(imagePages).ptr<uint8_t>();

    // Read the executable from disk
    if (!System::fs().readFullFile(*inode, ptr)) {
        panic("failed to read file");
    }

    // Create user address space and map the executable image into it
    addressSpace = System::mm().kaddressSpace().makeUserAddressSpace();
    VirtualAddress entryPoint = addressSpace->userMapBase();
    addressSpace->mapPages(entryPoint, imagePages, pagesNeeded);

    thread = OwnPtr<Thread>(new Thread(this, entryPoint));
    System::scheduler().runQueue.push_back(thread.get());
}

Process::~Process() { System::mm().pageFree(imagePages); }

int Process::open(const SharedPtr<File>& file) {
    // Find next available fd
    for (size_t i = 0; i < RLIMIT_NOFILE; ++i) {
        if (!openFiles[i]) {
            openFiles[i] = OpenFileDescription::create(file);
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
