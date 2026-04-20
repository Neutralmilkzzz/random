//
// Created by ender on 2026/4/20.
//

#ifndef TESTCPP1_PLAYER_H
#define TESTCPP1_PLAYER_H


#include <vector>
#include <string>
#include "trash/KeyBoard.h"
#include "KeyStateManager.h"

class Player {
public:

    KeyStateManager& ksm;

private:
    bool isJumping;      // 是否正在跳跃
    int jumpProgress;    // 跳跃进度（剩余上升次数）
    int jumpHeight;      // 跳跃总高度（4格）
    int jumpFrameCounter; // 跳跃帧计数器（用于每3帧上升1格）
    int gravityFrameCounter; // 重力帧计数器（用于每4帧下落1格）

public:
    Player(KeyStateManager& ksm);

    std::vector<int> getPalce();

    void move(std::string &currentmap);

    // 辅助函数
    bool applyGravity(std::string &currentmap, size_t pos);  // 返回是否下落了
    bool jumpUp(std::string &currentmap, size_t pos);        // 返回是否上升了
};

#endif //TESTCPP1_PLAYER_H
