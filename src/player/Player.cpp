#include "player/Player.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include "enemy/Enemy.h"

namespace {

const float kFixedDeltaSeconds = 0.016f;
const float kRunSpeed = 10.79f;
const float kAirMoveSpeed = 10.79f;
const float kInitialJumpUpwardSpeed = 17.11f;
const float kDoubleJumpUpwardSpeed = 15.85f;
const float kJumpInitialHoldTime = 0.2f;
const float kJumpVelocityDropStep = 0.95f;
const float kJumpVelocityDropInterval = 0.02f;
const float kMinimumJumpRiseTime = 0.08f;
const float kFallSpeedCap = 20.9f;
const float kGravityAcceleration = kJumpVelocityDropStep / kJumpVelocityDropInterval;
const float kDashSpeed = 34.0f;
const int kDashFrames = 8;
const int kDashCooldownFrames = 18;

const int kBlinkFrames = 125;
const int kHealLockoutFrames = 188;
const int kAttackRange = 2;
const int kUpWaveVerticalReach = 5;
const int kUpWaveUpperSpreadHalfWidth = 1;
const int kUpWaveCrownHalfWidth = 2;
const int kDownSlamUpperBlastHalfWidth = 1;
const int kDownSlamImpactHalfWidth = 2;
const int kHealCastFrames = 42;
const int kUpWaveCastFrames = 18;
const int kDownSlamCastFrames = 16;
const int kMeleeVisualFrames = 6;
const int kDeathFreezeFrames = 6;
const int kDeathDotFrames = 4;
const int kDeathExpandFrames = 24;
const int kDeathFullWhiteFrames = 10;
const int kDeathTextFrames = 60;
const int kSoulMeterMax = 99;
const int kSoulGainOnMeleeHit = 11;
const int kSoulSkillCost = 33;
const int kPlayerAttackDamage = 1;
const int kSoulFillUnits = 3;

int digitCount(int value) {
    int clampedValue = std::max(0, value);
    int digits = 1;
    while (clampedValue >= 10) {
        clampedValue /= 10;
        digits++;
    }
    return digits;
}

std::string buildSoulReadoutLine(const game::CharacterStats& stats) {
    const int displayMaximum = std::max(0, stats.soul.maximum);
    const int displayCurrent = std::max(0, std::min(stats.soul.current, displayMaximum));
    const int width = digitCount(std::max(displayMaximum, kSoulMeterMax));

    std::ostringstream line;
    line << "   "
         << std::setw(width) << displayCurrent
         << "/"
         << std::setw(width) << displayMaximum;
    return line.str();
}

int deathAnimationTotalFrames() {
    return kDeathFreezeFrames +
           kDeathDotFrames +
           kDeathExpandFrames +
           kDeathFullWhiteFrames +
           kDeathTextFrames;
}

size_t lineWidth(const std::string& map) {
    const size_t newlinePos = map.find('\n');
    return newlinePos == std::string::npos ? map.length() : newlinePos + 1;
}

game::Position findGlyphPosition(const std::string& map, char glyph) {
    const size_t glyphPos = map.find(glyph);
    const size_t width = lineWidth(map);

    if (glyphPos == std::string::npos || width == 0) {
        return game::Position(-1, -1);
    }

    return game::Position(static_cast<int>(glyphPos % width), static_cast<int>(glyphPos / width));
}

bool isInsidePlayableArea(const std::string& map, const game::Position& position) {
    const size_t width = lineWidth(map);
    const size_t height = width == 0 ? 0 : (map.length() + 1) / width;

    return position.x >= 0 &&
           position.y >= 0 &&
           position.x < static_cast<int>(width - 1) &&
           position.y < static_cast<int>(height);
}

size_t indexFromPosition(const std::string& map, const game::Position& position) {
    return static_cast<size_t>(position.y) * lineWidth(map) + static_cast<size_t>(position.x);
}

char tileAt(const std::string& map, const game::Position& position) {
    if (!isInsidePlayableArea(map, position)) {
        return '#';
    }
    return map[indexFromPosition(map, position)];
}

void placeGlyph(std::string& map, const game::Position& position, char glyph) {
    if (!isInsidePlayableArea(map, position)) {
        return;
    }
    map[indexFromPosition(map, position)] = glyph;
}

void placeText(std::string& map, const game::Position& startPosition, const std::string& text) {
    for (size_t index = 0; index < text.size(); ++index) {
        placeGlyph(map, game::Position(startPosition.x + static_cast<int>(index), startPosition.y), text[index]);
    }
}

bool moveHorizontallyOneTile(std::string& currentmap, int dx) {
    const size_t pos = currentmap.find('@');
    if (pos == std::string::npos || dx == 0) {
        return false;
    }

    if (dx < 0) {
        if (pos == 0 || currentmap[pos - 1] != ' ') {
            return false;
        }
        std::swap(currentmap[pos], currentmap[pos - 1]);
        return true;
    }

    if (pos + 1 >= currentmap.length() || currentmap[pos + 1] != ' ') {
        return false;
    }

    std::swap(currentmap[pos], currentmap[pos + 1]);
    return true;
}

} // namespace

Player::Player(KeyStateManager& keyStateManager)
    : ksm(keyStateManager),
      id("player"),
      combatPosition(-1, -1),
      isJumping(false),
      jumpHeldLastFrame(false),
      doubleJumpUnlocked(true),
      airJumpAvailable(true),
      dashActive(false),
      dashAvailable(true),
      horizontalMoveAccumulator(0.0f),
      verticalMoveAccumulator(0.0f),
      dashMoveAccumulator(0.0f),
      upwardVelocity(0.0f),
      downwardVelocity(0.0f),
      jumpHoldRemaining(0.0f),
      minimumJumpRiseRemaining(0.0f),
      riseVelocityDropAccumulator(0.0f),
      dashFramesRemaining(0),
      dashCooldownFrames(0),
      dashDirection(1),
      facing(game::FacingDirection::Right),
      framesSinceLastDamage(kHealLockoutFrames),
      blinkFramesRemaining(0),
      lastAction("Ready"),
      lastResult("Main player combat kit integrated."),
      resetQueued(false) {
    resetRuntimeState();
}

std::vector<int> Player::getPalce() {
    return std::vector<int>();
}

void Player::resetRuntimeState() {
    stats = game::CharacterStats();
    stats.soul.maximum = kSoulMeterMax;
    stats.soul.current = 0;
    combatPosition = game::Position(-1, -1);
    isJumping = false;
    jumpHeldLastFrame = false;
    airJumpAvailable = doubleJumpUnlocked;
    dashActive = false;
    dashAvailable = true;
    horizontalMoveAccumulator = 0.0f;
    verticalMoveAccumulator = 0.0f;
    dashMoveAccumulator = 0.0f;
    upwardVelocity = 0.0f;
    downwardVelocity = 0.0f;
    jumpHoldRemaining = 0.0f;
    minimumJumpRiseRemaining = 0.0f;
    riseVelocityDropAccumulator = 0.0f;
    dashFramesRemaining = 0;
    dashCooldownFrames = 0;
    dashDirection = 1;
    hitFeedback = game::HitFeedbackState();
    hitFeedback.blinking = false;
    facing = game::FacingDirection::Right;
    framesSinceLastDamage = kHealLockoutFrames;
    blinkFramesRemaining = 0;
    healCast = HealCastState();
    healCast.totalFrames = kHealCastFrames;
    upWaveCast = UpWaveCastState();
    upWaveCast.totalFrames = kUpWaveCastFrames;
    downSlamCast = DownSlamCastState();
    downSlamCast.totalFrames = kDownSlamCastFrames;
    meleeVisual = MeleeVisualState();
    meleeVisual.totalFrames = kMeleeVisualFrames;
    deathAnimation = DeathAnimationState();
    rewardPopup = RewardPopupState();
    projectiles.clear();
    previousKeys.clear();
    lastAction = "Ready";
    lastResult = "Move with A/D, jump with SPACE, dash with SHIFT.";
    resetQueued = false;
}

