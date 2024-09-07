#include "terminal.h"

#include "estd/vector.h"
#include "estd/print.h"
#include "io.h"

Terminal::Terminal(KeyboardDevice& keyboard, Screen& screen)
: _keyboard(keyboard), _screen(screen) {
    _keyboard.addListener(this);
}

const char* keyCodeToString(KeyCode keyCode) {
    switch (keyCode) {
        case KeyCode::Unknown:
            return "Unknown";
        case KeyCode::Escape:
            return "Escape";
        case KeyCode::One:
            return "One";
        case KeyCode::Two:
            return "Two";
        case KeyCode::Three:
            return "Three";
        case KeyCode::Four:
            return "Four";
        case KeyCode::Five:
            return "Five";
        case KeyCode::Six:
            return "Six";
        case KeyCode::Seven:
            return "Seven";
        case KeyCode::Eight:
            return "Eight";
        case KeyCode::Nine:
            return "Nine";
        case KeyCode::Zero:
            return "Zero";
        case KeyCode::Minus:
            return "Minus";
        case KeyCode::Equals:
            return "Equals";
        case KeyCode::Backspace:
            return "Backspace";
        case KeyCode::Tab:
            return "Tab";
        case KeyCode::Q:
            return "Q";
        case KeyCode::W:
            return "W";
        case KeyCode::E:
            return "E";
        case KeyCode::R:
            return "R";
        case KeyCode::T:
            return "T";
        case KeyCode::Y:
            return "Y";
        case KeyCode::U:
            return "U";
        case KeyCode::I:
            return "I";
        case KeyCode::O:
            return "O";
        case KeyCode::P:
            return "P";
        case KeyCode::LBracket:
            return "LBracket";
        case KeyCode::RBracket:
            return "RBracket";
        case KeyCode::Enter:
            return "Enter";
        case KeyCode::LCtrl:
            return "LCtrl";
        case KeyCode::A:
            return "A";
        case KeyCode::S:
            return "S";
        case KeyCode::D:
            return "D";
        case KeyCode::F:
            return "F";
        case KeyCode::G:
            return "G";
        case KeyCode::H:
            return "H";
        case KeyCode::J:
            return "J";
        case KeyCode::K:
            return "K";
        case KeyCode::L:
            return "L";
        case KeyCode::Semicolon:
            return "Semicolon";
        case KeyCode::Apostrophe:
            return "Apostrophe";
        case KeyCode::Backtick:
            return "Backtick";
        case KeyCode::LShift:
            return "LShift";
        case KeyCode::Backslash:
            return "Backslash";
        case KeyCode::Z:
            return "Z";
        case KeyCode::X:
            return "X";
        case KeyCode::C:
            return "C";
        case KeyCode::V:
            return "V";
        case KeyCode::B:
            return "B";
        case KeyCode::N:
            return "N";
        case KeyCode::M:
            return "M";
        case KeyCode::Comma:
            return "Comma";
        case KeyCode::Period:
            return "Period";
        case KeyCode::Slash:
            return "Slash";
        case KeyCode::RShift:
            return "RShift";
        case KeyCode::KeypadAsterisk:
            return "KeypadAsterisk";
        case KeyCode::LAlt:
            return "LAlt";
        case KeyCode::Space:
            return "Space";
        case KeyCode::CapsLock:
            return "CapsLock";
        case KeyCode::F1:
            return "F1";
        case KeyCode::F2:
            return "F2";
        case KeyCode::F3:
            return "F3";
        case KeyCode::F4:
            return "F4";
        case KeyCode::F5:
            return "F5";
        case KeyCode::F6:
            return "F6";
        case KeyCode::F7:
            return "F7";
        case KeyCode::F8:
            return "F8";
        case KeyCode::F9:
            return "F9";
        case KeyCode::F10:
            return "F10";
        case KeyCode::NumLock:
            return "NumLock";
        case KeyCode::ScrollLock:
            return "ScrollLock";
        case KeyCode::Keypad7:
            return "Keypad7";
        case KeyCode::Keypad8:
            return "Keypad8";
        case KeyCode::Keypad9:
            return "Keypad9";
        case KeyCode::KeypadMinus:
            return "KeypadMinus";
        case KeyCode::Keypad4:
            return "Keypad4";
        case KeyCode::Keypad5:
            return "Keypad5";
        case KeyCode::Keypad6:
            return "Keypad6";
        case KeyCode::KeypadPlus:
            return "KeypadPlus";
        case KeyCode::Keypad1:
            return "Keypad1";
        case KeyCode::Keypad2:
            return "Keypad2";
        case KeyCode::Keypad3:
            return "Keypad3";
        case KeyCode::Keypad0:
            return "Keypad0";
        case KeyCode::KeypadDot:
            return "KeypadDot";
        case KeyCode::F11:
            return "F11";
        case KeyCode::F12:
            return "F12";
        case KeyCode::KeypadEnter:
            return "KeypadEnter";
        case KeyCode::RCtrl:
            return "RCtrl";
        case KeyCode::KeypadSlash:
            return "KeypadSlash";
        case KeyCode::RAlt:
            return "RAlt";
        case KeyCode::Home:
            return "Home";
        case KeyCode::Up:
            return "Up";
        case KeyCode::PageUp:
            return "PageUp";
        case KeyCode::Left:
            return "Left";
        case KeyCode::Right:
            return "Right";
        case KeyCode::End:
            return "End";
        case KeyCode::Down:
            return "Down";
        case KeyCode::PageDown:
            return "PageDown";
        case KeyCode::Insert:
            return "Insert";
        case KeyCode::Delete:
            return "Delete";
        case KeyCode::Menu:
            return "Menu";
        default:
            return "<unknown>";
    }
}

