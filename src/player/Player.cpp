#include "player/Player.h"

#include <algorithm>

namespace {

const float kFixedDeltaSeconds = 0.016f;
const float kRunSpeed = 10.79f;
const float kAirMoveSpeed = 10.79f;
const float kInitialJumpUpwardSpeed = 12.22f;
const float kJumpInitialHoldTime = 0.2f;
const float kJumpVelocityDropStep = 0.95f;
const float kJumpVelocityDropInterval = 0.02f;
const float kMinimumJumpRiseTime = 0.08f;
const float kFallSpeedCap = 20.9f;
const float kGravityAcceleration = kJumpVelocityDropStep / kJumpVelocityDropInterval;

} // namespace

Player::Player(KeyStateManager& ksm1)
    : ksm(ksm1),
      isJumping(false),
      jumpHeldLastFrame(false),
      horizontalMoveAccumulator(0.0f),
      verticalMoveAccumulator(0.0f),
      upwardVelocity(0.0f),
      downwardVelocity(0.0f),
      jumpHoldRemaining(0.0f),
      minimumJumpRiseRemaining(0.0f),
      riseVelocityDropAccumulator(0.0f) {
}

bool Player::isGrounded(const std::string& currentmap, size_t pos) {
    size_t newlinePos = currentmap.find('\n');
    if (newlinePos == std::string::npos) {
        return false;
    }

    size_t lineWidth = newlinePos + 1;
    size_t belowPos = pos + lineWidth;
    if (belowPos >= currentmap.length()) {
        return false;
    }

    return currentmap[belowPos] != ' ';
}

bool Player::applyGravity(std::string& currentmap, size_t pos) {
    size_t newlinePos = currentmap.find('\n');
    if (newlinePos == std::string::npos) {
        return false;
    }

    size_t lineWidth = newlinePos + 1;
    size_t belowPos = pos + lineWidth;
    if (belowPos >= currentmap.length()) {
        return false;
    }

    if (currentmap[belowPos] == ' ') {
        std::swap(currentmap[pos], currentmap[belowPos]);
        return true;
    }

    return false;
}

bool Player::jumpUp(std::string& currentmap, size_t pos) {
    size_t newlinePos = currentmap.find('\n');
    if (newlinePos == std::string::npos) {
        return false;
    }

    size_t lineWidth = newlinePos + 1;
    if (pos < lineWidth) {
        return false;
    }

    size_t abovePos = pos - lineWidth;
    if (currentmap[abovePos] == ' ') {
        std::swap(currentmap[pos], currentmap[abovePos]);
        return true;
    }

    return false;
}

