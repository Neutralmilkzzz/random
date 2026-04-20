// KeyBoard.cpp
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif

#include "KeyBoard.h"
#include <iostream>
#include <chrono>
#include <thread>


Keyboard::Keyboard() {
    initKeyboard();
}

Keyboard::~Keyboard() {
    restoreKeyboard();
}

void Keyboard::initKeyboard() {
#ifdef _WIN32
    // Windows: 获取控制台句柄和原始模式
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &originalMode);  // 直接传 DWORD 地址

    // 设置新的控制台模式：禁用行缓冲和回显
    DWORD newMode = originalMode;
    newMode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    newMode |= ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hStdin, newMode);
#else
    // Linux/macOS: 获取当前终端设置
    tcgetattr(STDIN_FILENO, &originalTermios);

    // 设置新的终端属性
    struct termios newTermios = originalTermios;
    newTermios.c_lflag &= ~(ICANON | ECHO);  // 禁用规范模式和回显
    newTermios.c_cc[VMIN] = 1;   // 最小读取字符数
    newTermios.c_cc[VTIME] = 0;  // 超时时间

    tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
#endif
}

void Keyboard::restoreKeyboard() {
#ifdef _WIN32
    // Windows: 恢复原始控制台模式
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleMode(hStdin, originalMode);
#else
    // Linux/macOS: 恢复原始终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
#endif
}

int Keyboard::getKey() {
#ifdef _WIN32
    // Windows: 使用 _getch() 获取按键
    return _getch();
#else
    // Linux/macOS: 使用 read() 获取按键
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return -1;
#endif
}

bool Keyboard::kbhit() {
#ifdef _WIN32
    // Windows: 检查是否有按键
    return _kbhit() != 0;
#else
    // Linux/macOS: 使用 select 检查是否有输入
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
#endif
}

void Keyboard::clearScreen() {
#ifdef _WIN32
    // Windows: 使用系统命令清屏
    system("cls");
#else
    // Linux/macOS: 使用 ANSI 转义序列清屏
    std::cout << "\033[2J\033[H";
#endif
}

void Keyboard::setRawMode(bool enable) {
    if (enable) {
        initKeyboard();
    } else {
        restoreKeyboard();
    }
}

// ... 前面的包含和构造函数保持不变 ...

// 新增：更新按键状态
void Keyboard::updateKeyStates() {
#ifdef _WIN32
    // Windows: 使用 GetAsyncKeyState 检测按键状态
    // 常用按键的虚拟键码
    struct KeyMapping {
        char key;
        int vkCode;
    };

    static const KeyMapping mappings[] = {
            {'a', 0x41}, {'A', 0x41},
            {'d', 0x44}, {'D', 0x44},
            {'w', 0x57}, {'W', 0x57},
            {'s', 0x53}, {'S', 0x53},
            {' ', 0x20},  // 空格
            {0x1B, 0x1B}, // ESC
    };

    pressedKeys.clear();

    for (const auto& mapping : mappings) {
        SHORT state = GetAsyncKeyState(mapping.vkCode);
        bool isPressed = (state & 0x8000) != 0;

        keyStates[mapping.key] = isPressed;
        if (isPressed) {
            pressedKeys.push_back(mapping.key);
        }
    }

#else
    // Linux: 使用非阻塞读取获取所有可用按键
    pressedKeys.clear();

    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    // 清空之前的按键状态
    for (auto& pair : keyStates) {
        pair.second = false;
    }

    // 读取所有可用的按键
    while (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            keyStates[ch] = true;
            pressedKeys.push_back(ch);
        }

        // 重置 select
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
    }
#endif
}

// 新增：检查特定按键是否按下
bool Keyboard::isKeyPressed(int keyCode) {
    auto it = keyStates.find(keyCode);
    return it != keyStates.end() && it->second;
}

// 新增：获取所有按下的键
std::vector<int> Keyboard::getPressedKeys() {
    return pressedKeys;
}

// ... 其他函数保持不变 ...