char keyCodeToAsciiUnshifted(KeyCode keyCode) {
    switch (keyCode) {
        case KeyCode::Escape:
            return '\033';
        case KeyCode::One:
            return '1';
        case KeyCode::Two:
            return '2';
        case KeyCode::Three:
            return '3';
        case KeyCode::Four:
            return '4';
        case KeyCode::Five:
            return '5';
        case KeyCode::Six:
            return '6';
        case KeyCode::Seven:
            return '7';
        case KeyCode::Eight:
            return '8';
        case KeyCode::Nine:
            return '9';
        case KeyCode::Zero:
            return '0';
        case KeyCode::Minus:
            return '-';
        case KeyCode::Equals:
            return '=';
        case KeyCode::Q:
            return 'q';
        case KeyCode::W:
            return 'w';
        case KeyCode::E:
            return 'e';
        case KeyCode::R:
            return 'r';
        case KeyCode::T:
            return 't';
        case KeyCode::Y:
            return 'y';
        case KeyCode::U:
            return 'u';
        case KeyCode::I:
            return 'i';
        case KeyCode::O:
            return 'o';
        case KeyCode::P:
            return 'p';
        case KeyCode::LBracket:
            return '[';
        case KeyCode::RBracket:
            return ']';
        case KeyCode::A:
            return 'a';
        case KeyCode::S:
            return 's';
        case KeyCode::D:
            return 'd';
        case KeyCode::F:
            return 'f';
        case KeyCode::G:
            return 'g';
        case KeyCode::H:
            return 'h';
        case KeyCode::J:
            return 'j';
        case KeyCode::K:
            return 'k';
        case KeyCode::L:
            return 'l';
        case KeyCode::Semicolon:
            return ';';
        case KeyCode::Apostrophe:
            return '\'';
        case KeyCode::Backtick:
            return '`';
        case KeyCode::Backslash:
            return '\\';
        case KeyCode::Z:
            return 'z';
        case KeyCode::X:
            return 'x';
        case KeyCode::C:
            return 'c';
        case KeyCode::V:
            return 'v';
        case KeyCode::B:
            return 'b';
        case KeyCode::N:
            return 'n';
        case KeyCode::M:
            return 'm';
        case KeyCode::Comma:
            return ',';
        case KeyCode::Period:
            return '.';
        case KeyCode::Slash:
            return '/';
        case KeyCode::KeypadAsterisk:
            return '*';
        case KeyCode::Space:
            return ' ';
        case KeyCode::Keypad7:
            return '7';
        case KeyCode::Keypad8:
            return '8';
        case KeyCode::Keypad9:
            return '9';
        case KeyCode::KeypadMinus:
            return '-';
        case KeyCode::Keypad4:
            return '4';
        case KeyCode::Keypad5:
            return '5';
        case KeyCode::Keypad6:
            return '6';
        case KeyCode::KeypadPlus:
            return '+';
        case KeyCode::Keypad1:
            return '1';
        case KeyCode::Keypad2:
            return '2';
        case KeyCode::Keypad3:
            return '3';
        case KeyCode::Keypad0:
            return '0';
        case KeyCode::KeypadDot:
            return '.';
        case KeyCode::KeypadSlash:
            return '/';
        case KeyCode::Enter:
            return '\n';
        case KeyCode::KeypadEnter:
            return '\n';
        default:
            return '\0';
    }
}

