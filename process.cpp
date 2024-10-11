#include "process.h"

#include "api/errno.h"
#include "estd/utility.h"
#include "file.h"
#include "fs/ext2.h"
#include "klibc.h"
#include "mm.h"
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

pid_t ProcessTable::acquirePid() {
    SpinlockLocker locker(_lock);
    return _nextPid++;
}

void ProcessTable::releasePid(pid_t /*pid*/) {
    // TODO: reuse pids
}

void ProcessTable::insertProcess(Process* process) {
    SpinlockLocker locker(_lock);
    _processes.push_back(estd::move(estd::unique_ptr<Process>(process)));
}

void ProcessTable::removeProcess(Process* process) {
    SpinlockLocker locker(_lock);

    for (size_t i = 0; i < _processes.size(); ++i) {
        if (_processes[i].get() != process) continue;

        estd::swap(_processes[i], _processes.back());
        pid_t pid = process->pid;
        _processes.pop_back();  // calls the process destructor
        releasePid(pid);

        return;
    }

    panic("process not found");
}

Process* ProcessTable::findProcess(pid_t pid) {
    SpinlockLocker locker(_lock);

    for (auto& p : _processes) {
        if (p->pid == pid) {
            return p.get();
        }
    }

    return nullptr;
}

int ProcessTable::waitProcess(pid_t pid) {
    Process* process = findProcess(pid);

    if (!process) {
        return -ECHILD;
    }

    // Wait for the process to exit
    process->lock.lock();
    if (process->status != ProcessStatus::Exited) {
        sys.scheduler().sleepThread(process->exitBlocker, &process->lock);
    }
    process->lock.unlock();

    // Remove the process from the process table and destroy it
    removeProcess(process);

    return 0;
}

Process* Process::create(const char* path, const char* argv[], uint32_t initialCwdIno) {
    ProcessTable& ptable = ProcessTable::the();
    pid_t pid = ptable.acquirePid();

    Process* process = new Process(pid, path, argv, initialCwdIno);
    ptable.insertProcess(process);

    return process;
}

Process::Process(pid_t pid, const char* path, const char* argv[], uint32_t initialCwdIno)
: pid(pid), cwdIno(initialCwdIno), exitBlocker(new Blocker) {
    open(sys.terminal());  // stdin
    open(sys.terminal());  // stdout
    open(sys.terminal());  // stderr

    // Look up the executable on disk
    uint32_t ino = sys.fs().lookup(cwdIno, path);
    ASSERT(ino != ext2::BAD_INO);
    auto inode = sys.fs().readInode(ino);
    ASSERT(inode);

    // Allocate a fresh piece of page-aligned physical memory to store it
    textPagesCount = ceilDiv(inode->size(), PAGE_SIZE);
    textPages = mm.pageAlloc(textPagesCount);
    byte* ptr = mm.physicalToVirtual(textPages).ptr<byte>();

    // Read the executable from disk
    if (!sys.fs().readFullFile(*inode, ptr)) {
        panic("failed to read file");
    }

    // Create user address space and map the executable image into it
    addressSpace = mm.kaddressSpace().makeUserAddressSpace();
    addressSpace->mapPages(textStart(), textPages, textPagesCount);

    // Find the program name by taking everything after the last slash
    const char* p = path;
    const char* lastSlash = strchr(path, '/');
    while (lastSlash) {
        p = lastSlash + 1;
        lastSlash = strchr(p, '/');
    }
    const char* programName = p;

    thread = Thread::createUserThread(this, textStart(), programName, argv);
    sys.scheduler().startThread(thread);
}

