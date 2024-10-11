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
        return 0;
    } else {
        println("I am the parent process");
        waitpid(childPid, nullptr, 0);
        println("Child process has exited");
    }

    return 0;
}