char keyCodeToAsciiShifted(KeyCode keyCode) {
    switch (keyCode) {
        case KeyCode::Escape:
            return '\033';
        case KeyCode::One:
            return '!';
        case KeyCode::Two:
            return '@';
        case KeyCode::Three:
            return '#';
        case KeyCode::Four:
            return '$';
        case KeyCode::Five:
            return '%';
        case KeyCode::Six:
            return '^';
        case KeyCode::Seven:
            return '&';
        case KeyCode::Eight:
            return '*';
        case KeyCode::Nine:
            return '(';
        case KeyCode::Zero:
            return ')';
        case KeyCode::Minus:
            return '-';
        case KeyCode::Equals:
            return '=';
        case KeyCode::Q:
            return 'Q';
        case KeyCode::W:
            return 'W';
        case KeyCode::E:
            return 'E';
        case KeyCode::R:
            return 'R';
        case KeyCode::T:
            return 'T';
        case KeyCode::Y:
            return 'Y';
        case KeyCode::U:
            return 'U';
        case KeyCode::I:
            return 'I';
        case KeyCode::O:
            return 'O';
        case KeyCode::P:
            return 'P';
        case KeyCode::LBracket:
            return '{';
        case KeyCode::RBracket:
            return '}';
        case KeyCode::A:
            return 'A';
        case KeyCode::S:
            return 'S';
        case KeyCode::D:
            return 'D';
        case KeyCode::F:
            return 'F';
        case KeyCode::G:
            return 'G';
        case KeyCode::H:
            return 'H';
        case KeyCode::J:
            return 'J';
        case KeyCode::K:
            return 'K';
        case KeyCode::L:
            return 'L';
        case KeyCode::Semicolon:
            return ':';
        case KeyCode::Apostrophe:
            return '"';
        case KeyCode::Backtick:
            return '~';
        case KeyCode::Backslash:
            return '|';
        case KeyCode::Z:
            return 'Z';
        case KeyCode::X:
            return 'X';
        case KeyCode::C:
            return 'C';
        case KeyCode::V:
            return 'V';
        case KeyCode::B:
            return 'B';
        case KeyCode::N:
            return 'N';
        case KeyCode::M:
            return 'M';
        case KeyCode::Comma:
            return '<';
        case KeyCode::Period:
            return '>';
        case KeyCode::Slash:
            return '?';
        case KeyCode::KeypadAsterisk:
            return '*';
        case KeyCode::Space:
            return ' ';
        case KeyCode::Keypad7:
            return '7';
        case KeyCode::Keypad8:
            return '8';
        case KeyCode::Keypad9:
            return '9';
        case KeyCode::KeypadMinus:
            return '-';
        case KeyCode::Keypad4:
            return '4';
        case KeyCode::Keypad5:
            return '5';
        case KeyCode::Keypad6:
            return '6';
        case KeyCode::KeypadPlus:
            return '+';
        case KeyCode::Keypad1:
            return '1';
        case KeyCode::Keypad2:
            return '2';
        case KeyCode::Keypad3:
            return '3';
        case KeyCode::Keypad0:
            return '0';
        case KeyCode::KeypadDot:
            return '.';
        case KeyCode::KeypadSlash:
            return '/';
        case KeyCode::Enter:
            return '\n';
        case KeyCode::KeypadEnter:
            return '\n';
        default:
            return '\0';
    }
}

void Terminal::onKeyEvent(const KeyboardEvent& event) {
    SpinlockLocker locker(_lock);

    // println("Terminal::onKeyEvent: keycode={:X}, key={}, pressed={}",
    //        (uint8_t)event.key, keyCodeToString(event.key), event.pressed);

    if (event.pressed) {
        // TODO: input-buffer editing

        bool shifted =
            _keyboard.isPressed(KeyCode::LShift) | _keyboard.isPressed(KeyCode::RShift);
        char c = shifted ? keyCodeToAsciiShifted(event.key)
                         : keyCodeToAsciiUnshifted(event.key);

        if (c != '\0') {
            _inputBuffer.push(c);
            handleChar(c);
        }
    }
}

void Terminal::handleChar(char c) {
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
    echo(c);
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
        // CSI sequences
        if (++idx == _outputBuffer.size()) return false;
        char c = _outputBuffer[idx];

        // Look for an optional numerical argument
        Vector<int> args;
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
                    }
                }

                return true;
            }

            default:
                // Bad escape sequence, ignore it and clear the buffer
                return true;
        }
    } else {
        // Bad escape sequence, ignore it and clear the buffer
        return true;
    }
}

void Terminal::echo(char c) {
    if (c == '\n') {
        newline();
        return;
    } else if (c == '\b') {
        backspace();
        return;
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

void Terminal::newline() {
    _x = 0;
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

    // TODO: check fd mode
    // TODO: blocking, canonical mode
    size_t bytesRead = 0;
    char* dest = static_cast<char*>(buffer);

    while (_inputBuffer && bytesRead < count) {
        *dest++ = _inputBuffer.pop();
        ++bytesRead;
    }

    return bytesRead;
}

ssize_t Terminal::write(OpenFileDescription&, const void* buffer, size_t count) {
    SpinlockLocker locker(_lock);

    // TODO: check fd mode
    // TODO: output processing (NL/CR)
    const char* src = static_cast<const char*>(buffer);

    for (size_t i = 0; i < count; ++i) {
        handleChar(*src++);
    }

    return count;
}
