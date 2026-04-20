//
// Created by ender on 2026/4/20.
//

#ifndef TESTCPP1_PLAYER_H
#define TESTCPP1_PLAYER_H


#include <vector>
#include <string>
#include "input/KeyStateManager.h"

class Player {
public:

    KeyStateManager& ksm;

private:
    bool isJumping;      // 是否正在跳跃
    int jumpProgress;    // 跳跃进度（剩余上升次数）
    int jumpHeight;      // 跳跃总高度
    int jumpFrameCounter; // 跳跃帧计数器
    int gravityFrameCounter; // 重力帧计数器
    int moveFrameCounter; // 水平移动帧计数器
    int groundedMoveInterval; // 地面移动间隔
    int airborneMoveInterval; // 空中移动间隔
    int jumpRiseInterval; // 上升间隔
    int jumpApexInterval; // 接近最高点时的上升间隔
    int gravityInterval; // 下落间隔

public:
    Player(KeyStateManager& ksm);

    std::vector<int> getPalce();

    void move(std::string &currentmap);

    // 辅助函数
    bool isGrounded(const std::string &currentmap, size_t pos);
    bool applyGravity(std::string &currentmap, size_t pos);  // 返回是否下落了
    bool jumpUp(std::string &currentmap, size_t pos);        // 返回是否上升了
};

#endif //TESTCPP1_PLAYER_H
