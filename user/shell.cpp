#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "estd/optional.h"
#include "estd/print.h"
#include "estd/shared_ptr.h"
#include "estd/string.h"
#include "estd/vector.h"

struct FdInstructions {
    // Duplicate these file descriptors to the standard streams (if not -1)
    int newStdin = -1;
    int newStdout = -1;
    int newStderr = -1;

    // Close these file descriptors after fork (and after dup-ing)
    estd::vector<int> closeFds;

    // True if there are any redirections, in which case the command must be run in
    // a subprocess
    operator bool() const { return newStdin != -1 || newStdout != -1 || newStderr != -1; }

    void execute() const {
        if (newStdin != -1) {
            dup2(newStdin, STDIN_FILENO);
        }
        if (newStdout != -1) {
            dup2(newStdout, STDOUT_FILENO);
        }
        if (newStderr != -1) {
            dup2(newStderr, STDERR_FILENO);
        }

        for (int fd : closeFds) {
            close(fd);
        }
    }
};

struct ShellCommand {
    virtual ~ShellCommand() = default;

    // This function runs in the child subprocess (if applicable) and actually performs
    // the command logic or execs the relevant program
    virtual int runImpl(const estd::vector<estd::string>& args) const = 0;

    // This function launches the command (in a subprocess or in the current process)
    // without any redirections, and waits for it to complete before returning.
    virtual int run(const estd::vector<estd::string>& args) const;

    // This function starts the command in a new process, but does not wait for it to
    // finish.
    virtual pid_t start(const estd::vector<estd::string>& args,
                        const FdInstructions& fdInst) const = 0;
};

struct ExternalCommand : public ShellCommand {
    ExternalCommand(const estd::string& name, const estd::string& path)
    : name(name), path(path) {}

    estd::string name;  // name of the command as entered by the user
    estd::string path;  // full path to the program on disk

    int runImpl(const estd::vector<estd::string>& args) const override {
        const char** argv = new const char*[args.size() + 1];
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = args[i].c_str();
        }
        argv[args.size()] = nullptr;

        execvp(path.c_str(), argv);  // should not return

        // This is reached only if execvp fails
        println("{}: error executing: {}", name, errno);
        return 1;
    }

    int run(const estd::vector<estd::string>& args) const override {
        if (pid_t childPid = fork(); childPid == 0) {
            // Child process
            int exitCode = runImpl(args);
            exit(exitCode);
        } else if (childPid > 0) {
            // Parent process
            waitpid(childPid, nullptr, 0);
            return 0;  // TODO: return exit code of the child process
        } else {
            println("fork failed: {}", errno);
            return 1;
        }
    }

    pid_t start(const estd::vector<estd::string>& args,
                const FdInstructions& fdInst) const override {
        if (pid_t childPid = fork(); childPid == 0) {
            // Child process
            fdInst.execute();
            int exitCode = runImpl(args);
            exit(exitCode);
        } else if (childPid > 0) {
            // Parent process
            return childPid;
        } else {
            println("fork failed: {}", errno);
            return -1;
        }
    }
};

struct BuiltinCommand : public ShellCommand {
    virtual estd::string name() const = 0;

    int run(const estd::vector<estd::string>& args) const override {
        return runImpl(args);
    }

    pid_t start(const estd::vector<estd::string>& args,
                const FdInstructions& fdInst) const override {
        if (pid_t childPid = fork(); childPid == 0) {
            // Child process
            fdInst.execute();
            int exitCode = runImpl(args);
            exit(exitCode);
        } else if (childPid > 0) {
            // Parent process
            return childPid;
        } else {
            println("fork failed: {}", errno);
            return -1;
        }
    }
};

struct Builtin$clear : public BuiltinCommand {
    estd::string name() const override { return "clear"; }

    int runImpl(const estd::vector<estd::string>& args) const override {
        print("\033[J");
        return 0;
    }
};

struct Builtin$spam : public BuiltinCommand {
    estd::string name() const override { return "spam"; }

    int runImpl(const estd::vector<estd::string>& args) const override {
        launch("/bin/spam", nullptr);
        return 0;
    }
};

struct Builtin$ls : public BuiltinCommand {
    estd::string name() const override { return "ls"; }

    int runImpl(const estd::vector<estd::string>& args) const override {
        // ls with no arguments lists the current directory
        const char* path = args.empty() ? "." : args[0].c_str();

        DIR* dir = opendir(path);
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            println("{}", entry->d_name);
        }

        return 0;
    }
};

struct Builtin$cd : public BuiltinCommand {
    estd::string name() const override { return "cd"; }

    int runImpl(const estd::vector<estd::string>& args) const override {
        // cd with no arguments goes to the root directory
        const char* path = args.empty() ? "/" : args[0].c_str();

        if (chdir(path) != 0) {
            println("cd: no such file or directory");
            return 1;
        }

        return 0;
    }
};

struct Builtin$pwd : public BuiltinCommand {
    estd::string name() const override { return "pwd"; }

    int runImpl(const estd::vector<estd::string>& args) const override {
        char buffer[64];
        if (!getcwd(buffer, 64)) {
            println("pwd: no such file or directory");
            return 1;
        }

        println(buffer);
        return 0;
    }
};

struct Builtin$echo : public BuiltinCommand {
    estd::string name() const override { return "echo"; }

    int runImpl(const estd::vector<estd::string>& args) const override {
        const char* msg = args.empty() ? "" : args[0].c_str();
        println(msg);
        return 0;
    }
};

