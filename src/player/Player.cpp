#include "player/Player.h"

Player::Player(KeyStateManager& ksm1)  : ksm(ksm1){
    isJumping = false;
    jumpProgress = 0;
    jumpHeight = 4;
    jumpFrameCounter = 0;
    gravityFrameCounter = 0;
    moveFrameCounter = 0;
    groundedMoveInterval = 3;
    airborneMoveInterval = 4;
    jumpRiseInterval = 4;
    jumpApexInterval = 6;
    gravityInterval = 3;
}

bool Player::isGrounded(const std::string &currentmap, size_t pos) {
    size_t newlinePos = currentmap.find('\n');
    if (newlinePos == std::string::npos) return false;
    size_t lineWidth = newlinePos + 1;
    size_t belowPos = pos + lineWidth;

    if (belowPos >= currentmap.length()) {
        return false;
    }

    return currentmap[belowPos] != ' ';
}

bool Player::applyGravity(std::string &currentmap, size_t pos) {
    // 计算行宽
    size_t newlinePos = currentmap.find('\n');
    if (newlinePos == std::string::npos) return false;
    size_t lineWidth = newlinePos + 1;

    // 检查下方是否是空格
    size_t belowPos = pos + lineWidth;

    // 检查是否超出地图范围
    if (belowPos >= currentmap.length()) {
        return false;  // 超出地图底部
    }

    // 如果下方是空格，则下落1格
    if (currentmap[belowPos] == ' ') {
        std::swap(currentmap[pos], currentmap[belowPos]);
        return true;  // 成功下落
    }

    // 检查下方是否是地面（比如 '#' 或其他非空格字符）
    if (currentmap[belowPos] != ' ') {
        return false;  // 碰到地面，不能下落
    }

    return false;
}


bool Player::jumpUp(std::string &currentmap, size_t pos) {
    // 计算行宽
    size_t newlinePos = currentmap.find('\n');
    if (newlinePos == std::string::npos) return false;
    size_t lineWidth = newlinePos + 1;

    // 检查上方是否是空格
    size_t abovePos = pos - lineWidth;

    // 如果上方是空格，则上升1格
    if (abovePos < currentmap.length() && currentmap[abovePos] == ' ') {
        std::swap(currentmap[pos], currentmap[abovePos]);
        return true;  // 成功上升
    }
    return false;  // 没有上升（碰到天花板）
}

void Player::move(std::string &currentmap) {
    // 先找到玩家位置
    size_t pos = currentmap.find('@');
    if (pos == std::string::npos) {
        return;
    }
    bool movingLeft = ksm.keyStates['a'] || ksm.keyStates['A'];
    bool movingRight = ksm.keyStates['d'] || ksm.keyStates['D'];
    bool grounded = isGrounded(currentmap, pos);

    // 第一步：水平移动
    if (movingLeft != movingRight) {
        moveFrameCounter++;
        int moveInterval = grounded ? groundedMoveInterval : airborneMoveInterval;

        if (moveFrameCounter >= moveInterval) {
            moveFrameCounter = 0;

            if (movingLeft && pos > 0 && currentmap[pos - 1] == ' ') {
                std::swap(currentmap[pos], currentmap[pos - 1]);
                pos = pos - 1;
            } else if (movingRight && pos + 1 < currentmap.length() && currentmap[pos + 1] == ' ') {
                std::swap(currentmap[pos], currentmap[pos + 1]);
                pos = pos + 1;
            }
        }
    } else {
        moveFrameCounter = 0;
    }

    pos = currentmap.find('@');
    grounded = isGrounded(currentmap, pos);

    // 第二步：处理跳跃（检查空格键）
    if (ksm.keyStates[' '] && !isJumping) {
        if (grounded) {
            isJumping = true;
            jumpProgress = jumpHeight;
            jumpFrameCounter = 0;
            gravityFrameCounter = 0;
        }
    }

    pos = currentmap.find('@');
    // 第三步：处理跳跃状态
    if (isJumping) {
        jumpFrameCounter++;

        int riseInterval = jumpProgress <= 1 ? jumpApexInterval : jumpRiseInterval;
        if (jumpFrameCounter >= riseInterval) {
            jumpFrameCounter = 0;

            if (jumpProgress > 0) {
                size_t currentPos = currentmap.find('@');
                if (currentPos != std::string::npos) {
                    if (jumpUp(currentmap, currentPos)) {
                        jumpProgress--;
                    } else {
                        isJumping = false;
                        jumpProgress = 0;
                        gravityFrameCounter = 0;
                    }
                }
            } else {
                isJumping = false;
                gravityFrameCounter = 0;
            }
        }
    }

    pos = currentmap.find('@');
    // 第四步：非跳跃状态下应用重力
    if (!isJumping) {
        grounded = isGrounded(currentmap, pos);

        if (!grounded) {
            gravityFrameCounter++;

            if (gravityFrameCounter >= gravityInterval) {
                gravityFrameCounter = 0;
                size_t currentPos = currentmap.find('@');
                if (currentPos != std::string::npos) {
                    applyGravity(currentmap, currentPos);
                }
            }
        } else {
            gravityFrameCounter = 0;
        }
    }
}
