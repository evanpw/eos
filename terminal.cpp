#include "terminal.h"

#include "estd/print.h"
#include "io.h"

Terminal::Terminal(KeyboardDevice& keyboard, Screen& screen)
: _keyboard(keyboard), _screen(screen) {
    _keyboard.addListener(this);
    echo('$');
    echo(' ');
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
    // println("Terminal::onKeyEvent: keycode={:X}, key={}, pressed={}",
    //        (uint8_t)event.key, keyCodeToString(event.key), event.pressed);

    if (event.pressed) {
        SpinlockLocker locker(_lock);

        if (event.key == KeyCode::Backspace) {
            if (_inputBuffer) {
                _inputBuffer.popBack();
                echo('\b');
            }
        }

        bool shifted = _keyboard.isPressed(KeyCode::LShift) |
                       _keyboard.isPressed(KeyCode::RShift);
        char c = shifted ? keyCodeToAsciiShifted(event.key)
                         : keyCodeToAsciiUnshifted(event.key);
        if (c != '\0') {
            _inputBuffer.push(c);
            echo(c);
        }
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

    _screen.putChar(_x, _y, c, Screen::Black, Screen::LightGrey);

    // Advance cursor
    ++_x;
    if (_x == _screen.width()) {
        _x = 0;
        ++_y;
        if (_y == _screen.height()) {
            // TODO: scroll
            _y = 0;
        }
    }

    _screen.setCursor(_x, _y);
}

void Terminal::newline() {
    _x = 0;
    ++_y;
    if (_y == _screen.height()) {
        // TODO: scroll
        _y = 0;
    }

    _screen.setCursor(_x, _y);
}

void Terminal::backspace() {
    --_x;
    if (_x < 0) {
        _x = _screen.width() - 1;
        --_y;
        if (_y < 0) {
            // TODO: scroll
            _y = _screen.height() - 1;
        }
    }

    _screen.putChar(_x, _y, ' ', Screen::Black, Screen::LightGrey);
    _screen.setCursor(_x, _y);
}

ssize_t Terminal::read(OpenFileDescription& fd, void* buffer, size_t count) {
    // TODO: check fd mode
    // TODO: blocking, canonical mode
    size_t bytesRead = 0;
    char* dest = (char*)buffer;

    SpinlockLocker locker(_lock);
    while (_inputBuffer && bytesRead < count) {
        *dest++ = _inputBuffer.pop();
        ++bytesRead;
    }

    return bytesRead;
}

ssize_t Terminal::write(OpenFileDescription& fd, const void* buffer,
                        size_t count) {
    // TODO: check fd mode
    // TODO: output processing (NL/CR)
    char* src = (char*)buffer;

    for (size_t i = 0; i < count; ++i) {
        echo(*src++);
    }

    return count;
}
