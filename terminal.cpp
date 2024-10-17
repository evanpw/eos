#include "terminal.h"

#include <asm/termbits.h>
#include <string.h>

#include "estd/vector.h"
#include "klibc.h"
#include "system.h"

Terminal::Terminal(KeyboardDevice& keyboard, Screen& screen)
: _keyboard(keyboard), _screen(screen), _inputBlocker(new Blocker) {
    _keyboard.addListener(this);

    _settings.c_iflag = ICRNL | IXON;
    _settings.c_oflag = OPOST | ONLCR | NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
    _settings.c_cflag = CS8 | CREAD;
    _settings.c_lflag = ECHO | ECHOE | ECHOK | ICANON | IEXTEN | ISIG | ECHOCTL;
    _settings.c_cc[VEOF] = '\x04';  // Ctrl-D
    _settings.c_cc[VEOL] = 0;
    _settings.c_cc[VERASE] = '\x7F';   // Ctrl-? or Backspace
    _settings.c_cc[VINTR] = '\x03';    // Ctrl-C
    _settings.c_cc[VKILL] = '\x15';    // Ctrl-U
    _settings.c_cc[VQUIT] = '\x1C';    // Ctrl-Backslash
    _settings.c_cc[VSTART] = '\x11';   // Ctrl-Q
    _settings.c_cc[VSTOP] = '\x13';    // Ctrl-S
    _settings.c_cc[VSUSP] = '\x1A';    // Ctrl-Z
    _settings.c_cc[VWERASE] = '\x17';  // Ctrl-W
    _settings.c_cc[VLNEXT] = '\x16';   // Ctrl-V
    _settings.c_cc[VMIN] = 1;
    _settings.c_cc[VTIME] = 0;
}