bool Player::isGrounded(const std::string& currentmap, size_t pos) {
    const size_t width = lineWidth(currentmap);
    if (width == 0) {
        return false;
    }

    const size_t belowPos = pos + width;
    if (belowPos >= currentmap.length()) {
        return false;
    }

    return currentmap[belowPos] != ' ';
}

bool Player::applyGravity(std::string& currentmap, size_t pos) {
    const size_t width = lineWidth(currentmap);
    if (width == 0) {
        return false;
    }

    const size_t belowPos = pos + width;
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
    const size_t width = lineWidth(currentmap);
    if (width == 0 || pos < width) {
        return false;
    }

    const size_t abovePos = pos - width;
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

    updateFacingFromInput();

    const bool movingLeft = isKeyDown('a') || isKeyDown('A');
    const bool movingRight = isKeyDown('d') || isKeyDown('D');
    const bool jumpHeld = isKeyDown(' ');
    const bool jumpJustPressed = jumpHeld && !jumpHeldLastFrame;
    const bool dashJustPressed = isJustPressed(0x10);

    bool grounded = isGrounded(currentmap, pos);

    if (dashCooldownFrames > 0) {
        dashCooldownFrames--;
    }

    if (grounded && !isJumping && upwardVelocity <= 0.0f) {
        downwardVelocity = 0.0f;
        verticalMoveAccumulator = 0.0f;
        airJumpAvailable = true;
        if (!dashActive && dashCooldownFrames <= 0) {
            dashAvailable = true;
        }
    }

    if (jumpJustPressed && grounded) {
        isJumping = true;
        upwardVelocity = kInitialJumpUpwardSpeed;
        downwardVelocity = 0.0f;
        jumpHoldRemaining = kJumpInitialHoldTime;
        minimumJumpRiseRemaining = kMinimumJumpRiseTime;
        riseVelocityDropAccumulator = 0.0f;
        verticalMoveAccumulator = 0.0f;
        dashActive = false;
        dashFramesRemaining = 0;
        dashMoveAccumulator = 0.0f;
        grounded = false;
    } else if (jumpJustPressed && !grounded && doubleJumpUnlocked && airJumpAvailable && !dashActive) {
        isJumping = true;
        upwardVelocity = kDoubleJumpUpwardSpeed;
        downwardVelocity = 0.0f;
        jumpHoldRemaining = kJumpInitialHoldTime;
        minimumJumpRiseRemaining = kMinimumJumpRiseTime;
        riseVelocityDropAccumulator = 0.0f;
        verticalMoveAccumulator = 0.0f;
        airJumpAvailable = false;
        lastAction = "Double Jump";
        lastResult = "Air jump triggered.";
    }

    if (dashJustPressed && dashAvailable && !dashActive) {
        dashDirection = movingLeft == movingRight
                ? (facing == game::FacingDirection::Left ? -1 : 1)
                : (movingLeft ? -1 : 1);
        dashActive = true;
        dashAvailable = false;
        dashFramesRemaining = kDashFrames;
        dashCooldownFrames = kDashCooldownFrames;
        dashMoveAccumulator = 0.0f;
        isJumping = false;
        upwardVelocity = 0.0f;
        downwardVelocity = 0.0f;
        jumpHoldRemaining = 0.0f;
        minimumJumpRiseRemaining = 0.0f;
        riseVelocityDropAccumulator = 0.0f;
        verticalMoveAccumulator = 0.0f;
        facing = dashDirection < 0 ? game::FacingDirection::Left : game::FacingDirection::Right;
        lastAction = "Dash";
        lastResult = dashDirection < 0 ? "Dashed left." : "Dashed right.";
    }

    if (dashActive) {
        dashMoveAccumulator += kDashSpeed * kFixedDeltaSeconds;
        while (dashMoveAccumulator >= 1.0f) {
            if (!moveHorizontallyOneTile(currentmap, dashDirection)) {
                dashFramesRemaining = 0;
                dashMoveAccumulator = 0.0f;
                break;
            }
            dashMoveAccumulator -= 1.0f;
        }

        if (dashFramesRemaining > 0) {
            dashFramesRemaining--;
        }
        if (dashFramesRemaining <= 0) {
            dashActive = false;
            dashMoveAccumulator = 0.0f;
        }

        jumpHeldLastFrame = jumpHeld;
        return;
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
            if (movingLeft) {
                moved = moveHorizontallyOneTile(currentmap, -1);
            } else if (movingRight) {
                moved = moveHorizontallyOneTile(currentmap, 1);
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
                const size_t currentPos = currentmap.find('@');
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
            const size_t currentPos = currentmap.find('@');
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

void Player::updateCombat(const std::string& gameplayMap,
                          game::GroundEnemy& groundEnemy,
                          game::FlyingEnemy& flyingEnemy) {
    if (deathAnimation.active) {
        updateDeathAnimation();
        previousKeys = ksm.keyStates;
        return;
    }

    updateBlink();
    updateRewardPopup();
    updateFacingFromInput();

    if (!isAlive()) {
        const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
        if (playerPosition.x >= 0 && playerPosition.y >= 0) {
            triggerDeathAnimation(playerPosition, "Unknown source");
        }
        previousKeys = ksm.keyStates;
        return;
    }

    const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
    if (playerPosition.x < 0 || playerPosition.y < 0) {
        previousKeys = ksm.keyStates;
        return;
    }

    setCombatPosition(playerPosition);

    const game::CombatSystem combatSystem;

    const bool aimingUp = isKeyDown('w') || isKeyDown('W');
    const bool aimingDown = isKeyDown('s') || isKeyDown('S');
    const bool physicalPressed = isJustPressed('j') || isJustPressed('J');
    const bool spellPressed = isJustPressed('k') || isJustPressed('K');

    if (!isMovementLocked() && physicalPressed) {
        if (aimingUp) {
            startMeleeVisual(MeleeVisualType::Up, playerPosition);
            lastAction = "Up Slash";
            game::AttackDefinition attack;
            attack.damage = game::DamageInfo(kPlayerAttackDamage, game::DamageType::UpSlash, id, true);
            const bool groundHit = applyEnemyCombatResolution(
                    groundEnemy,
                    combatSystem.resolveAttackInVerticalRange(*this, groundEnemy, attack, true),
                    combatSystem);
            const bool flyingHit = applyEnemyCombatResolution(
                    flyingEnemy,
                    combatSystem.resolveAttackInVerticalRange(*this, flyingEnemy, attack, true),
                    combatSystem);
            if (groundHit || flyingHit) {
                stats.soul.current = std::min(stats.soul.maximum, stats.soul.current + kSoulGainOnMeleeHit);
                lastResult = "Up slash connected. Soul +11.";
            } else {
                lastResult = "Slash triggered. No target in range.";
            }
        } else if (aimingDown) {
            startMeleeVisual(MeleeVisualType::Down, playerPosition);
            lastAction = "Down Slash";
            game::AttackDefinition attack;
            attack.damage = game::DamageInfo(kPlayerAttackDamage, game::DamageType::DownSlash, id, true);
            const bool groundHit = applyEnemyCombatResolution(
                    groundEnemy,
                    combatSystem.resolveAttackInVerticalRange(*this, groundEnemy, attack, false),
                    combatSystem);
            const bool flyingHit = applyEnemyCombatResolution(
                    flyingEnemy,
                    combatSystem.resolveAttackInVerticalRange(*this, flyingEnemy, attack, false),
                    combatSystem);
            if (groundHit || flyingHit) {
                stats.soul.current = std::min(stats.soul.maximum, stats.soul.current + kSoulGainOnMeleeHit);
                lastResult = "Down slash connected. Soul +11.";
            } else {
                lastResult = "Slash triggered. No target in range.";
            }
        } else {
            startMeleeVisual(MeleeVisualType::Horizontal, playerPosition);
            lastAction = "Basic Attack";
            game::AttackDefinition attack;
            attack.damage = game::DamageInfo(kPlayerAttackDamage, game::DamageType::BasicAttack, id, true);
            const bool groundHit = applyEnemyCombatResolution(
                    groundEnemy,
                    combatSystem.resolveAttackInFrontRange(*this, groundEnemy, attack, kAttackRange, 1),
                    combatSystem);
            const bool flyingHit = applyEnemyCombatResolution(
                    flyingEnemy,
                    combatSystem.resolveAttackInFrontRange(*this, flyingEnemy, attack, kAttackRange, 1),
                    combatSystem);
            if (groundHit || flyingHit) {
                stats.soul.current = std::min(stats.soul.maximum, stats.soul.current + kSoulGainOnMeleeHit);
                lastResult = "Slash connected. Soul +11.";
            } else {
                lastResult = "No target in range.";
            }
        }
    }

    if (!isMovementLocked() && spellPressed) {
        if (aimingUp) {
            castUpWave(playerPosition);
        } else if (aimingDown) {
            castDownSlam(gameplayMap, playerPosition);
        } else {
            castHorizontalWave(playerPosition);
        }
    }

    if (!isMovementLocked() && (isJustPressed('r') || isJustPressed('R'))) {
        startHealCast();
    }

    updateDownSlamCast(gameplayMap, groundEnemy, flyingEnemy);
    updateUpWaveCast(gameplayMap, groundEnemy, flyingEnemy);
    updateMeleeVisual();
    updateProjectiles(gameplayMap, groundEnemy, flyingEnemy);
    updateHealCast();

    stats.soul.maximum = kSoulMeterMax;
    if (stats.soul.current > stats.soul.maximum) {
        stats.soul.current = stats.soul.maximum;
    }

    previousKeys = ksm.keyStates;
}

void Player::receiveDamage(const std::string& sourceLabel, const game::Position& playerPosition) {
    const game::Position resolvedPosition = playerPosition.x >= 0 && playerPosition.y >= 0
            ? playerPosition
            : combatPosition;
    setCombatPosition(resolvedPosition);
    takeDamage(game::DamageInfo(1, game::DamageType::Contact, sourceLabel, true));
}

void Player::applyIncomingDamage(const game::DamageInfo& damageInfo,
                                 const game::Position& playerPosition) {
    if (blinkFramesRemaining > 0 ||
        stats.health.current <= 0 ||
        deathAnimation.active ||
        damageInfo.amount <= 0) {
        return;
    }

    const std::string sourceLabel = damageInfo.sourceId.empty() ? "Unknown source" : damageInfo.sourceId;
    const int damageAmount = std::max(0, damageInfo.amount);
    stats.health.current = std::max(0, stats.health.current - damageAmount);
    framesSinceLastDamage = 0;
    blinkFramesRemaining = kBlinkFrames;
    hitFeedback.blinking = true;
    healCast.active = false;
    lastAction = "Damaged";

    if (stats.health.current == 0) {
        lastResult = sourceLabel + " defeated the player.";
        triggerDeathAnimation(playerPosition, sourceLabel);
    } else {
        lastResult = sourceLabel + " dealt " + std::to_string(damageAmount) + " damage.";
    }
}

bool Player::isMovementLocked() const {
    return healCast.active || downSlamCast.active;
}

bool Player::isVisible() const {
    return blinkFramesRemaining <= 0 || ((blinkFramesRemaining / 5) % 2 == 0);
}

bool Player::isAlive() const {
    return stats.health.current > 0;
}

bool Player::consumeResetRequest() {
    if (!resetQueued) {
        return false;
    }

    resetQueued = false;
    return true;
}

const game::CharacterStats& Player::getStats() const {
    return stats;
}

const std::string& Player::getId() const {
    return id;
}

game::Position Player::getPosition() const {
    return combatPosition;
}

game::CharacterStats& Player::accessStats() {
    return stats;
}

game::HitFeedbackState& Player::accessHitFeedback() {
    return hitFeedback;
}

void Player::takeDamage(const game::DamageInfo& damageInfo) {
    applyIncomingDamage(damageInfo, combatPosition);
}

void Player::restoreSavedStats(const game::CharacterStats& savedStats) {
    resetRuntimeState();
    stats = savedStats;
    if (stats.health.maximum <= 0) {
        stats.health.maximum = 5;
    }
    if (stats.soul.maximum <= 0) {
        stats.soul.maximum = kSoulMeterMax;
    }
    stats.health.current = std::max(1, std::min(stats.health.current, stats.health.maximum));
    stats.soul.current = std::max(0, std::min(stats.soul.current, stats.soul.maximum));
    if (stats.hkd < 0) {
        stats.hkd = 0;
    }
}

game::FacingDirection Player::getFacingDirection() const {
    return facing;
}

void Player::setCombatPosition(const game::Position& position) {
    combatPosition = position;
}

void Player::setDoubleJumpUnlocked(bool unlocked) {
    doubleJumpUnlocked = unlocked;
    if (!doubleJumpUnlocked) {
        airJumpAvailable = false;
    }
}

bool Player::hasDoubleJumpUnlocked() const {
    return doubleJumpUnlocked;
}

std::string Player::buildHud() const {
    return buildHud(std::string());
}

std::string Player::buildHud(const std::string& locationName) const {
    std::ostringstream hud;
    const std::vector<std::string> soulLines = buildSoulVesselLines();
    for (size_t index = 0; index < soulLines.size(); ++index) {
        hud << soulLines[index] << "\n";
    }
    hud << buildHealthOrbLine() << "\n";
    hud << "HKD " << stats.hkd;
    if (!locationName.empty()) {
        hud << " | Location " << locationName;
    }
    hud << "\n\n";
    return hud.str();
}

void Player::overlayRender(std::string& renderMap, const std::string& gameplayMap) const {
    const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
    if (playerPosition.x >= 0 && playerPosition.y >= 0 && healCast.active) {
        const int stage = (healCast.elapsedFrames / 6) % 4;
        if (stage == 0) {
            placeGlyph(renderMap, game::Position(playerPosition.x - 1, playerPosition.y), '.');
            placeGlyph(renderMap, game::Position(playerPosition.x + 1, playerPosition.y), '.');
        } else if (stage == 1) {
            placeGlyph(renderMap, game::Position(playerPosition.x - 1, playerPosition.y - 1), ':');
            placeGlyph(renderMap, game::Position(playerPosition.x + 1, playerPosition.y - 1), ':');
        } else if (stage == 2) {
            placeGlyph(renderMap, game::Position(playerPosition.x, playerPosition.y - 1), '*');
            placeGlyph(renderMap, game::Position(playerPosition.x - 1, playerPosition.y - 2), '*');
            placeGlyph(renderMap, game::Position(playerPosition.x + 1, playerPosition.y - 2), '*');
        } else {
            placeGlyph(renderMap, game::Position(playerPosition.x, playerPosition.y - 2), '\'');
            placeGlyph(renderMap, game::Position(playerPosition.x, playerPosition.y - 3), '.');
        }
    }

    if (playerPosition.x >= 0 && playerPosition.y >= 0 && dashActive) {
        const std::vector<SpellVisualCell> cells = buildDashVisualCells();
        for (size_t index = 0; index < cells.size(); ++index) {
            const game::Position target(playerPosition.x + cells[index].dx,
                                        playerPosition.y + cells[index].dy);
            if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
                continue;
            }
            placeGlyph(renderMap, target, cells[index].glyph);
        }
    }

    if (playerPosition.x >= 0 && playerPosition.y >= 0 && meleeVisual.active) {
        const std::vector<SpellVisualCell> cells = buildMeleeVisualCells();
        for (size_t index = 0; index < cells.size(); ++index) {
            const game::Position target(meleeVisual.origin.x + cells[index].dx,
                                        meleeVisual.origin.y + cells[index].dy);
            if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
                continue;
            }
            placeGlyph(renderMap, target, cells[index].glyph);
        }
    }

    if (playerPosition.x >= 0 && playerPosition.y >= 0 && upWaveCast.active) {
        const std::vector<SpellVisualCell> cells = buildUpWaveVisualCells();
        for (size_t index = 0; index < cells.size(); ++index) {
            const game::Position target(upWaveCast.origin.x + cells[index].dx,
                                        upWaveCast.origin.y + cells[index].dy);
            if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
                continue;
            }
            placeGlyph(renderMap, target, cells[index].glyph);
        }
    }

    if (playerPosition.x >= 0 && playerPosition.y >= 0 && downSlamCast.active) {
        const std::vector<SpellVisualCell> cells = buildDownSlamVisualCells();
        for (size_t index = 0; index < cells.size(); ++index) {
            const game::Position target(downSlamCast.origin.x + cells[index].dx,
                                        downSlamCast.origin.y + cells[index].dy);
            if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
                continue;
            }
            placeGlyph(renderMap, target, cells[index].glyph);
        }
    }

    for (size_t index = 0; index < projectiles.size(); ++index) {
        const std::vector<SpellVisualCell> cells = buildHorizontalWaveVisualCells(projectiles[index]);
        for (size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
            const game::Position target(projectiles[index].position.x + cells[cellIndex].dx,
                                        projectiles[index].position.y + cells[cellIndex].dy);
            if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
                continue;
            }
            placeGlyph(renderMap, target, cells[cellIndex].glyph);
        }
    }

    if (playerPosition.x >= 0 && playerPosition.y >= 0) {
        if (isVisible()) {
            placeGlyph(renderMap, playerPosition, '@');
        } else {
            placeGlyph(renderMap, playerPosition, ' ');
        }
    }

    if (playerPosition.x >= 0 && playerPosition.y >= 0 && rewardPopup.active) {
        const int riseOffset = rewardPopup.elapsedFrames / 8;
        const std::string amountText = "+" + std::to_string(rewardPopup.amount);
        const game::Position popupPosition(
                playerPosition.x - static_cast<int>(amountText.length() / 2),
                playerPosition.y - 2 - riseOffset);
        placeText(renderMap, popupPosition, amountText);
    }

    applyDeathAnimationOverlay(renderMap);
}

