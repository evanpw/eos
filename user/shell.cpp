#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "estd/print.h"

void clear() { print("\033[J"); }

void spam() { launch("/bin/spam.bin"); }

void ls(const char* path) {
    DIR* dir = opendir(path);
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        println("{}", entry->d_name);
    }
}

// Just split at the first space and return a pointer to the arguments
const char* parseCommand(char* buffer) {
    char* space = const_cast<char*>(strchr(buffer, ' '));
    if (!space) {
        return buffer + strlen(buffer);
    }

    *space = '\0';
    return space + 1;
}

int main() {
    char buffer[64];

    int fd = open("/etc/version.txt", 0);
    ssize_t bytesRead = read(fd, buffer, 63);
    buffer[bytesRead] = '\0';
    print("\033[31;42m{}\033[m", buffer);
    close(fd);

    println("pid={}", getpid());

    while (true) {
        // Include the cwd in the prompt
        if (getcwd(buffer, 64)) {
            print("[eos:{}]$ ", buffer);
        } else {
            print("$ ");
        }

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

        const char* cmd = buffer;
        const char* args = parseCommand(buffer);

        if (strcmp(cmd, "clear") == 0) {
            clear();
        } else if (strcmp(cmd, "spam") == 0) {
            spam();
        } else if (strcmp(cmd, "ls") == 0) {
            // ls with no arguments lists the current directory
            if (*args == '\0') {
                args = ".";
            }

            ls(args);
        } else if (strcmp(cmd, "cd") == 0) {
            // cd with no arguments goes to the root directory
            if (*args == '\0') {
                args = "/";
            }

            if (chdir(args) != 0) {
                println("cd: no such file or directory");
            }
        } else if (strcmp(cmd, "pwd") == 0) {
            if (getcwd(buffer, 64)) {
                println(buffer);
            } else {
                println("pwd: no such file or directory");
            }
        } else {
            println("no such command: {}", buffer);
        }
    }

    return 0;
}