static const char* parseKeyCode(KeyCode keyCode, bool shift, bool ctrl) {
    auto selectFrom = [&](const char* normal, const char* ifShift, const char* ifCtrl,
                          const char* ifShiftCtrl) {
        if (!shift && !ctrl) {
            return normal;
        } else if (shift && !ctrl) {
            return ifShift;
        } else if (!shift && ctrl) {
            return ifCtrl;
        } else {
            return ifShiftCtrl;
        }
    };

    switch (keyCode) {
        // These keys have no ASCII or escape-code equivalent
        case KeyCode::Unknown:
        case KeyCode::CapsLock:
        case KeyCode::LCtrl:
        case KeyCode::LShift:
        case KeyCode::RShift:
        case KeyCode::LAlt:
        case KeyCode::NumLock:
        case KeyCode::ScrollLock:
        case KeyCode::RCtrl:
        case KeyCode::RAlt:
        case KeyCode::Menu:
        default:
            return nullptr;

        // Regular number keys
        case KeyCode::One:
            return selectFrom("1", "!", "1", nullptr);
        case KeyCode::Two:
            return selectFrom("2", "@", "\0", nullptr);
        case KeyCode::Three:
            return selectFrom("3", "#", "\x1B", nullptr);
        case KeyCode::Four:
            return selectFrom("4", "$", "\x1C", nullptr);
        case KeyCode::Five:
            return selectFrom("5", "%", "\x1D", nullptr);
        case KeyCode::Six:
            return selectFrom("6", "^", "\x1E", nullptr);
        case KeyCode::Seven:
            return selectFrom("7", "&", "\x1F", nullptr);
        case KeyCode::Eight:
            return selectFrom("8", "*", "\x7F", "\x7F");
        case KeyCode::Nine:
            return selectFrom("9", "(", "9", "9");
        case KeyCode::Zero:
            return selectFrom("0", ")", nullptr, nullptr);

        // Letter keys
        case KeyCode::A:
            return selectFrom("a", "A", "\x01", "\x01");
        case KeyCode::B:
            return selectFrom("b", "B", "\x02", "\x02");
        case KeyCode::C:
            return selectFrom("c", "C", "\x03", "\x03");
        case KeyCode::D:
            return selectFrom("d", "D", "\x04", "\x04");
        case KeyCode::E:
            return selectFrom("e", "E", "\x05", "\x05");
        case KeyCode::F:
            return selectFrom("f", "F", "\x06", "\x06");
        case KeyCode::G:
            return selectFrom("g", "G", "\x07", "\x07");
        case KeyCode::H:
            return selectFrom("h", "H", "\x08", "\x08");
        case KeyCode::I:
            return selectFrom("i", "I", "\x09", "\x09");
        case KeyCode::J:
            return selectFrom("j", "J", "\x0A", "\x0A");
        case KeyCode::K:
            return selectFrom("k", "K", "\x0B", "\x0B");
        case KeyCode::L:
            return selectFrom("l", "L", "\x0C", "\x0C");
        case KeyCode::M:
            return selectFrom("m", "M", "\x0D", "\x0D");
        case KeyCode::N:
            return selectFrom("n", "N", "\x0E", "\x0E");
        case KeyCode::O:
            return selectFrom("o", "O", "\x0F", "\x0F");
        case KeyCode::P:
            return selectFrom("p", "P", "\x10", "\x10");
        case KeyCode::Q:
            return selectFrom("q", "Q", "\x11", "\x11");
        case KeyCode::R:
            return selectFrom("r", "R", "\x12", "\x12");
        case KeyCode::S:
            return selectFrom("s", "S", "\x13", "\x13");
        case KeyCode::T:
            return selectFrom("t", "T", "\x14", "\x14");
        case KeyCode::U:
            return selectFrom("u", "U", "\x15", "\x15");
        case KeyCode::V:
            return selectFrom("v", "V", "\x16", "\x16");
        case KeyCode::W:
            return selectFrom("w", "W", "\x17", "\x17");
        case KeyCode::X:
            return selectFrom("x", "X", "\x18", "\x18");
        case KeyCode::Y:
            return selectFrom("y", "Y", "\x19", "\x19");
        case KeyCode::Z:
            return selectFrom("z", "Z", "\x1A", "\x1A");

        // Symbol keys
        case KeyCode::LBracket:
            return selectFrom("[", "{", "\x1B", "\x1B");
        case KeyCode::RBracket:
            return selectFrom("]", "}", "\x1D", "\x1D");
        case KeyCode::Semicolon:
            return selectFrom(";", ":", ";", ":");
        case KeyCode::Apostrophe:
            return selectFrom("'", "\"", "'", "\"");
        case KeyCode::Backtick:
            return selectFrom("`", "~", "\0", "\x1E");
        case KeyCode::Backslash:
            return selectFrom("\\", "|", "\x1C", "\x1C");
        case KeyCode::Comma:
            return selectFrom(",", "<", nullptr, nullptr);
        case KeyCode::Period:
            return selectFrom(".", ">", ".", nullptr);
        case KeyCode::Slash:
            return selectFrom("/", "?", "\x1F", "\x7F");
        case KeyCode::Minus:
            return selectFrom("-", "_", nullptr, "\x1F");
        case KeyCode::Equals:
            return selectFrom("=", "+", nullptr, "+");
        case KeyCode::Space:
            return selectFrom(" ", " ", "\0", nullptr);

        // Navigation keys
        case KeyCode::Backspace:
            return selectFrom("\x7F", "\x7F", "\x08", "\x08");
        case KeyCode::Tab:
            return selectFrom("\t", "\x1B[Z", nullptr, nullptr);
        case KeyCode::Enter:
            return selectFrom("\r", "\r", "\n", "\n");
        case KeyCode::Up:
            return selectFrom("\x1B[A", "\x1B[1;2A", "\x1B[1;5A", "\x1B[1;6A");
        case KeyCode::Down:
            return selectFrom("\x1B[B", "\x1B[1;2B", "\x1B[1;5B", "\x1B[1;6B");
        case KeyCode::Right:
            return selectFrom("\x1B[C", "\x1B[1;2C", "\x1B[1;5C", "\x1B[1;6C");
        case KeyCode::Left:
            return selectFrom("\x1B[D", "\x1B[1;2D", "\x1B[1;5D", "\x1B[1;6D");
        case KeyCode::Home:
            return selectFrom("\x1B[1~", "\x1B[1;2H", "\x1B[1;5H", nullptr);
        case KeyCode::End:
            return selectFrom("\x1B[4~", "\x1B[1;2F", "\x1B[1;5F", nullptr);
        case KeyCode::PageUp:
            return selectFrom("\x1B[5~", "\x1B[5;2~", "\x1B[5;5~", nullptr);
        case KeyCode::PageDown:
            return selectFrom("\x1B[6~", "\x1B[6;2~", "\x1B[6;5~", nullptr);

        // Keypad keys
        // TODO: handle NumLock (this assumes NumLock is off)
        case KeyCode::Keypad1:
            // Equivalent to End
            return selectFrom("\x1B[4~", "\x1B[1;2F", "\x1B[1;5F", nullptr);
        case KeyCode::Keypad2:
            // Equivalent to Down
            return selectFrom("\x1B[B", "\x1B[1;2B", "\x1B[1;5B", "\x1B[1;6B");
        case KeyCode::Keypad3:
            // Equivalent to PageDown
            return selectFrom("\x1B[6~", "\x1B[6;2~", "\x1B[6;5~", nullptr);
        case KeyCode::Keypad4:
            // Equivalent to Left
            return selectFrom("\x1B[D", "\x1B[1;2D", "\x1B[1;5D", "\x1B[1;6D");
        case KeyCode::Keypad5:
            return selectFrom("\x1B[OE", "\x1B[1;2E", "\x1B[1;5E", "\x1B[1;6E");
        case KeyCode::Keypad6:
            // Equivalent to Right
            return selectFrom("\x1B[C", "\x1B[1;2C", "\x1B[1;5C", "\x1B[1;6C");
        case KeyCode::Keypad7:
            // Equivalent to Home
            return selectFrom("\x1B[1~", "\x1B[1;2H", "\x1B[1;5H", nullptr);
        case KeyCode::Keypad8:
            // Equivalent to Up
            return selectFrom("\x1B[A", "\x1B[1;2A", "\x1B[1;5A", "\x1B[1;6A");
        case KeyCode::Keypad9:
            // Equivalent to PageUp
            return selectFrom("\x1B[5~", "\x1B[5;2~", "\x1B[5;5~", nullptr);
        case KeyCode::Keypad0:
            // Equivalent to Insert
            return selectFrom("\x1B[2~", nullptr, "\x1B[2;5~", "\x1B[2;6~");
        case KeyCode::KeypadAsterisk:
            return "*";
        case KeyCode::KeypadMinus:
            return "-";
        case KeyCode::KeypadPlus:
            return "+";
        case KeyCode::KeypadSlash:
            return selectFrom("/", "/", "\x1F", "\x1F");
        case KeyCode::KeypadDot:
            // Equivalent to Delete
            return selectFrom("\x1B[3~", "\x1B[3;2~", "\x1B[3;5~", "\x1B[3;6~");
        case KeyCode::KeypadEnter:
            // Equivalent to Enter
            return selectFrom("\r", "\r", "\n", "\n");

        // Function keys
        case KeyCode::F1:
            return selectFrom("\x1B[OP", "\x1B[1;2P", "\x1B[1;5P", "\x1B[1;6P");
        case KeyCode::F2:
            return selectFrom("\x1B[OQ", "\x1B[1;2Q", "\x1B[1;5Q", "\x1B[1;6Q");
        case KeyCode::F3:
            return selectFrom("\x1B[OR", "\x1B[1;2R", "\x1B[1;5R", "\x1B[1;6R");
        case KeyCode::F4:
            return selectFrom("\x1B[OS", "\x1B[1;2S", "\x1B[1;5S", "\x1B[1;6S");
        case KeyCode::F5:
            return selectFrom("\x1B[15~", "\x1B[15;2~", "\x1B[15;5~", "\x1B[15;6~");
        case KeyCode::F6:
            return selectFrom("\x1B[17~", "\x1B[17;2~", "\x1B[17;5~", "\x1B[17;6~");
        case KeyCode::F7:
            return selectFrom("\x1B[18~", "\x1B[18;2~", "\x1B[18;5~", "\x1B[18;6~");
        case KeyCode::F8:
            return selectFrom("\x1B[19~", "\x1B[19;2~", "\x1B[19;5~", "\x1B[19;6~");
        case KeyCode::F9:
            return selectFrom("\x1B[20~", "\x1B[20;2~", "\x1B[20;5~", "\x1B[20;6~");
        case KeyCode::F10:
            return selectFrom("\x1B[21~", "\x1B[21;2~", "\x1B[21;5~", "\x1B[21;6~");
        case KeyCode::F11:
            return selectFrom("\x1B[23~", "\x1B[23;2~", "\x1B[23;5~", "\x1B[23;6~");
        case KeyCode::F12:
            return selectFrom("\x1B[24~", "\x1B[24;2~", "\x1B[24;5~", "\x1B[24;6~");

        // Other special keys
        case KeyCode::Insert:
            return selectFrom("\x1B[2~", nullptr, "\x1B[2;5~", "\x1B[2;6~");
        case KeyCode::Delete:
            return selectFrom("\x1B[3~", "\x1B[3;2~", "\x1B[3;5~", "\x1B[3;6~");
        case KeyCode::Escape:
            return selectFrom("\x1B", "\x1B", nullptr, nullptr);
    }
}