bool Player::isKeyDown(int keyCode) const {
    const std::unordered_map<int, bool>::const_iterator it = ksm.keyStates.find(keyCode);
    return it != ksm.keyStates.end() && it->second;
}

bool Player::wasKeyDown(int keyCode) const {
    const std::unordered_map<int, bool>::const_iterator it = previousKeys.find(keyCode);
    return it != previousKeys.end() && it->second;
}

bool Player::isJustPressed(int keyCode) const {
    return isKeyDown(keyCode) && !wasKeyDown(keyCode);
}

bool Player::canSpendSoul(int soulCost) {
    if (stats.soul.current < soulCost) {
        return false;
    }

    stats.soul.current -= soulCost;
    return true;
}

void Player::updateFacingFromInput() {
    if (isKeyDown('a') || isKeyDown('A')) {
        facing = game::FacingDirection::Left;
    } else if (isKeyDown('d') || isKeyDown('D')) {
        facing = game::FacingDirection::Right;
    }
}

void Player::updateBlink() {
    if (deathAnimation.active) {
        return;
    }

    if (framesSinceLastDamage < kHealLockoutFrames) {
        framesSinceLastDamage++;
    }

    if (blinkFramesRemaining > 0) {
        blinkFramesRemaining--;
    }

    if (blinkFramesRemaining <= 0) {
        blinkFramesRemaining = 0;
        hitFeedback.blinking = false;
    }
}

