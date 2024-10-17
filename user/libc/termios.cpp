#include "termios.h"

#include <asm/termbits.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "syscall.h"

speed_t cfgetispeed(const termios* termios_p) { return B38400; }

speed_t cfgetospeed(const termios* termios_p) { return B38400; }

int cfsetispeed(termios* termios_p, speed_t speed) {
    if (speed == B38400) {
        return 0;
    }

    errno = EINVAL;
    return -1;
}

int cfsetospeed(termios* termios_p, speed_t speed) {
    if (speed == B38400) {
        return 0;
    }

    errno = EINVAL;
    return -1;
}

int tcdrain(int fd) {
    errno = ENOSYS;
    return -1;
}

int tcflow(int fd, int action) {
    errno = ENOSYS;
    return -1;
}

int tcflush(int fd, int queue_selector) {
    errno = ENOSYS;
    return -1;
}

pid_t tcgetsid(int fd) {
    errno = ENOSYS;
    return -1;
}

int tcsendbreak(int fd, int duration) {
    errno = ENOSYS;
    return -1;
}

int tcgetattr(int fd, termios* termios_p) { return try_ioctl(fd, TCGETS, termios_p); }

int tcsetattr(int fd, int optional_actions, const termios* termios_p) {
    return try_ioctl(fd, TCSETS, termios_p);
}

int tcgetwinsize(int fd, winsize* winsize_p) {
    return try_ioctl(fd, TIOCGWINSZ, winsize_p);
}

int tcsetwinsize(int fd, const winsize* winsize_p) {
    return try_ioctl(fd, TIOCSWINSZ, winsize_p);
}

void cfmakeraw(termios* termios_p) {
    termios_p->c_iflag &=
        ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;
}