bool isControlChar(char c) {
    // Exclude tab, newline, carriage return, and backspace since they have special roles
    return ((c < 0x20 || c == 0x7F) && c != '\t' && c != '\n' && c != '\r' &&
            c != '\x7F');
}

void Terminal::onKeyEvent(const KeyboardEvent& event) {
    SpinlockLocker locker(_lock);

    // println("Terminal::onKeyEvent: keycode={:X}, key={}, pressed={}",
    //        (uint8_t)event.key, keyCodeToString(event.key), event.pressed);

    if (event.pressed) {
        // TODO: pass modifiers in with the event
        bool shift =
            _keyboard.isPressed(KeyCode::LShift) || _keyboard.isPressed(KeyCode::RShift);
        bool ctrl =
            _keyboard.isPressed(KeyCode::LCtrl) || _keyboard.isPressed(KeyCode::RCtrl);

        const char* str = parseKeyCode(event.key, shift, ctrl);
        if (str) {
            for (size_t i = 0; str[i] != '\0' || i == 0; i++) {
                char c = str[i];

                // ICRNL: convert carriage return to newline on input
                if (_settings.c_iflag & ICRNL && c == '\r') {
                    c = '\n';
                }

                if (!handleInput(c)) {
                    // Discard the character if it couldn't be handled
                    continue;
                }

                if (_settings.c_lflag & ECHO) {
                    if (isControlChar(c) && (_settings.c_lflag & ECHOCTL)) {
                        // ECHOCTL: echo control characters as ^X
                        handleOutput('^');
                        handleOutput(c ^ 0x40);
                    } else {
                        handleOutput(c);
                    }
                }
            }
        }
    }
}

