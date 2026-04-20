#ifndef KEYBOARD_H
#define KEYBOARD_H

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#include <unordered_map>
#include <vector>

class Keyboard {
private:
#ifdef _WIN32
    HANDLE hStdin;
    DWORD originalMode;
#else
    struct termios originalTermios;
#endif

    // 按键状态映射
    std::unordered_map<int, bool> keyStates;
    std::vector<int> pressedKeys;  // 当前按下的键

public:
    Keyboard();
    ~Keyboard();

    void initKeyboard();
    void restoreKeyboard();

    bool kbhit();           // 是否有按键
    int getKey();           // 获取单个按键（向后兼容）

    // 新增方法
    void updateKeyStates(); // 更新所有按键状态
    bool isKeyPressed(int keyCode);  // 检查特定按键是否按下
    std::vector<int> getPressedKeys();  // 获取所有按下的键

    static void clearScreen();
    void setRawMode(bool enable);
};

#endif // KEYBOARD_H