void Player::updateDeathAnimation() {
    if (!deathAnimation.active || resetQueued) {
        return;
    }

    deathAnimation.elapsedFrames++;
    if (deathAnimation.elapsedFrames >= deathAnimationTotalFrames()) {
        deathAnimation.elapsedFrames = deathAnimationTotalFrames() - 1;
        resetQueued = true;
        lastAction = "YOU DEAD";
        lastResult = "The room will reset.";
    } else if (deathAnimation.elapsedFrames >= kDeathFreezeFrames + kDeathDotFrames + kDeathExpandFrames + kDeathFullWhiteFrames) {
        lastAction = "YOU DEAD";
        lastResult = "The white screen holds for a moment.";
    } else if (deathAnimation.elapsedFrames >= kDeathFreezeFrames + kDeathDotFrames + kDeathExpandFrames) {
        lastAction = "Death Bloom";
        lastResult = "The screen is swallowed by white.";
    } else if (deathAnimation.elapsedFrames >= kDeathFreezeFrames + kDeathDotFrames) {
        lastAction = "Death Bloom";
        lastResult = "White rings expand from the shell.";
    } else if (deathAnimation.elapsedFrames >= kDeathFreezeFrames) {
        lastAction = "Death Bloom";
        lastResult = "A white point remains.";
    }
}

void Player::updateRewardPopup() {
    if (!rewardPopup.active) {
        return;
    }

    rewardPopup.elapsedFrames++;
    if (rewardPopup.elapsedFrames >= rewardPopup.totalFrames) {
        rewardPopup.active = false;
        rewardPopup.elapsedFrames = 0;
        rewardPopup.amount = 0;
    }
}

void Player::updateHealCast() {
    if (!healCast.active) {
        return;
    }

    healCast.elapsedFrames++;
    lastAction = "Heal";

    if (healCast.elapsedFrames >= healCast.totalFrames) {
        healCast.active = false;
        stats.health.current = std::min(stats.health.maximum, stats.health.current + 1);
        lastResult = "Heal completed. Restored 1 mask.";
        return;
    }

    lastResult = "Channeling soul heal...";
}

