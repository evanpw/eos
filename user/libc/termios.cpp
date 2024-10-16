#include "termios.h"

#include <asm/termbits.h>
#include <errno.h>
#include <sys/ioctl.h>

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

int tcgetattr(int fd, termios* termios_p) {
    int result = ioctl(fd, TCGETS, termios_p);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

int tcsetattr(int fd, int optional_actions, const termios* termios_p) {
    int result = ioctl(fd, TCSETS, (void*)termios_p);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

void cfmakeraw(termios* termios_p) {
    termios_p->c_iflag &=
        ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;
}