int Process::execvp(const char* path, const char* argv[]) {
    // Make a copy of the arguments in kernel memory before we wipe the address space
    char* pathCopy = new char[strlen(path) + 1];
    memcpy(pathCopy, path, strlen(path) + 1);

    estd::vector<const char*> argvCopy;
    for (size_t i = 0; argv[i]; ++i) {
        char* argCopy = new char[strlen(argv[i]) + 1];
        memcpy(argCopy, argv[i], strlen(argv[i]) + 1);
        argvCopy.push_back(argCopy);
    }
    argvCopy.push_back(nullptr);

    // Close all open files
    for (size_t i = 0; i < RLIMIT_NOFILE; ++i) {
        openFiles[i].clear();
    }

    open(sys.terminal());  // stdin
    open(sys.terminal());  // stdout
    open(sys.terminal());  // stderr

    // Free and unmap any physical memory used by the process
    mm.pageFree(textPages, textPagesCount);
    if (heapPagesCount > 0) {
        mm.pageFree(heapPages, heapPagesCount);
        heapPagesCount = 0;
    }

    mm.kaddressSpace().clearUserAddressSpace(*addressSpace);

    // Look up the executable on disk
    uint32_t ino = sys.fs().lookup(cwdIno, pathCopy);
    ASSERT(ino != ext2::BAD_INO);
    auto inode = sys.fs().readInode(ino);
    ASSERT(inode);

    // Allocate a fresh piece of page-aligned physical memory to store it
    textPagesCount = ceilDiv(inode->size(), PAGE_SIZE);
    textPages = mm.pageAlloc(textPagesCount);
    byte* ptr = mm.physicalToVirtual(textPages).ptr<byte>();

    // Read the executable from disk
    if (!sys.fs().readFullFile(*inode, ptr)) {
        panic("failed to read file");
    }

    // Map the executable image into the user address space
    addressSpace->mapPages(textStart(), textPages, textPagesCount);

    // Find the program name by taking everything after the last slash
    const char* p = pathCopy;
    const char* lastSlash = strchr(pathCopy, '/');
    while (lastSlash) {
        p = lastSlash + 1;
        lastSlash = strchr(p, '/');
    }
    const char* programName = p;

    // Unlink the old thread from this process, so that we don't destroy the process
    // when the thread exits
    thread->process = nullptr;

    // Replace the current thread with a new one
    thread = Thread::createUserThread(this, textStart(), programName, argvCopy.data());

    delete[] pathCopy;
    for (const char* arg : argvCopy) {
        delete[] arg;
    }

    // This won't return, and the scheduler will delete the old thread once it's no
    // longer running
    sys.scheduler().replaceThread(thread);
}

Process* Process::fork(TrapRegisters& trapRegs) {
    ProcessTable& ptable = ProcessTable::the();

    Process* child = new Process;
    child->pid = ptable.acquirePid();

    for (size_t i = 0; i < RLIMIT_NOFILE; ++i) {
        child->openFiles[i] = openFiles[i];
    }

    child->cwdIno = cwdIno;
    child->status = ProcessStatus::Running;
    child->exitBlocker.assign(new Blocker);

    child->textPagesCount = textPagesCount;
    child->textPages = mm.pageAlloc(textPagesCount);

    // TODO: use copy-on-write instead of copying up front
    byte* destPtr = mm.physicalToVirtual(child->textPages).ptr<byte>();
    byte* srcPtr = mm.physicalToVirtual(textPages).ptr<byte>();
    memcpy(destPtr, srcPtr, textPagesCount * PAGE_SIZE);

    child->heapPagesCount = heapPagesCount;
    if (child->heapPagesCount > 0) {
        child->heapPages = mm.pageAlloc(child->heapPagesCount);
        byte* destPtr = mm.physicalToVirtual(child->heapPages).ptr<byte>();
        byte* srcPtr = mm.physicalToVirtual(heapPages).ptr<byte>();
        memcpy(destPtr, srcPtr, heapPagesCount * PAGE_SIZE);
    }

    child->addressSpace = mm.kaddressSpace().makeUserAddressSpace();

    child->addressSpace->mapPages(child->textStart(), child->textPages,
                                  child->textPagesCount);
    if (child->heapPagesCount > 0) {
        child->addressSpace->mapPages(child->heapStart(), child->heapPages,
                                      child->heapPagesCount);
    }

    child->thread = Thread::createUserThread(child, thread, trapRegs);

    ptable.insertProcess(child);
    sys.scheduler().startThread(child->thread);

    return child;
}

Process::~Process() {
    mm.pageFree(textPages, textPagesCount);

    if (heapPagesCount > 0) {
        mm.pageFree(heapPages, heapPagesCount);
    }
}

void Process::createHeap(size_t size) {
    // TODO: allow expanding an existing heap
    ASSERT(heapPagesCount == 0);

    heapPagesCount = ceilDiv(size, PAGE_SIZE);
    heapPages = mm.pageAlloc(heapPagesCount);
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

void Process::exit() {
    SpinlockLocker locker(lock);
    ASSERT(status == ProcessStatus::Exiting);
    status = ProcessStatus::Exited;
    sys.scheduler().wakeThreadsLocked(exitBlocker);
    // Thread cleanup is handled by the scheduler, so we don't have to do that here
}