bool Terminal::handleInput(char c) {
    if (_settings.c_lflag & ICANON) {
        return handleInputCanonical(c);
    } else {
        return handleInputRaw(c);
    }
}

bool Terminal::handleInputCanonical(char c) {
    // Backspace deletes rather than appends a character to the input buffer
    if (c == '\x7F') {
        // TODO: don't delete past the beginning of the line or EOF
        if (!_inputBuffer.empty()) {
            char dc = _inputBuffer.popBack();
            if (dc == '\n') {
                ASSERT(_inputLines > 0);
                --_inputLines;
            }

            return true;
        }

        // If the input buffer is empty, don't echo the backspace
        return false;
    }

    // If the input buffer is completely full, discard any further input
    if (_inputBuffer.full()) return false;

    // If the input buffer is nearly full (only one spot left), allow a newline
    // but discard any other input
    if (_inputBuffer.almostFull() && c != '\n') return false;

    _inputBuffer.push(c);

    if (c == '\n') {
        _inputLines++;
        sys.scheduler().wakeThreads(_inputBlocker);
    }

    return true;
}

bool Terminal::handleInputRaw(char c) {
    // If the input buffer is nearly full, discard any further input
    if (_inputBuffer.full() || _inputBuffer.almostFull()) return false;

    _inputBuffer.push(c);

    if (_inputBuffer.size() == 1) {
        sys.scheduler().wakeThreads(_inputBlocker);
    }

    return true;
}