void Player::move(std::string& currentmap) {
    size_t pos = currentmap.find('@');
    if (pos == std::string::npos) {
        return;
    }

    const bool movingLeft = ksm.keyStates['a'] || ksm.keyStates['A'];
    const bool movingRight = ksm.keyStates['d'] || ksm.keyStates['D'];
    const bool jumpHeld = ksm.keyStates[' '];
    const bool jumpJustPressed = jumpHeld && !jumpHeldLastFrame;

    bool grounded = isGrounded(currentmap, pos);

    if (grounded && !isJumping && upwardVelocity <= 0.0f) {
        downwardVelocity = 0.0f;
        verticalMoveAccumulator = 0.0f;
    }

    if (jumpJustPressed && grounded) {
        isJumping = true;
        upwardVelocity = kInitialJumpUpwardSpeed;
        downwardVelocity = 0.0f;
        jumpHoldRemaining = kJumpInitialHoldTime;
        minimumJumpRiseRemaining = kMinimumJumpRiseTime;
        riseVelocityDropAccumulator = 0.0f;
        verticalMoveAccumulator = 0.0f;
        grounded = false;
    }

    if (movingLeft != movingRight) {
        const float moveSpeed = grounded ? kRunSpeed : kAirMoveSpeed;
        horizontalMoveAccumulator += moveSpeed * kFixedDeltaSeconds;

        while (horizontalMoveAccumulator >= 1.0f) {
            pos = currentmap.find('@');
            if (pos == std::string::npos) {
                break;
            }

            bool moved = false;
            if (movingLeft && pos > 0 && currentmap[pos - 1] == ' ') {
                std::swap(currentmap[pos], currentmap[pos - 1]);
                moved = true;
            } else if (movingRight && pos + 1 < currentmap.length() && currentmap[pos + 1] == ' ') {
                std::swap(currentmap[pos], currentmap[pos + 1]);
                moved = true;
            }

            if (!moved) {
                horizontalMoveAccumulator = 0.0f;
                break;
            }

            horizontalMoveAccumulator -= 1.0f;
        }
    } else {
        horizontalMoveAccumulator = 0.0f;
    }

    pos = currentmap.find('@');
    if (pos == std::string::npos) {
        jumpHeldLastFrame = jumpHeld;
        return;
    }

    grounded = isGrounded(currentmap, pos);

    if (isJumping) {
        if (minimumJumpRiseRemaining > 0.0f) {
            minimumJumpRiseRemaining = std::max(0.0f, minimumJumpRiseRemaining - kFixedDeltaSeconds);
        }

        if (jumpHoldRemaining > 0.0f) {
            jumpHoldRemaining = std::max(0.0f, jumpHoldRemaining - kFixedDeltaSeconds);
        }

        if (!jumpHeld && minimumJumpRiseRemaining <= 0.0f) {
            upwardVelocity = 0.0f;
            jumpHoldRemaining = 0.0f;
        }

        if (jumpHoldRemaining <= 0.0f && upwardVelocity > 0.0f) {
            riseVelocityDropAccumulator += kFixedDeltaSeconds;
            while (riseVelocityDropAccumulator >= kJumpVelocityDropInterval && upwardVelocity > 0.0f) {
                riseVelocityDropAccumulator -= kJumpVelocityDropInterval;
                upwardVelocity = std::max(0.0f, upwardVelocity - kJumpVelocityDropStep);
            }
        }

        if (upwardVelocity > 0.0f) {
            verticalMoveAccumulator += upwardVelocity * kFixedDeltaSeconds;

            while (verticalMoveAccumulator >= 1.0f) {
                size_t currentPos = currentmap.find('@');
                if (currentPos == std::string::npos) {
                    break;
                }

                if (!jumpUp(currentmap, currentPos)) {
                    upwardVelocity = 0.0f;
                    jumpHoldRemaining = 0.0f;
                    riseVelocityDropAccumulator = 0.0f;
                    verticalMoveAccumulator = 0.0f;
                    break;
                }

                verticalMoveAccumulator -= 1.0f;
            }
        }

        if (upwardVelocity <= 0.0f) {
            isJumping = false;
        }
    }

    pos = currentmap.find('@');
    if (pos == std::string::npos) {
        jumpHeldLastFrame = jumpHeld;
        return;
    }

    grounded = isGrounded(currentmap, pos);

    if (!grounded && !isJumping) {
        downwardVelocity = std::min(kFallSpeedCap, downwardVelocity + kGravityAcceleration * kFixedDeltaSeconds);
        verticalMoveAccumulator += downwardVelocity * kFixedDeltaSeconds;

        while (verticalMoveAccumulator >= 1.0f) {
            size_t currentPos = currentmap.find('@');
            if (currentPos == std::string::npos) {
                break;
            }

            if (!applyGravity(currentmap, currentPos)) {
                downwardVelocity = 0.0f;
                verticalMoveAccumulator = 0.0f;
                break;
            }

            verticalMoveAccumulator -= 1.0f;
        }
    } else if (grounded && !isJumping) {
        downwardVelocity = 0.0f;
        verticalMoveAccumulator = 0.0f;
    }

    jumpHeldLastFrame = jumpHeld;
}