class CommandLibrary {
public:
    CommandLibrary();
    estd::shared_ptr<const ShellCommand> lookup(const estd::string& name) const;

private:
    estd::vector<estd::shared_ptr<const BuiltinCommand>> _builtins;
};

CommandLibrary::CommandLibrary() {
    _builtins.emplace_back(new Builtin$clear());
    _builtins.emplace_back(new Builtin$spam());
    _builtins.emplace_back(new Builtin$ls());
    _builtins.emplace_back(new Builtin$cd());
    _builtins.emplace_back(new Builtin$pwd());
    _builtins.emplace_back(new Builtin$echo());
}

estd::shared_ptr<const ShellCommand> CommandLibrary::lookup(
    const estd::string& cmd) const {
    for (auto&& builtin : _builtins) {
        if (builtin->name() == cmd) {
            return builtin;
        }
    }

    // If not builtin, then try to look it up as a program on disk
    DIR* dir = opendir("/bin");
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, cmd.c_str()) == 0) {
            return estd::make_shared<ExternalCommand>(cmd, "/bin/" + cmd);
        }
    }

    return nullptr;
}

struct PipelineComponent {
    estd::shared_ptr<const ShellCommand> command;
    estd::vector<estd::string> args;

    int stdinFd = -1;
    int stdoutFd = -1;
    int stderrFd = -1;

    pid_t start(const estd::vector<int>& fds);
};

pid_t PipelineComponent::start(const estd::vector<int>& fds) {
    FdInstructions fdInst = {stdinFd, stdoutFd, stderrFd, fds};
    return command->start(args, fdInst);
}

struct Pipeline {
    estd::vector<PipelineComponent> commands;
    estd::vector<int> fds;

    Pipeline() = default;
    ~Pipeline() {
        for (int fd : fds) {
            close(fd);
        }
    }

    // Moveable, not copyable
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = default;
    Pipeline& operator=(Pipeline&&) = default;

    static estd::optional<Pipeline> parse(const CommandLibrary& library,
                                          const char* cmdline);
    int run();
};

estd::optional<Pipeline> Pipeline::parse(const CommandLibrary& library,
                                         const char* cmdline) {
    const char* p = cmdline;

    Pipeline pipeline;
    int nextStdin = -1;
    while (*p) {
        PipelineComponent command;

        // Skip any number of spaces
        while (*p == ' ') ++p;

        // Parse the command name (up to the first space, pipe, or null)
        const char* start = p;
        while (*p && *p != ' ' && *p != '|') ++p;
        estd::string cmd = estd::string(start, p - start);

        command.command = library.lookup(cmd);
        if (!command.command) {
            println("{}: command not found", cmd);
            return {};
        }

        // Parse the arguments (up to the next pipe or null)
        while (true) {
            // Skip any number of spaces
            while (*p == ' ') ++p;

            // A pipe or null ends the argument list
            if (*p == '\0' || *p == '|') break;

            // The argument continues until a pipe, null, or space
            start = p;
            while (*p && *p != ' ' && *p != '|') ++p;
            command.args.push_back(estd::string(start, p - start));
        }

        ASSERT(*p == '\0' || *p == '|');

        command.stdinFd = nextStdin;
        if (*p == '|') {
            ++p;

            int pipeFds[2];
            if (pipe(pipeFds) == -1) {
                println("pipe failed: {}", errno);
                return {};
            }

            pipeline.fds.push_back(pipeFds[0]);
            pipeline.fds.push_back(pipeFds[1]);

            command.stdoutFd = pipeFds[1];
            nextStdin = pipeFds[0];
        }

        pipeline.commands.push_back(estd::move(command));
    }

    return pipeline;
}

int Pipeline::run() {
    if (commands.size() == 1 && fds.empty()) {
        // If there is only one command and no redirections, run it in the current process
        return commands[0].command->run(commands[0].args);
    }

    estd::vector<pid_t> pids;
    for (auto&& command : commands) {
        pid_t childPid = command.start(fds);
        if (childPid == -1) {
            println("pipeline failed");
            return 1;
        }

        pids.push_back(childPid);
    }

    // Close all the pipe file descriptors so that each side knows when the other is done
    for (int fd : fds) {
        close(fd);
    }
    fds.clear();

    // Wait for all the child processes to finish
    for (pid_t pid : pids) {
        waitpid(pid, nullptr, 0);
    }

    return 0;
}

int main() {
    char buffer[64];

    int fd = open("/etc/version.txt", 0);
    size_t bytesRead;
    while ((bytesRead = read(fd, buffer, 63)) > 0) {
        buffer[bytesRead] = '\0';
        print("{}", buffer);
    }
    close(fd);

    CommandLibrary library;
    while (true) {
        // Include the cwd in the prompt
        if (getcwd(buffer, 64)) {
            print("\033[36m{}\033[97m$\033[0m ", buffer);
        } else {
            print("$ ");
        }
        fflush(stdout);

        bytesRead = read(STDIN_FILENO, buffer, 63);
        if (bytesRead == 0) {
            continue;
        }

        // Trim trailing newline and/or add null terminator
        if (bytesRead > 0 && buffer[bytesRead - 1] == '\n') {
            buffer[bytesRead - 1] = '\0';
        } else {
            buffer[bytesRead] = '\0';
        }

        if (*buffer == '\0') {
            continue;
        }

        if (auto pipeline = Pipeline::parse(library, buffer)) {
            pipeline->run();
        }
    }

    return 0;
}