void Terminal::handleOutput(char c) {
    // Start or continuance of an escape sequence
    if (!_outputBuffer.empty()) {
        // If the escape sequence is too long, it's invalid
        if (_outputBuffer.full()) {
            _outputBuffer.clear();
            return;
        }

        _outputBuffer.pushBack(c);
        handleEscapeSequence();
        return;
    } else if (c == '\033') {
        _outputBuffer.pushBack(c);
        return;
    }

    // Ordinary printable character
    putchar(c);
}

void Terminal::handleEscapeSequence() {
    if (parseEscapeSequence()) {
        _outputBuffer.clear();
    }
}

bool Terminal::parseEscapeSequence() {
    ASSERT(_outputBuffer.size() >= 2 && _outputBuffer[0] == '\033');

    size_t idx = 1;
    if (_outputBuffer[idx] == '[') {
        return parseCSI();
    } else {
        // Bad escape sequence, ignore it and clear the buffer
        return true;
    }
}

bool Terminal::parseCSI() {
    ASSERT(_outputBuffer.size() >= 2 && _outputBuffer[0] == '\033' &&
           _outputBuffer[1] == '[');

    size_t idx = 2;
    if (idx == _outputBuffer.size()) return false;
    char c = _outputBuffer[idx];

    if (c == '?') {
        return parseDEC();
    }

    // Look for optional numerical arguments
    estd::vector<int> args;
    if (c >= '0' && c <= '9') {
        while (true) {
            int arg = 0;
            while (c >= '0' && c <= '9') {
                // TODO: check for overflow
                arg = (arg * 10) + (c - '0');

                if (++idx == _outputBuffer.size()) return false;
                c = _outputBuffer[idx];
            }

            args.push_back(arg);

            if (c == ';') {
                if (++idx == _outputBuffer.size()) return false;
                c = _outputBuffer[idx];
            } else {
                break;
            }
        }
    }

    switch (c) {
        case 's':
            _savedX = _x;
            _savedY = _y;
            return true;

        case 'u':
            _x = _savedX;
            _y = _savedY;
            _screen.setCursor(_x, _y);
            return true;

        case 'J':
            _screen.clear(_bg);
            _x = 0;
            _y = 0;
            _screen.setCursor(_x, _y);
            return true;

        case 'm': {
            if (args.size() == 0) {
                _fg = Screen::LightGrey;
                _bg = Screen::Black;
            }

            for (int arg : args) {
                if (arg == 0) {
                    _fg = Screen::LightGrey;
                    _bg = Screen::Black;
                } else if (arg == 30) {
                    _fg = Screen::Black;
                } else if (arg == 31) {
                    _fg = Screen::Red;
                } else if (arg == 32) {
                    _fg = Screen::Green;
                } else if (arg == 33) {
                    _fg = Screen::Brown;
                } else if (arg == 34) {
                    _fg = Screen::Blue;
                } else if (arg == 35) {
                    _fg = Screen::Magenta;
                } else if (arg == 36) {
                    _fg = Screen::Cyan;
                } else if (arg == 37) {
                    _fg = Screen::LightGrey;
                } else if (arg == 90) {
                    _fg = Screen::DarkGrey;
                } else if (arg == 91) {
                    _fg = Screen::LightRed;
                } else if (arg == 92) {
                    _fg = Screen::LightGreen;
                } else if (arg == 93) {
                    _fg = Screen::Yellow;
                } else if (arg == 94) {
                    _fg = Screen::LightBlue;
                } else if (arg == 95) {
                    _fg = Screen::LightMagenta;
                } else if (arg == 96) {
                    _fg = Screen::LightCyan;
                } else if (arg == 97) {
                    _fg = Screen::White;
                } else if (arg == 40) {
                    _bg = Screen::Black;
                } else if (arg == 41) {
                    _bg = Screen::Red;
                } else if (arg == 42) {
                    _bg = Screen::Green;
                } else if (arg == 43) {
                    _bg = Screen::Brown;
                } else if (arg == 44) {
                    _bg = Screen::Blue;
                } else if (arg == 45) {
                    _bg = Screen::Magenta;
                } else if (arg == 46) {
                    _bg = Screen::Cyan;
                } else if (arg == 47) {
                    _bg = Screen::LightGrey;
                } else if (arg == 100) {
                    _bg = Screen::DarkGrey;
                } else if (arg == 101) {
                    _bg = Screen::LightRed;
                } else if (arg == 102) {
                    _bg = Screen::LightGreen;
                } else if (arg == 103) {
                    _bg = Screen::Yellow;
                } else if (arg == 104) {
                    _bg = Screen::LightBlue;
                } else if (arg == 105) {
                    _bg = Screen::LightMagenta;
                } else if (arg == 106) {
                    _bg = Screen::LightCyan;
                } else if (arg == 107) {
                    _bg = Screen::White;
                }
            }

            return true;
        }

        case 'H': {
            if (args.size() == 2) {
                _x = min(max(0, args[1] - 1), (int)_screen.width() - 1);
                _y = min(max(0, args[0] - 1), (int)_screen.height() - 1);
                _screen.setCursor(_x, _y);
            }

            return true;
        }

        default:
            // Bad escape sequence, ignore it and clear the buffer
            return true;
    }
}

