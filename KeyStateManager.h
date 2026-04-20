#ifndef KEYSTATEMANAGER_H
#define KEYSTATEMANAGER_H

#include <unordered_map>

class KeyStateManager {
public:
    // 按键状态存储
    std::unordered_map<int, bool> keyStates;

    // 读取当前按键状态（不自动清空）
    void readKeys();

    // 清空所有按键状态
    void clearKeys();
};

#endif // KEYSTATEMANAGER_H
