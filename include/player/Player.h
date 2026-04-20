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
    bool isJumping;
    bool jumpHeldLastFrame;
    float horizontalMoveAccumulator;
    float verticalMoveAccumulator;
    float upwardVelocity;
    float downwardVelocity;
    float jumpHoldRemaining;
    float minimumJumpRiseRemaining;
    float riseVelocityDropAccumulator;

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