bool Terminal::parseDEC() {
    ASSERT(_outputBuffer.size() >= 3 && _outputBuffer[0] == '\033' &&
           _outputBuffer[1] == '[' && _outputBuffer[2] == '?');

    size_t idx = 3;
    if (idx == _outputBuffer.size()) return false;
    char c = _outputBuffer[idx];

    // Look for optional numerical arguments
    estd::vector<int> args;
    if (c >= '0' && c <= '9') {
        while (true) {
            int arg = 0;
            while (c >= '0' && c <= '9') {
                // TODO: check for overflow
                arg = (arg * 10) + (c - '0');

                if (++idx == _outputBuffer.size()) return false;
                c = _outputBuffer[idx];
            }

            args.push_back(arg);

            if (c == ';') {
                if (++idx == _outputBuffer.size()) return false;
                c = _outputBuffer[idx];
            } else {
                break;
            }
        }
    }

    if ((c != 'h' && c != 'l') || args.size() != 1) return true;

    if (c == 'h' && args[0] == 1049) {
        //// CSI ? Pm l = Use alternate screen buffer
        // Save normal screen buffer state
        _normalX = _x;
        _normalY = _y;
        _normalFg = _fg;
        _normalBg = _bg;
        _screen.save(_normalScreen);

        // Reset the screen state
        _x = 0;
        _y = 0;
        _fg = Screen::LightGrey;
        _bg = Screen::Black;
        _screen.clear(_bg);
        _screen.setCursor(_x, _y);
    } else if (c == 'l' && args[0] == 1049) {
        //// CSI ? Pm h = Use normal screen buffer
        // Just restore normal screen buffer state, alternate state is discarded
        _x = _normalX;
        _y = _normalY;
        _fg = _normalFg;
        _bg = _normalBg;
        _screen.restore(_normalScreen);
        _screen.setCursor(_x, _y);
    }

    return true;
}

