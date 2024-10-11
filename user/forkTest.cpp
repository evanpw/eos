#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "estd/print.h"

int main(int argc, char* argv[]) {
    pid_t childPid = fork();
    if (childPid < 0) {
        println("{}: error forking: {}", argv[0], errno);
        return 1;
    } else if (childPid == 0) {
        println("I am the child process. Sleeping for 5 seconds...");
        sleep(500);
        println("done sleeping");

        const char* argv[] = {"/etc/version.txt", nullptr};
        int result = execvp("/bin/cat", argv);
        if (result < 0) {
            println("error executing ls: {}", errno);
            return 1;
        }
    } else {
        println("I am the parent process");
        waitpid(childPid, nullptr, 0);
        println("Child process has exited");
    }

    return 0;
}