void Player::updateUpWaveCast(const std::string& gameplayMap,
                              game::GroundEnemy& groundEnemy,
                              game::FlyingEnemy& flyingEnemy) {
    if (!upWaveCast.active) {
        return;
    }

    upWaveCast.elapsedFrames++;
    const int stage = upWaveStage();

    if (!upWaveCast.damageResolved && stage >= 3) {
        const game::CombatSystem combatSystem;
        game::AttackDefinition attack;
        attack.damage = game::DamageInfo(kPlayerAttackDamage, game::DamageType::SoulWaveUp, id, true);
        const std::vector<game::Position> damageCells = buildUpWaveDamageCells(gameplayMap, stage >= 4);
        for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
            const bool hitGround = applyEnemyCombatResolution(
                    groundEnemy,
                    combatSystem.resolveAttackAtPosition(groundEnemy, damageCells[cellIndex], attack),
                    combatSystem);
            const bool hitFlying = applyEnemyCombatResolution(
                    flyingEnemy,
                    combatSystem.resolveAttackAtPosition(flyingEnemy, damageCells[cellIndex], attack),
                    combatSystem);
            if (hitGround || hitFlying) {
                lastAction = "Up Soul Wave";
                lastResult = "Soul crown connected.";
                break;
            }
        }
        upWaveCast.damageResolved = true;
    }

    if (upWaveCast.elapsedFrames >= upWaveCast.totalFrames) {
        upWaveCast.active = false;
        return;
    }

    if (!upWaveCast.damageResolved) {
        lastAction = "Up Soul Wave";
        if (stage <= 1) {
            lastResult = "Soul gathers above the head.";
        } else if (stage <= 3) {
            lastResult = "Soul column surges upward.";
        } else {
            lastResult = "Soul crown bursts overhead.";
        }
    }
}

void Player::updateDownSlamCast(const std::string& gameplayMap,
                                game::GroundEnemy& groundEnemy,
                                game::FlyingEnemy& flyingEnemy) {
    if (!downSlamCast.active) {
        return;
    }

    downSlamCast.elapsedFrames++;
    const int stage = downSlamStage();

    if (!downSlamCast.damageResolved && stage >= 2) {
        const game::CombatSystem combatSystem;
        game::AttackDefinition attack;
        attack.damage = game::DamageInfo(kPlayerAttackDamage, game::DamageType::SoulSlam, id, true);
        const std::vector<game::Position> damageCells = buildDownSlamDamageCells(gameplayMap);
        for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
            applyEnemyCombatResolution(
                    groundEnemy,
                    combatSystem.resolveAttackAtPosition(groundEnemy, damageCells[cellIndex], attack),
                    combatSystem);
            applyEnemyCombatResolution(
                    flyingEnemy,
                    combatSystem.resolveAttackAtPosition(flyingEnemy, damageCells[cellIndex], attack),
                    combatSystem);
        }
        downSlamCast.damageResolved = true;
        lastAction = "Down Slam";
        lastResult = "The ground bursts from the impact.";
    }

    if (downSlamCast.elapsedFrames >= downSlamCast.totalFrames) {
        downSlamCast.active = false;
        return;
    }

    if (!downSlamCast.damageResolved) {
        lastAction = "Down Slam";
        if (stage == 0) {
            lastResult = "Black force gathers below the feet.";
        } else {
            lastResult = "The black spike drives downward.";
        }
    }

    (void)gameplayMap;
}

void Player::updateMeleeVisual() {
    if (!meleeVisual.active) {
        return;
    }

    meleeVisual.elapsedFrames++;
    if (meleeVisual.elapsedFrames >= meleeVisual.totalFrames) {
        meleeVisual.active = false;
    }
}

void Player::updateProjectiles(const std::string& gameplayMap,
                               game::GroundEnemy& groundEnemy,
                               game::FlyingEnemy& flyingEnemy) {
    std::vector<VisualProjectile> activeProjectiles;
    const game::CombatSystem combatSystem;

    for (size_t index = 0; index < projectiles.size(); ++index) {
        VisualProjectile projectile = projectiles[index];
        projectile.position.x += projectile.dx;
        projectile.position.y += projectile.dy;
        projectile.remainingFrames--;

        if (projectile.remainingFrames <= 0) {
            continue;
        }

        if (!isInsidePlayableArea(gameplayMap, projectile.position)) {
            continue;
        }

        if (tileAt(gameplayMap, projectile.position) == '=') {
            continue;
        }

        const game::DamageType damageType = projectile.dy < 0
                ? game::DamageType::SoulWaveUp
                : game::DamageType::SoulWaveHorizontal;
        game::AttackDefinition attack;
        attack.damage = game::DamageInfo(kPlayerAttackDamage, damageType, id, true);
        if (applyEnemyCombatResolution(
                    groundEnemy,
                    combatSystem.resolveAttackAtPosition(groundEnemy, projectile.position, attack),
                    combatSystem) ||
            applyEnemyCombatResolution(
                    flyingEnemy,
                    combatSystem.resolveAttackAtPosition(flyingEnemy, projectile.position, attack),
                    combatSystem)) {
            lastAction = projectile.label;
            lastResult = "Projectile hit the target.";
            continue;
        }

        activeProjectiles.push_back(projectile);
    }

    projectiles.swap(activeProjectiles);
}

void Player::castHorizontalWave(const game::Position& playerPosition) {
    lastAction = "Horizontal Soul Wave";
    if (!canSpendSoul(kSoulSkillCost)) {
        lastResult = "Not enough soul.";
        return;
    }

    VisualProjectile projectile;
    projectile.position = game::Position(playerPosition.x + (facing == game::FacingDirection::Right ? 1 : -1), playerPosition.y);
    projectile.dx = facing == game::FacingDirection::Right ? 1 : -1;
    projectile.dy = 0;
    projectile.remainingFrames = 28;
    projectile.totalFrames = projectile.remainingFrames;
    projectile.label = "Horizontal Soul Wave";
    projectiles.push_back(projectile);
    lastResult = "Soul wave launched.";
}

void Player::castUpWave(const game::Position& playerPosition) {
    lastAction = "Up Soul Wave";
    if (upWaveCast.active) {
        lastResult = "Up wave is already active.";
        return;
    }

    if (!canSpendSoul(kSoulSkillCost)) {
        lastResult = "Not enough soul.";
        return;
    }

    upWaveCast.active = true;
    upWaveCast.damageResolved = false;
    upWaveCast.elapsedFrames = 0;
    upWaveCast.totalFrames = kUpWaveCastFrames;
    upWaveCast.origin = playerPosition;
    lastResult = "Soul gathers above the head.";
}

void Player::castDownSlam(const std::string& gameplayMap, const game::Position& playerPosition) {
    lastAction = "Down Slam";
    if (downSlamCast.active) {
        lastResult = "Down slam is already active.";
        return;
    }

    if (!canSpendSoul(kSoulSkillCost)) {
        lastResult = "Not enough soul.";
        return;
    }

    downSlamCast.active = true;
    downSlamCast.damageResolved = false;
    downSlamCast.elapsedFrames = 0;
    downSlamCast.totalFrames = kDownSlamCastFrames;
    downSlamCast.origin = playerPosition;
    downSlamCast.impact = findDownSlamImpactPosition(gameplayMap, playerPosition);
    lastResult = "Black force gathers below the feet.";
}

void Player::startHealCast() {
    lastAction = "Heal";
    if (healCast.active) {
        lastResult = "Already channeling heal.";
        return;
    }

    if (stats.health.current >= stats.health.maximum) {
        lastResult = "Health is already full.";
        return;
    }

    if (framesSinceLastDamage < kHealLockoutFrames) {
        lastResult = "Heal interrupted: wait 3 seconds after taking damage.";
        return;
    }

    if (!canSpendSoul(kSoulSkillCost)) {
        lastResult = "Not enough soul.";
        return;
    }

    healCast.active = true;
    healCast.elapsedFrames = 0;
    healCast.totalFrames = kHealCastFrames;
    lastResult = "Channeling soul heal.";
}

void Player::startMeleeVisual(MeleeVisualType type, const game::Position& origin) {
    meleeVisual.active = true;
    meleeVisual.elapsedFrames = 0;
    meleeVisual.totalFrames = kMeleeVisualFrames;
    meleeVisual.type = type;
    meleeVisual.origin = origin;
    meleeVisual.facing = facing;
}

