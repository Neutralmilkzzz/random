#ifndef KEYSTATEMANAGER_H
#define KEYSTATEMANAGER_H

#include <unordered_map>

int runtimeDashKeyCode();
const char* runtimeDashKeyLabel();

class KeyStateManager {
public:
    KeyStateManager();
    ~KeyStateManager();

    // 按键状态存储
    std::unordered_map<int, bool> keyStates;

    // 读取当前按键状态（不自动清空）
    void readKeys();

    // 清空所有按键状态
    void clearKeys();

#ifndef _WIN32
private:
    std::unordered_map<int, int> keyHoldFrames;

    void registerKeyState(int keyCode, int holdFrames);
#endif
};

#endif // KEYSTATEMANAGER_H