void Terminal::putchar(char c) {
    if (_settings.c_oflag & OPOST) {
        if (c == '\r') {
            if (_settings.c_oflag & OCRNL) {
                // OCRNL: map CR to NL
                newline();
            } else {
                carriageReturn();
            }

            return;
        } else if (c == '\n') {
            if (_settings.c_oflag & ONLCR) {
                // ONLCR: map NL to CR-NL
                carriageReturn();
                newline();
            } else {
                newline();
            }

            return;
        } else if (c == '\x7F') {
            backspace();
            return;
        }
    }

    _screen.putChar(_x, _y, c, _bg, _fg);

    // Advance cursor
    ++_x;
    if (_x == _screen.width()) {
        _x = 0;
        ++_y;
        if (_y == _screen.height()) {
            _screen.scrollUp();
            --_y;
        }
    }

    _screen.setCursor(_x, _y);
}

void Terminal::carriageReturn() {
    _x = 0;
    _screen.setCursor(_x, _y);
}

void Terminal::newline() {
    ++_y;
    if (_y == _screen.height()) {
        _screen.scrollUp();
        --_y;
    }

    _screen.setCursor(_x, _y);
}

void Terminal::backspace() {
    if (_x > 0) {
        --_x;
    } else {
        _x = _screen.width() - 1;

        if (_y > 0) {
            --_y;
        } else {
            return;
        }
    }

    _screen.putChar(_x, _y, ' ', Screen::Black, Screen::LightGrey);
    _screen.setCursor(_x, _y);
}

ssize_t Terminal::read(OpenFileDescription&, void* buffer, size_t count) {
    SpinlockLocker locker(_lock);

    // Block until some input is available (depending on the input mode)
    while (true) {
        if (_settings.c_lflag & ICANON) {
            if (_inputLines > 0) break;
        } else {
            if (_inputBuffer.size() >= (size_t)_settings.c_cc[VMIN]) break;
        }

        sys.scheduler().sleepThread(_inputBlocker, &_lock);
    }

    // TODO: check fd mode
    size_t bytesRead = 0;
    char* dest = static_cast<char*>(buffer);

    if (_settings.c_lflag & ICANON) {
        // Read all complete lines as long as space remains in the buffer
        while (_inputLines > 0 && bytesRead < count) {
            char c = _inputBuffer.pop();

            *dest++ = c;
            ++bytesRead;

            if (c == '\n') {
                ASSERT(_inputLines > 0);
                --_inputLines;
            }
        }
    } else {
        // Read all available characters as long as space remains in the buffer
        while (!_inputBuffer.empty() && bytesRead < count) {
            *dest++ = _inputBuffer.pop();
            ++bytesRead;
        }
    }

    return bytesRead;
}

ssize_t Terminal::write(OpenFileDescription&, const void* buffer, size_t count) {
    SpinlockLocker locker(_lock);

    // TODO: check fd mode
    // TODO: output processing (NL/CR)
    const char* src = static_cast<const char*>(buffer);

    for (size_t i = 0; i < count; ++i) {
        handleOutput(*src++);
    }

    return count;
}

int64_t Terminal::ioctl(OpenFileDescription&, int op, void* argp) {
    SpinlockLocker locker(_lock);

    switch (op) {
        case TCGETS:
            memcpy(argp, &_settings, sizeof(termios));
            return 0;

        case TCSETS:
            // TODO: If we switch from canonical to non-canonical mode, wake up any
            // readers. If we switch form non-canonical to canonical, count up the number
            // of lines in the input buffer If ECHOCTL is turned on, then flush the output
            // buffer and echo the contents
            memcpy(&_settings, argp, sizeof(termios));
            return 0;

        case TIOCGWINSZ: {
            winsize* ws = static_cast<winsize*>(argp);
            ws->ws_row = _screen.height();
            ws->ws_col = _screen.width();
            return 0;
        }

        case TIOCSWINSZ:
            return -EINVAL;

        default:
            return -EINVAL;
    }
}