void Player::triggerDeathAnimation(const game::Position& center, const std::string& sourceLabel) {
    if (deathAnimation.active) {
        return;
    }

    deathAnimation.active = true;
    deathAnimation.elapsedFrames = 0;
    deathAnimation.center = center;
    projectiles.clear();
    healCast.active = false;
    upWaveCast.active = false;
    downSlamCast.active = false;
    meleeVisual.active = false;
    rewardPopup.active = false;
    blinkFramesRemaining = 0;
    hitFeedback.blinking = false;
    isJumping = false;
    airJumpAvailable = false;
    dashActive = false;
    dashAvailable = false;
    horizontalMoveAccumulator = 0.0f;
    verticalMoveAccumulator = 0.0f;
    dashMoveAccumulator = 0.0f;
    upwardVelocity = 0.0f;
    downwardVelocity = 0.0f;
    jumpHoldRemaining = 0.0f;
    minimumJumpRiseRemaining = 0.0f;
    riseVelocityDropAccumulator = 0.0f;
    dashFramesRemaining = 0;
    dashCooldownFrames = 0;
    lastAction = "Death";
    lastResult = sourceLabel + " defeated the player.";
}

int Player::upWaveStage() const {
    if (!upWaveCast.active || upWaveCast.totalFrames <= 0) {
        return -1;
    }

    int stage = (upWaveCast.elapsedFrames * 6) / upWaveCast.totalFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 5) {
        stage = 5;
    }
    return stage;
}

int Player::downSlamStage() const {
    if (!downSlamCast.active || downSlamCast.totalFrames <= 0) {
        return -1;
    }

    int stage = (downSlamCast.elapsedFrames * 4) / downSlamCast.totalFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 3) {
        stage = 3;
    }
    return stage;
}

int Player::meleeVisualStage() const {
    if (!meleeVisual.active || meleeVisual.totalFrames <= 0) {
        return -1;
    }

    int stage = (meleeVisual.elapsedFrames * 2) / meleeVisual.totalFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 1) {
        stage = 1;
    }
    return stage;
}

int Player::deathExpansionRadius() const {
    if (!deathAnimation.active) {
        return -1;
    }

    const int expandElapsed = deathAnimation.elapsedFrames - (kDeathFreezeFrames + kDeathDotFrames);
    if (expandElapsed < 0) {
        return 0;
    }

    static const int radii[] = {1, 2, 4, 6, 9, 13};
    const int stageCount = static_cast<int>(sizeof(radii) / sizeof(radii[0]));
    int stage = (expandElapsed * stageCount) / kDeathExpandFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage >= stageCount) {
        stage = stageCount - 1;
    }
    return radii[stage];
}

void Player::applyCombatReward(const game::RewardResolution& reward,
                               const game::CombatSystem& combatSystem) {
    if (reward.hkdGranted <= 0 && reward.soulGranted <= 0) {
        return;
    }

    combatSystem.applyReward(stats, reward);
    if (reward.hkdGranted > 0) {
        rewardPopup.active = true;
        rewardPopup.elapsedFrames = 0;
        rewardPopup.totalFrames = 24;
        rewardPopup.amount = reward.hkdGranted;
        lastResult = "Enemy defeated. HKD +" + std::to_string(reward.hkdGranted) + ".";
    }
}

bool Player::applyEnemyCombatResolution(const game::Enemy& enemy,
                                        const game::DamageResolution& resolution,
                                        const game::CombatSystem& combatSystem) {
    if (!resolution.hitApplied) {
        return false;
    }

    applyCombatReward(combatSystem.buildEnemyRewardOnDefeat(enemy, resolution), combatSystem);
    return true;
}

game::Position Player::findDownSlamImpactPosition(const std::string& gameplayMap,
                                                  const game::Position& origin) const {
    game::Position impact = origin;

    for (int y = origin.y + 1; y < 200; ++y) {
        const game::Position candidate(origin.x, y);
        if (!isInsidePlayableArea(gameplayMap, candidate)) {
            break;
        }
        if (tileAt(gameplayMap, candidate) == '=') {
            break;
        }
        impact = candidate;
    }

    return impact;
}

std::vector<Player::SpellVisualCell> Player::buildHorizontalWaveVisualCells(const VisualProjectile& projectile) const {
    std::vector<SpellVisualCell> cells;
    if (projectile.dx == 0) {
        return cells;
    }

    const int age = projectile.totalFrames - projectile.remainingFrames;
    const bool facingRight = projectile.dx > 0;

    if (age <= 2) {
        if (facingRight) {
            cells.push_back(SpellVisualCell(-2, 0, '-'));
            cells.push_back(SpellVisualCell(-1, 0, '='));
            cells.push_back(SpellVisualCell(0, 0, '>'));
        } else {
            cells.push_back(SpellVisualCell(0, 0, '<'));
            cells.push_back(SpellVisualCell(1, 0, '='));
            cells.push_back(SpellVisualCell(2, 0, '-'));
        }
        return cells;
    }

    if (projectile.remainingFrames <= 4) {
        if (facingRight) {
            cells.push_back(SpellVisualCell(-2, 0, '~'));
            cells.push_back(SpellVisualCell(-1, 0, '~'));
            cells.push_back(SpellVisualCell(0, 0, '>'));
        } else {
            cells.push_back(SpellVisualCell(0, 0, '<'));
            cells.push_back(SpellVisualCell(1, 0, '~'));
            cells.push_back(SpellVisualCell(2, 0, '~'));
        }
        return cells;
    }

    if (facingRight) {
        cells.push_back(SpellVisualCell(-3, 0, '~'));
        cells.push_back(SpellVisualCell(-2, 0, '='));
        cells.push_back(SpellVisualCell(-1, 0, '='));
        cells.push_back(SpellVisualCell(0, 0, '>'));
        cells.push_back(SpellVisualCell(-2, -1, '.'));
        cells.push_back(SpellVisualCell(-2, 1, '\''));
    } else {
        cells.push_back(SpellVisualCell(0, 0, '<'));
        cells.push_back(SpellVisualCell(1, 0, '='));
        cells.push_back(SpellVisualCell(2, 0, '='));
        cells.push_back(SpellVisualCell(3, 0, '~'));
        cells.push_back(SpellVisualCell(2, -1, '.'));
        cells.push_back(SpellVisualCell(2, 1, '\''));
    }

    return cells;
}

