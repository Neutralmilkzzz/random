#include "input/KeyStateManager.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

// Linux/macOS 终端设置
#ifndef _WIN32
static struct termios orig_termios;
static bool terminal_configured = false;
#endif

void KeyStateManager::readKeys() {
#ifdef _WIN32
    const char watchedLetters[] = {
            'A', 'C', 'D', 'W', 'S',
            'J', 'I', 'K', 'L',
            'O', 'M', 'R', 'H',
            'P'
    };
    const char watchedDigits[] = {'1', '2', '3'};

    for (size_t index = 0; index < sizeof(watchedLetters) / sizeof(watchedLetters[0]); ++index) {
        const char uppercase = watchedLetters[index];
        const char lowercase = static_cast<char>(uppercase + ('a' - 'A'));
        const bool pressed = (GetAsyncKeyState(uppercase) & 0x8000) != 0;

        keyStates[uppercase] = pressed;
        keyStates[lowercase] = pressed;
    }

    for (size_t index = 0; index < sizeof(watchedDigits) / sizeof(watchedDigits[0]); ++index) {
        const char digit = watchedDigits[index];
        keyStates[digit] = (GetAsyncKeyState(digit) & 0x8000) != 0;
    }

    keyStates[' '] = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
    keyStates[0x1B] = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
#else
    // Linux/macOS: 配置终端（只做一次）
    if (!terminal_configured) {
        struct termios new_termios;
        tcgetattr(STDIN_FILENO, &orig_termios);
        new_termios = orig_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        terminal_configured = true;
    }

    // 读取按键（不自动清空之前的状态）
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char ch;
        while (read(STDIN_FILENO, &ch, 1) == 1) {
            keyStates[ch] = true;  // 设置按键为按下

            // 大小写互相关联
            if (ch >= 'A' && ch <= 'Z') {
                keyStates[ch + 32] = true;
            } else if (ch >= 'a' && ch <= 'z') {
                keyStates[ch - 32] = true;
            }
        }
    }
#endif
}

void KeyStateManager::clearKeys() {
    // 清空所有按键状态
    keyStates.clear();
}
