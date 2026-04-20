#include "Player.h"
#include "KeyStateManager.h"

Player::Player(KeyStateManager& ksm1)  : ksm(ksm1){
    isJumping = false;
    jumpProgress = 0;
    jumpHeight = 5;  // 跳跃总高度4格
    jumpFrameCounter = 0;
    gravityFrameCounter = 0;
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
    // 计算行宽（用于垂直移动检查）
    size_t newlinePos = currentmap.find('\n');
    size_t lineWidth = newlinePos + 1;

    // 第一步：水平移动
    // 向左移动检查（同时检查大小写）
    if (ksm.keyStates['a'] || ksm.keyStates['A']) {
        // 检查左边是否是空格，并且左边不是换行符
        if (pos > 0 && currentmap[pos - 1] == ' ') {
            std::swap(currentmap[pos], currentmap[pos - 1]);
            pos = pos - 1;  // 更新位置
        }
    }

    pos = currentmap.find('@');
    // 向右移动检查（同时检查大小写）
    if (ksm.keyStates['d'] || ksm.keyStates['D']) {
        // 检查右边是否是空格，并且右边不是换行符
        if (pos + 1 < currentmap.length() && currentmap[pos + 1] == ' ') {
            std::swap(currentmap[pos], currentmap[pos + 1]);
            pos = pos + 1;  // 更新位置
        }
    }

    pos = currentmap.find('@');
    // 第二步：处理跳跃（检查空格键）
    if (ksm.keyStates[' '] && !isJumping) {
        // 检查是否在地面上
        size_t belowPos = pos + lineWidth;

        // 如果下方不是空格（即在地面上），才能起跳
        if (belowPos < currentmap.length() && currentmap[belowPos] != ' ') {
            isJumping = true;
            jumpProgress = jumpHeight;
            jumpFrameCounter = 0;
            gravityFrameCounter = 0;  // 重置重力计数器
        }
    }

    pos = currentmap.find('@');
    // 第三步：处理跳跃状态
    if (isJumping) {
        // 跳跃期间：每5帧上升1格
        jumpFrameCounter++;

        if (jumpFrameCounter >= 6) {
            jumpFrameCounter = 0;

            if (jumpProgress > 0) {
                // 尝试上升
                size_t currentPos = currentmap.find('@');
                if (currentPos != std::string::npos) {
                    if (jumpUp(currentmap, currentPos)) {
                        jumpProgress--;  // 成功上升
                    } else {
                        // 碰到天花板，结束跳跃
                        isJumping = false;
                        jumpProgress = 0;
                        gravityFrameCounter = 0;
                    }
                }
            } else {
                // 跳跃上升阶段结束
                isJumping = false;
                gravityFrameCounter = 0;
            }
        }
    }
    pos = currentmap.find('@');
    // 第四步：非跳跃状态下应用重力
    if (!isJumping) {
        gravityFrameCounter++;

        if (gravityFrameCounter >= 8) {
            gravityFrameCounter = 0;

            size_t currentPos = currentmap.find('@');
            if (currentPos != std::string::npos) {
                // 尝试下落
                applyGravity(currentmap, currentPos);
            }
        }
    }
}