std::vector<Player::SpellVisualCell> Player::buildUpWaveVisualCells() const {
    std::vector<SpellVisualCell> cells;
    switch (upWaveStage()) {
    case 0:
        cells.push_back(SpellVisualCell(-1, -2, '.'));
        cells.push_back(SpellVisualCell(0, -2, '^'));
        cells.push_back(SpellVisualCell(1, -2, '.'));
        cells.push_back(SpellVisualCell(-1, -1, '/'));
        cells.push_back(SpellVisualCell(1, -1, '\\'));
        break;
    case 1:
        cells.push_back(SpellVisualCell(0, -3, '^'));
        cells.push_back(SpellVisualCell(-1, -2, '.'));
        cells.push_back(SpellVisualCell(0, -2, '*'));
        cells.push_back(SpellVisualCell(1, -2, '.'));
        cells.push_back(SpellVisualCell(0, -1, '!'));
        break;
    case 2:
        cells.push_back(SpellVisualCell(0, -4, '^'));
        cells.push_back(SpellVisualCell(-1, -3, '.'));
        cells.push_back(SpellVisualCell(0, -3, '!'));
        cells.push_back(SpellVisualCell(1, -3, '.'));
        cells.push_back(SpellVisualCell(0, -2, '!'));
        cells.push_back(SpellVisualCell(0, -1, '!'));
        break;
    case 3:
        cells.push_back(SpellVisualCell(-1, -4, '\\'));
        cells.push_back(SpellVisualCell(0, -4, '|'));
        cells.push_back(SpellVisualCell(1, -4, '/'));
        cells.push_back(SpellVisualCell(-2, -3, '<'));
        cells.push_back(SpellVisualCell(-1, -3, '-'));
        cells.push_back(SpellVisualCell(0, -3, '!'));
        cells.push_back(SpellVisualCell(1, -3, '-'));
        cells.push_back(SpellVisualCell(2, -3, '>'));
        cells.push_back(SpellVisualCell(-1, -2, '\\'));
        cells.push_back(SpellVisualCell(0, -2, '|'));
        cells.push_back(SpellVisualCell(1, -2, '/'));
        cells.push_back(SpellVisualCell(0, -1, '!'));
        break;
    case 4:
        cells.push_back(SpellVisualCell(-2, -5, '\\'));
        cells.push_back(SpellVisualCell(-1, -5, '^'));
        cells.push_back(SpellVisualCell(0, -5, '^'));
        cells.push_back(SpellVisualCell(1, -5, '^'));
        cells.push_back(SpellVisualCell(2, -5, '/'));
        cells.push_back(SpellVisualCell(-3, -4, '<'));
        cells.push_back(SpellVisualCell(-2, -4, '-'));
        cells.push_back(SpellVisualCell(-1, -4, '*'));
        cells.push_back(SpellVisualCell(0, -4, '*'));
        cells.push_back(SpellVisualCell(1, -4, '*'));
        cells.push_back(SpellVisualCell(2, -4, '-'));
        cells.push_back(SpellVisualCell(3, -4, '>'));
        cells.push_back(SpellVisualCell(-1, -3, '\\'));
        cells.push_back(SpellVisualCell(0, -3, '|'));
        cells.push_back(SpellVisualCell(1, -3, '/'));
        cells.push_back(SpellVisualCell(-1, -2, '\\'));
        cells.push_back(SpellVisualCell(0, -2, '|'));
        cells.push_back(SpellVisualCell(1, -2, '/'));
        cells.push_back(SpellVisualCell(0, -1, '!'));
        break;
    case 5:
    default:
        cells.push_back(SpellVisualCell(-1, -4, '.'));
        cells.push_back(SpellVisualCell(0, -4, '^'));
        cells.push_back(SpellVisualCell(1, -4, '.'));
        cells.push_back(SpellVisualCell(-1, -3, '\\'));
        cells.push_back(SpellVisualCell(0, -3, '|'));
        cells.push_back(SpellVisualCell(1, -3, '/'));
        cells.push_back(SpellVisualCell(0, -2, '!'));
        cells.push_back(SpellVisualCell(0, -1, '.'));
        break;
    }
    return cells;
}

std::vector<Player::SpellVisualCell> Player::buildDownSlamVisualCells() const {
    std::vector<SpellVisualCell> cells;
    const int stage = downSlamStage();
    const int descentLength = std::max(1, downSlamCast.impact.y - downSlamCast.origin.y);

    switch (stage) {
    case 0:
        cells.push_back(SpellVisualCell(0, 1, '.'));
        cells.push_back(SpellVisualCell(-1, 2, '\\'));
        cells.push_back(SpellVisualCell(0, 2, '|'));
        cells.push_back(SpellVisualCell(1, 2, '/'));
        cells.push_back(SpellVisualCell(0, 3, '!'));
        break;
    case 1:
        for (int step = 1; step <= std::min(3, descentLength); ++step) {
            cells.push_back(SpellVisualCell(0, step, '!'));
        }
        break;
    case 2: {
        const int impactDy = downSlamCast.impact.y - downSlamCast.origin.y;
        cells.push_back(SpellVisualCell(0, std::max(1, impactDy - 1), '!'));
        cells.push_back(SpellVisualCell(-1, impactDy - 1, '\\'));
        cells.push_back(SpellVisualCell(0, impactDy - 1, '|'));
        cells.push_back(SpellVisualCell(1, impactDy - 1, '/'));
        cells.push_back(SpellVisualCell(-3, impactDy, '<'));
        cells.push_back(SpellVisualCell(-2, impactDy, '-'));
        cells.push_back(SpellVisualCell(-1, impactDy, '*'));
        cells.push_back(SpellVisualCell(0, impactDy, '*'));
        cells.push_back(SpellVisualCell(1, impactDy, '*'));
        cells.push_back(SpellVisualCell(2, impactDy, '-'));
        cells.push_back(SpellVisualCell(3, impactDy, '>'));
        break;
    }
    case 3:
    default: {
        const int impactDy = downSlamCast.impact.y - downSlamCast.origin.y;
        cells.push_back(SpellVisualCell(0, std::max(1, impactDy - 1), '.'));
        cells.push_back(SpellVisualCell(-1, impactDy - 1, '\\'));
        cells.push_back(SpellVisualCell(0, impactDy - 1, '|'));
        cells.push_back(SpellVisualCell(1, impactDy - 1, '/'));
        cells.push_back(SpellVisualCell(0, impactDy, '.'));
        break;
    }
    }

    return cells;
}

std::vector<Player::SpellVisualCell> Player::buildDashVisualCells() const {
    std::vector<SpellVisualCell> cells;
    if (!dashActive) {
        return cells;
    }

    const int elapsedFrames = std::max(0, kDashFrames - dashFramesRemaining);
    const bool lateStage = elapsedFrames >= (kDashFrames / 2);

    if (dashDirection >= 0) {
        cells.push_back(SpellVisualCell(-1, 0, lateStage ? '-' : '='));
        cells.push_back(SpellVisualCell(-2, 0, '.'));
        if (!lateStage) {
            cells.push_back(SpellVisualCell(1, 0, '>'));
        }
    } else {
        cells.push_back(SpellVisualCell(1, 0, lateStage ? '-' : '='));
        cells.push_back(SpellVisualCell(2, 0, '.'));
        if (!lateStage) {
            cells.push_back(SpellVisualCell(-1, 0, '<'));
        }
    }

    return cells;
}

std::vector<Player::SpellVisualCell> Player::buildMeleeVisualCells() const {
    std::vector<SpellVisualCell> cells;
    const int stage = meleeVisualStage();
    switch (meleeVisual.type) {
    case MeleeVisualType::Horizontal:
        if (meleeVisual.facing == game::FacingDirection::Right) {
            cells.push_back(SpellVisualCell(1, 0, stage == 0 ? '-' : '='));
            cells.push_back(SpellVisualCell(2, 0, '/'));
        } else {
            cells.push_back(SpellVisualCell(-2, 0, '\\'));
            cells.push_back(SpellVisualCell(-1, 0, stage == 0 ? '-' : '='));
        }
        break;
    case MeleeVisualType::Up:
        if (stage == 0) {
            cells.push_back(SpellVisualCell(0, -1, '^'));
        } else {
            cells.push_back(SpellVisualCell(-1, -1, '/'));
            cells.push_back(SpellVisualCell(0, -2, '^'));
            cells.push_back(SpellVisualCell(0, -1, '|'));
            cells.push_back(SpellVisualCell(1, -1, '\\'));
        }
        break;
    case MeleeVisualType::Down:
        if (stage == 0) {
            cells.push_back(SpellVisualCell(0, 1, '!'));
        } else {
            cells.push_back(SpellVisualCell(0, 1, 'v'));
            cells.push_back(SpellVisualCell(0, 2, '!'));
        }
        break;
    }
    return cells;
}

