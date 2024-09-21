#include "process.h"

#include "api/errno.h"
#include "estd/utility.h"
#include "file.h"
#include "fs/ext2.h"
#include "klibc.h"
#include "page_map.h"
#include "panic.h"
#include "system.h"
#include "terminal.h"  // IWYU pragma: keep
#include "thread.h"

ProcessTable* ProcessTable::_instance = nullptr;

void ProcessTable::init() {
    ASSERT(_instance == nullptr);
    _instance = new ProcessTable;
}

Process* ProcessTable::create(const char* path, const char* argv[]) {
    SpinlockLocker locker(_lock);

    Process* process = new Process(_nextPid++, path, argv);
    // TODO: write an emplace_back function for vector
    _processes.push_back(estd::move(estd::unique_ptr<Process>(process)));
    return process;
}

void ProcessTable::destroy(Process* process) {
    SpinlockLocker locker(_lock);

    for (size_t i = 0; i < _processes.size(); ++i) {
        if (_processes[i].get() != process) continue;

        estd::swap(_processes[i], _processes.back());
        _processes.pop_back();

        for (size_t j = 0; j < _blockers.size(); ++j) {
            if (_blockers[j]->pid != process->pid) continue;

            System::scheduler().wakeThreads(_blockers.back());
            estd::swap(_blockers[j], _blockers.back());
            _blockers.pop_back();
        }

        return;
    }

    panic("process not found");
}

int ProcessTable::waitProcess(pid_t pid) {
    if (pid < 1) {
        return -ECHILD;
    }

    SpinlockLocker locker(_lock);

    bool found = false;
    for (auto& process : _processes) {
        if (process->pid == pid) {
            found = true;
            break;
        }
    }

    if (!found) {
        return -ECHILD;
    }

    estd::shared_ptr<ProcessBlocker> blocker(new ProcessBlocker(pid));
    _blockers.push_back(blocker);
    System::scheduler().sleepThread(blocker, &_lock);

    return 0;
}

Process::Process(pid_t pid, const char* path, const char* argv[])
: pid(pid), cwdIno(ext2::ROOT_INO) {
    open(System::terminal());  // stdin
    open(System::terminal());  // stdout
    open(System::terminal());  // stderr

    // Look up the executable on disk
    uint32_t ino = System::fs().lookup(cwdIno, path);
    ASSERT(ino != ext2::BAD_INO);
    auto inode = System::fs().readInode(ino);
    ASSERT(inode);

    // Allocate a fresh piece of page-aligned physical memory to store it
    imagePagesCount = ceilDiv(inode->size(), PAGE_SIZE);
    imagePages = System::mm().pageAlloc(imagePagesCount);
    uint8_t* ptr = System::mm().physicalToVirtual(imagePages).ptr<uint8_t>();

    // Read the executable from disk
    if (!System::fs().readFullFile(*inode, ptr)) {
        panic("failed to read file");
    }

    // Create user address space and map the executable image into it
    addressSpace = System::mm().kaddressSpace().makeUserAddressSpace();
    VirtualAddress entryPoint = addressSpace->userMapBase();
    addressSpace->mapPages(entryPoint, imagePages, imagePagesCount);

    // Find the program name by taking everything after the last slash
    const char* p = path;
    const char* lastSlash = strchr(path, '/');
    while (lastSlash) {
        p = lastSlash + 1;
        lastSlash = strchr(p, '/');
    }
    const char* programName = p;

    thread = Thread::createUserThread(this, entryPoint, programName, argv);
    System::scheduler().startThread(thread.get());
}

Process::~Process() {
    System::mm().pageFree(imagePages, imagePagesCount);

    if (heapPagesCount > 0) {
        System::mm().pageFree(heapPages, heapPagesCount);
    }
}

void Process::createHeap(size_t size) {
    // TODO: allow expanding an existing heap
    ASSERT(heapPagesCount == 0);

    heapPagesCount = ceilDiv(size, PAGE_SIZE);
    heapPages = System::mm().pageAlloc(heapPagesCount);
    addressSpace->mapPages(heapStart(), heapPages, heapPagesCount);
}

int Process::open(const estd::shared_ptr<File>& file) {
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
