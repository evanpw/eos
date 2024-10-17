#pragma once

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef char cc_t;
typedef uint32_t speed_t;
typedef uint32_t tcflag_t;

// Special characters
enum : cc_t {
    VEOF,
    VEOL,
    VERASE,
    VINTR,
    VKILL,
    VMIN,
    VQUIT,
    VSTART,
    VSTOP,
    VSUSP,
    VTIME,
    VWERASE,
    VLNEXT,
    NCCS,
};

// Input modes
enum : tcflag_t {
    BRKINT = 1 << 0,
    ICRNL = 1 << 1,
    IGNBRK = 1 << 2,
    IGNCR = 1 << 3,
    IGNPAR = 1 << 4,
    INLCR = 1 << 5,
    INPCK = 1 << 6,
    ISTRIP = 1 << 7,
    IXOFF = 1 << 8,
    IXON = 1 << 9,
    PARMRK = 1 << 10,
};

// Output modes
enum : tcflag_t {
    OPOST = 1 << 0,
    ONLCR = 1 << 1,
    OCRNL = 1 << 2,
    ONOCR = 1 << 3,
    ONLRET = 1 << 4,
    OFILL = 1 << 5,
    NL0 = 1 << 6,
    NL1 = 1 << 7,
    NLDLY = NL0 | NL1,
    CR0 = 1 << 8,
    CR1 = 1 << 9,
    CR2 = 1 << 10,
    CR3 = 1 << 11,
    CRDLY = CR0 | CR1 | CR2 | CR3,
    TAB0 = 1 << 12,
    TAB1 = 1 << 13,
    TAB2 = 1 << 14,
    TAB3 = 1 << 15,
    TABDLY = TAB0 | TAB1 | TAB2 | TAB3,
    BS0 = 1 << 16,
    BS1 = 1 << 17,
    BSDLY = BS0 | BS1,
    VT0 = 1 << 18,
    VT1 = 1 << 19,
    VTDLY = VT0 | VT1,
    FF0 = 1 << 20,
    FF1 = 1 << 21,
    FFDLY = FF0 | FF1,
};

// Baud rate selection
enum : speed_t {
    B0 = 0,
    B50 = 50,
    B75 = 75,
    B110 = 110,
    B134 = 134,
    B150 = 150,
    B200 = 200,
    B300 = 300,
    B600 = 600,
    B1200 = 1200,
    B1800 = 1800,
    B2400 = 2400,
    B4800 = 4800,
    B9600 = 9600,
    B19200 = 19200,
    B38400 = 38400,
};

// control modes
enum : tcflag_t {
    CS5 = 1 << 0,
    CS6 = 1 << 1,
    CS7 = 1 << 2,
    CS8 = 1 << 3,
    CSIZE = CS5 | CS6 | CS7 | CS8,
    CSTOPB = 1 << 4,
    CREAD = 1 << 5,
    PARENB = 1 << 6,
    PARODD = 1 << 7,
    HUPCL = 1 << 8,
    CLOCAL = 1 << 9,
};

// local flags
enum : tcflag_t {
    ECHO = 1 << 0,
    ECHOE = 1 << 1,
    ECHOK = 1 << 2,
    ECHONL = 1 << 3,
    ICANON = 1 << 4,
    IEXTEN = 1 << 5,
    ISIG = 1 << 6,
    NOFLSH = 1 << 7,
    TOSTOP = 1 << 8,
    ECHOCTL = 1 << 9,
};

struct termios {
    tcflag_t c_iflag;  // input modes
    tcflag_t c_oflag;  // output modes
    tcflag_t c_cflag;  // control modes
    tcflag_t c_lflag;  // local modes
    cc_t c_cc[NCCS];   // control characters
};

struct winsize {
    uint16_t ws_row;
    uint16_t ws_col;
};

// Attribute selection for tcsetattr
enum {
    TCSANOW,
    TCSADRAIN,
    TCSAFLUSH,
};

// Line control
enum {
    // For tcflush
    TCIFLUSH,
    TCIOFLUSH,
    TCOFLUSH,

    // For tcflow
    TCIOFF,
    TCION,
    TCOOFF,
    TCOON,
};

speed_t cfgetispeed(const struct termios* termios_p);
speed_t cfgetospeed(const struct termios* termios_p);
int cfsetispeed(struct termios* termios_p, speed_t speed);
int cfsetospeed(struct termios* termios_p, speed_t speed);

int tcdrain(int fd);
int tcflow(int fd, int action);
int tcflush(int fd, int queue_selector);

pid_t tcgetsid(int fd);
int tcsendbreak(int fd, int duration);

int tcgetattr(int fd, struct termios* termios_p);
int tcsetattr(int fd, int optional_actions, const struct termios* termios_p);

int tcgetwinsize(int fd, struct winsize* winsize_p);
int tcsetwinsize(int fd, const struct winsize* winsize_p);

// Not in POSIX
void cfmakeraw(struct termios* termios_p);

#ifdef __cplusplus
}
#endif