std::vector<game::Position> Player::buildUpWaveDamageCells(const std::string& gameplayMap, bool includeCrown) const {
    std::vector<game::Position> cells;

    for (int dy = -1; dy >= -kUpWaveVerticalReach; --dy) {
        const game::Position centerPosition(upWaveCast.origin.x, upWaveCast.origin.y + dy);
        if (!isInsidePlayableArea(gameplayMap, centerPosition) || tileAt(gameplayMap, centerPosition) == '=') {
            break;
        }
        cells.push_back(centerPosition);

        if (dy <= -3) {
            for (int dx = -kUpWaveUpperSpreadHalfWidth; dx <= kUpWaveUpperSpreadHalfWidth; ++dx) {
                if (dx == 0) {
                    continue;
                }

                const game::Position sidePosition(upWaveCast.origin.x + dx, upWaveCast.origin.y + dy);
                if (!isInsidePlayableArea(gameplayMap, sidePosition) || tileAt(gameplayMap, sidePosition) == '=') {
                    continue;
                }
                cells.push_back(sidePosition);
            }
        }
    }

    if (includeCrown) {
        const int crownY = upWaveCast.origin.y - (kUpWaveVerticalReach - 1);
        for (int dx = -kUpWaveCrownHalfWidth; dx <= kUpWaveCrownHalfWidth; ++dx) {
            const game::Position position(upWaveCast.origin.x + dx, crownY);
            if (!isInsidePlayableArea(gameplayMap, position) || tileAt(gameplayMap, position) == '=') {
                continue;
            }
            cells.push_back(position);
        }
    }

    return cells;
}

std::vector<game::Position> Player::buildDownSlamDamageCells(const std::string& gameplayMap) const {
    std::vector<game::Position> cells;

    for (int y = downSlamCast.origin.y + 1; y <= downSlamCast.impact.y - 1; ++y) {
        const game::Position columnCell(downSlamCast.origin.x, y);
        if (isInsidePlayableArea(gameplayMap, columnCell)) {
            cells.push_back(columnCell);
        }
    }

    const int upperBlastY = downSlamCast.impact.y - 1;
    for (int dx = -kDownSlamUpperBlastHalfWidth; dx <= kDownSlamUpperBlastHalfWidth; ++dx) {
        const game::Position upperBlastCell(downSlamCast.impact.x + dx, upperBlastY);
        if (isInsidePlayableArea(gameplayMap, upperBlastCell) && tileAt(gameplayMap, upperBlastCell) != '=') {
            cells.push_back(upperBlastCell);
        }
    }

    for (int dx = -kDownSlamImpactHalfWidth; dx <= kDownSlamImpactHalfWidth; ++dx) {
        const game::Position impactCell(downSlamCast.impact.x + dx, downSlamCast.impact.y);
        if (isInsidePlayableArea(gameplayMap, impactCell) && tileAt(gameplayMap, impactCell) != '=') {
            cells.push_back(impactCell);
        }
    }

    return cells;
}

std::vector<std::string> Player::buildSoulVesselLines() const {
    const int fillUnits = stats.soul.maximum == 0
            ? 0
            : (stats.soul.current * kSoulFillUnits) / stats.soul.maximum;

    char fillGlyph = 'o';
    if (healCast.active) {
        const char pulseGlyphs[] = {'o', 'O', '*', 'O'};
        const int pulseIndex = (healCast.elapsedFrames / 5) % 4;
        fillGlyph = pulseGlyphs[pulseIndex];
    }

    std::string fill = "   ";
    if (fillUnits >= kSoulFillUnits) {
        fill = std::string(kSoulFillUnits, fillGlyph);
    } else if (fillUnits == 2) {
        fill = std::string(2, fillGlyph) + " ";
    } else if (fillUnits == 1) {
        fill = std::string(1, fillGlyph) + "  ";
    }

    std::vector<std::string> lines;
    lines.push_back("SOUL");
    lines.push_back("    /-\\");
    lines.push_back("   |" + fill + "|");
    lines.push_back("    \\_/");
    lines.push_back(buildSoulReadoutLine(stats));
    return lines;
}

std::string Player::buildHealthOrbLine() const {
    const int displayedSlots = std::max(5, stats.health.maximum);
    const float progress = (!healCast.active || healCast.totalFrames == 0)
            ? 0.0f
            : static_cast<float>(healCast.elapsedFrames) / static_cast<float>(healCast.totalFrames);

    std::ostringstream line;
    line << "HP   ";
    for (int slot = 0; slot < displayedSlots; ++slot) {
        if (slot < stats.health.current) {
            line << "(*)";
        } else if (healCast.active && slot == stats.health.current) {
            if (progress < 0.34f) {
                line << "(.)";
            } else if (progress < 0.67f) {
                line << "(:)";
            } else {
                line << "(*)";
            }
        } else {
            line << "( )";
        }

        if (slot + 1 < displayedSlots) {
            line << " ";
        }
    }
    return line.str();
}

void Player::applyDeathAnimationOverlay(std::string& renderMap) const {
    if (!deathAnimation.active) {
        return;
    }

    const size_t widthWithNewline = lineWidth(renderMap);
    if (widthWithNewline == 0) {
        return;
    }

    const int mapWidth = static_cast<int>(widthWithNewline - 1);
    const int mapHeight = static_cast<int>((renderMap.length() + 1) / widthWithNewline);
    const int fullWhiteStart = kDeathFreezeFrames + kDeathDotFrames + kDeathExpandFrames;
    const int textStart = fullWhiteStart + kDeathFullWhiteFrames;

    if (deathAnimation.elapsedFrames >= fullWhiteStart) {
        for (size_t index = 0; index < renderMap.length(); ++index) {
            if (renderMap[index] != '\n') {
                renderMap[index] = '@';
            }
        }
    } else if (deathAnimation.elapsedFrames >= kDeathFreezeFrames + kDeathDotFrames) {
        const int radius = deathExpansionRadius();
        const int innerRadius = std::max(0, radius - 2);
        const int midRadius = std::max(0, radius - 4);
        const int coreRadius = std::max(0, radius - 6);

        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                const int dx = x - deathAnimation.center.x;
                const int dy = y - deathAnimation.center.y;
                const int distanceSq = dx * dx + dy * dy;

                char glyph = '\0';
                if (distanceSq <= coreRadius * coreRadius) {
                    glyph = '@';
                } else if (distanceSq <= midRadius * midRadius) {
                    glyph = 'O';
                } else if (distanceSq <= innerRadius * innerRadius) {
                    glyph = 'o';
                } else if (distanceSq <= radius * radius) {
                    glyph = '.';
                }

                if (glyph != '\0') {
                    placeGlyph(renderMap, game::Position(x, y), glyph);
                }
            }
        }
    } else if (deathAnimation.elapsedFrames >= kDeathFreezeFrames) {
        placeGlyph(renderMap, deathAnimation.center, 'o');
    }

    if (deathAnimation.elapsedFrames >= textStart) {
        const std::string text = "YOU DEAD";
        const int textX = std::max(0, (mapWidth - static_cast<int>(text.length())) / 2);
        const int textY = std::max(0, mapHeight / 2);
        for (size_t index = 0; index < text.length(); ++index) {
            placeGlyph(renderMap, game::Position(textX + static_cast<int>(index), textY), text[index]);
        }
    }
}
