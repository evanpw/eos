#include "syscalls.h"

#include <stdint.h>
#include <sys/socket.h>

#include "api/errno.h"
#include "api/syscalls.h"
#include "estd/print.h"
#include "file.h"
#include "fs/ext2_file.h"
#include "klibc.h"
#include "net/socket.h"
#include "process.h"
#include "processor.h"
#include "scheduler.h"
#include "system.h"
#include "thread.h"
#include "timer.h"
#include "trap.h"

using SyscallHandler = int64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                   uint64_t);

ssize_t sys_read(int fd, void* buffer, size_t count) {
    Process& process = *currentThread->process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || !process.openFiles[fd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = *description.file;
    return file.read(description, buffer, count);
}

ssize_t sys_read_dir(int fd, void* buffer, size_t count) {
    Process& process = *currentThread->process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || !process.openFiles[fd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = *description.file;
    return file.readDir(description, buffer, count);
}

ssize_t sys_write(int fd, const void* buffer, size_t count) {
    Process& process = *currentThread->process;

    if (fd < 0 || fd >= RLIMIT_NOFILE || !process.openFiles[fd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[fd];
    File& file = *description.file;
    return file.write(description, buffer, count);
}

pid_t sys_getpid() {
    Process& process = *currentThread->process;
    return process.pid;
}

[[noreturn]] void sys_exit(int status) {
    println("proc: user process exited with status {}", status);

    Process& process = *currentThread->process;
    {
        SpinlockLocker locker(process.lock);
        process.status = ProcessStatus::Exiting;
        process.exitCode = status;
    }

    // Does not return. The actual call to process.exit() is done in the scheduler after
    // switching away from this thread
    sys.scheduler().threadExit();
    __builtin_unreachable();
}

int64_t sys_sleep(int ticks) {
    // Will block the current thread and allow other threads to run
    sys.timer().sleep(ticks);
    return 0;
}

int64_t sys_open(const char* path, int /*oflag*/) {
    // TODO: handle flags correctly
    Process& process = *currentThread->process;

    uint32_t ino = sys.fs().lookup(process.cwdIno, path);
    if (ino == ext2::BAD_INO) {
        return -ENOENT;
    }

    auto inode = sys.fs().readInode(ino);
    if (!inode) {
        return -EIO;
    }

    estd::shared_ptr<Ext2File> file(new Ext2File(sys.fs(), move(inode)));
    return process.open(file);
}

int64_t sys_close(int fd) {
    Process& process = *currentThread->process;
    return process.close(fd);
}

pid_t sys_launch(const char* path, const char* argv[]) {
    Process& process = *currentThread->process;

    Process* child = Process::create(path, argv, process.cwdIno);
    return child->pid;
}

VirtualAddress sys_sbrk(intptr_t incr) {
    Process& process = *currentThread->process;

    if (incr < 0) {
        return -EINVAL;
    }

    if (incr > 0) {
        process.createHeap(incr);
    }

    return process.heapStart();
}

int64_t sys_getcwd(char* buffer, size_t size) {
    Process& process = *currentThread->process;
    return sys.fs().getPath(process.cwdIno, buffer, size);
}

int64_t sys_chdir(const char* path) {
    Process& process = *currentThread->process;

    uint32_t ino = sys.fs().lookup(process.cwdIno, path);
    if (ino == ext2::BAD_INO) {
        return -ENOENT;
    }

    auto inode = sys.fs().readInode(ino);
    if (!inode) {
        return -EIO;
    }

    if (!inode->isDirectory()) {
        return -ENOTDIR;
    }

    // TODO: process needs a lock
    process.cwdIno = ino;
    return 0;
}

int64_t sys_wait_pid(pid_t pid) {
    int result = ProcessTable::the().waitProcess(pid);
    if (result < 0) {
        return result;
    }

    return pid;
}

int64_t sys_socket(int domain, int type, int protocol) {
    // TODO: handle flags correctly
    Process& process = *currentThread->process;

    if (domain != AF_INET) return -EAFNOSUPPORT;
    if (protocol != 0) return -EPROTONOSUPPORT;

    estd::shared_ptr<Socket> socket;
    if (type == SOCK_STREAM) {
        socket.assign(new TcpSocket);
    } else if (type == SOCK_DGRAM) {
        socket.assign(new UdpSocket);
    } else {
        return -EPROTOTYPE;
    }

    return process.open(socket);
}

int64_t sys_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    Process& process = *currentThread->process;

    if (sockfd < 0 || sockfd >= RLIMIT_NOFILE || !process.openFiles[sockfd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[sockfd];
    File& file = *description.file;

    if (!file.isSocket()) {
        return -ENOTSOCK;
    }

    Socket& socket = static_cast<Socket&>(file);
    return socket.connect(addr, addrlen);
}

int64_t sys_send(int sockfd, const void* buffer, size_t length, int /*flags*/) {
    Process& process = *currentThread->process;

    if (sockfd < 0 || sockfd >= RLIMIT_NOFILE || !process.openFiles[sockfd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[sockfd];
    File& file = *description.file;

    if (!file.isSocket()) {
        return -ENOTSOCK;
    }

    // TODO: handle flags correctly

    return file.write(description, buffer, length);
}

int64_t sys_recv(int sockfd, void* buffer, size_t length, int /*flags*/) {
    Process& process = *currentThread->process;

    if (sockfd < 0 || sockfd >= RLIMIT_NOFILE || !process.openFiles[sockfd]) {
        return -EBADF;
    }

    OpenFileDescription& description = *process.openFiles[sockfd];
    File& file = *description.file;

    if (!file.isSocket()) {
        return -ENOTSOCK;
    }

    // TODO: handle flags correctly

    return file.read(description, buffer, length);
}

// We don't have static initialization, so this is initialized at runtime
SyscallHandler syscallTable[SYS_COUNT];

// Defined in entry.S
extern "C" void syscallEntryAsm();

// Called by syscallEntryAsm
extern "C" void syscallEntry(TrapRegisters& regs) {
    if (regs.rax >= SYS_COUNT) {
        regs.rax = -1;
        return;
    }

    auto fn = syscallTable[regs.rax];
    ASSERT(fn);

    regs.rax = fn(regs.rdi, regs.rsi, regs.rdx, regs.r10, regs.r8, regs.r9);
}

// Model-Specific Registers (MSRs)
static constexpr uint64_t IA32_EFER = 0xC0000080;
static constexpr uint64_t IA32_STAR = 0xC0000081;
static constexpr uint64_t IA32_LSTAR = 0xC0000082;
static constexpr uint64_t IA32_FMASK = 0xC0000084;

void initSyscalls() {
    // Set syscall enable bit of the IA32_EFER MSR
    Processor::wrmsr(IA32_EFER, Processor::rdmsr(IA32_EFER) | 1);

    // Set the upper dword of the IA32_STAR MSR to 0x0018'0008. This instructs
    // the CPU to set CS to 0x28 (0x18 + 0x10) and SS to 0x20 (0x18 + 0x08). And
    // it also sets up a later syscall to set CS to 0x08 and SS to 0x10
    uint32_t lowStar = lowBits(Processor::rdmsr(IA32_STAR), 32);
    Processor::wrmsr(IA32_STAR, concatBits((uint32_t)0x00180008, lowStar));

    // Set up syscall to jump to syscallEntry
    Processor::wrmsr(IA32_LSTAR, (uint64_t)&syscallEntryAsm);

    // Disable interrupts on syscall entry
    Processor::wrmsr(IA32_FMASK, 0x200);

    // Create table of syscall handlers
    syscallTable[SYS_read] = bit_cast<SyscallHandler>((void*)sys_read);
    syscallTable[SYS_write] = bit_cast<SyscallHandler>((void*)sys_write);
    syscallTable[SYS_getpid] = bit_cast<SyscallHandler>((void*)sys_getpid);
    syscallTable[SYS_exit] = bit_cast<SyscallHandler>((void*)sys_exit);
    syscallTable[SYS_sleep] = bit_cast<SyscallHandler>((void*)sys_sleep);
    syscallTable[SYS_open] = bit_cast<SyscallHandler>((void*)sys_open);
    syscallTable[SYS_close] = bit_cast<SyscallHandler>((void*)sys_close);
    syscallTable[SYS_launch] = bit_cast<SyscallHandler>((void*)sys_launch);
    syscallTable[SYS_read_dir] = bit_cast<SyscallHandler>((void*)sys_read_dir);
    syscallTable[SYS_sbrk] = bit_cast<SyscallHandler>((void*)sys_sbrk);
    syscallTable[SYS_getcwd] = bit_cast<SyscallHandler>((void*)sys_getcwd);
    syscallTable[SYS_chdir] = bit_cast<SyscallHandler>((void*)sys_chdir);
    syscallTable[SYS_wait_pid] = bit_cast<SyscallHandler>((void*)sys_wait_pid);
    syscallTable[SYS_socket] = bit_cast<SyscallHandler>((void*)sys_socket);
    syscallTable[SYS_connect] = bit_cast<SyscallHandler>((void*)sys_connect);
    syscallTable[SYS_send] = bit_cast<SyscallHandler>((void*)sys_send);
    syscallTable[SYS_recv] = bit_cast<SyscallHandler>((void*)sys_recv);

    println("syscall: init complete");
}
