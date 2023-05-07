int main() {
    char* screen = (char*)0xB8000;

    for (int i = 0; i < 80 * 25; ++i) {
        screen[2 * i] = 0x20;
        screen[2 * i + 1] = 0xA0;
    }

    return 0;
}
