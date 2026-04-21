#include "player/Player.h"

#include <algorithm>
#include <sstream>

#include "enemy/Enemy.h"

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

const int kBlinkFrames = 125;
const int kHealLockoutFrames = 188;
const int kAttackRange = 2;
const int kHealCastFrames = 42;
const int kUpWaveCastFrames = 18;
const int kDownSlamCastFrames = 16;
const int kMeleeVisualFrames = 6;
const int kSoulMeterMax = 99;
const int kSoulGainOnBasicHit = 11;
const int kSoulSkillCost = 33;
const int kPlayerAttackDamage = 1;

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

} // namespace

Player::Player(KeyStateManager& keyStateManager)
    : ksm(keyStateManager),
      isJumping(false),
      jumpHeldLastFrame(false),
      horizontalMoveAccumulator(0.0f),
      verticalMoveAccumulator(0.0f),
      upwardVelocity(0.0f),
      downwardVelocity(0.0f),
      jumpHoldRemaining(0.0f),
      minimumJumpRiseRemaining(0.0f),
      riseVelocityDropAccumulator(0.0f),
      facing(game::FacingDirection::Right),
      framesSinceLastDamage(kHealLockoutFrames),
      blinkFramesRemaining(0),
      lastAction("Ready"),
      lastResult("Main player combat kit integrated.") {
    resetRuntimeState();
}

std::vector<int> Player::getPalce() {
    return std::vector<int>();
}

void Player::resetRuntimeState() {
    stats = game::CharacterStats();
    stats.soul.maximum = kSoulMeterMax;
    stats.soul.current = 0;
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
    rewardPopup = RewardPopupState();
    projectiles.clear();
    previousKeys.clear();
    lastAction = "Ready";
    lastResult = "Move with A/D, jump with SPACE, attack with J/I/K, cast with L/O/M, heal with R.";
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

void Player::updateCombat(const std::string& gameplayMap, game::GroundEnemy& groundEnemy) {
    updateBlink();
    updateRewardPopup();
    updateFacingFromInput();

    if (!isAlive()) {
        previousKeys = ksm.keyStates;
        return;
    }

    const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
    if (playerPosition.x < 0 || playerPosition.y < 0) {
        previousKeys = ksm.keyStates;
        return;
    }

    if (!isMovementLocked() && (isJustPressed('j') || isJustPressed('J'))) {
        startMeleeVisual(MeleeVisualType::Horizontal, playerPosition);
        lastAction = "Basic Attack";
        if (applyDamageToEnemyInMeleeRange(
                groundEnemy,
                playerPosition,
                game::DamageInfo(kPlayerAttackDamage, game::DamageType::BasicAttack, "player", true))) {
            stats.soul.current = std::min(stats.soul.maximum, stats.soul.current + kSoulGainOnBasicHit);
            lastResult = "Slash connected. Soul +11.";
        } else {
            lastResult = "No target in range.";
        }
    }

    if (!isMovementLocked() && (isJustPressed('i') || isJustPressed('I'))) {
        startMeleeVisual(MeleeVisualType::Up, playerPosition);
        lastAction = "Up Slash";
        if (applyDamageToEnemyInMeleeRange(
                groundEnemy,
                playerPosition,
                game::DamageInfo(kPlayerAttackDamage, game::DamageType::UpSlash, "player", true))) {
            lastResult = "Up slash connected.";
        } else {
            lastResult = "Slash triggered. No target in range.";
        }
    }

    if (!isMovementLocked() && (isJustPressed('k') || isJustPressed('K'))) {
        startMeleeVisual(MeleeVisualType::Down, playerPosition);
        lastAction = "Down Slash";
        if (applyDamageToEnemyInMeleeRange(
                groundEnemy,
                playerPosition,
                game::DamageInfo(kPlayerAttackDamage, game::DamageType::DownSlash, "player", true))) {
            lastResult = "Down slash connected.";
        } else {
            lastResult = "Slash triggered. No target in range.";
        }
    }

    if (!isMovementLocked() && (isJustPressed('l') || isJustPressed('L'))) {
        castHorizontalWave(playerPosition);
    }

    if (!isMovementLocked() && (isJustPressed('o') || isJustPressed('O'))) {
        castUpWave(playerPosition);
    }

    if (!isMovementLocked() && (isJustPressed('m') || isJustPressed('M'))) {
        castDownSlam(gameplayMap, playerPosition);
    }

    if (!isMovementLocked() && (isJustPressed('r') || isJustPressed('R'))) {
        startHealCast();
    }

    updateDownSlamCast(gameplayMap, groundEnemy);
    updateUpWaveCast(gameplayMap, groundEnemy);
    updateMeleeVisual();
    updateProjectiles(gameplayMap, groundEnemy);
    updateHealCast();

    stats.soul.maximum = kSoulMeterMax;
    if (stats.soul.current > stats.soul.maximum) {
        stats.soul.current = stats.soul.maximum;
    }

    previousKeys = ksm.keyStates;
}

void Player::receiveDamage(const std::string& sourceLabel) {
    if (blinkFramesRemaining > 0 || stats.health.current <= 0) {
        return;
    }

    stats.health.current = std::max(0, stats.health.current - 1);
    framesSinceLastDamage = 0;
    blinkFramesRemaining = kBlinkFrames;
    hitFeedback.blinking = true;
    healCast.active = false;
    lastAction = "Damaged";

    if (stats.health.current == 0) {
        lastResult = sourceLabel + " defeated the player. Press R to reset.";
    } else {
        lastResult = sourceLabel + " dealt 1 damage.";
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

const game::CharacterStats& Player::getStats() const {
    return stats;
}

game::FacingDirection Player::getFacingDirection() const {
    return facing;
}

std::string Player::buildHud() const {
    std::ostringstream hud;
    const std::vector<std::string> soulLines = buildSoulVesselLines();
    for (size_t index = 0; index < soulLines.size(); ++index) {
        hud << soulLines[index] << "\n";
    }
    hud << buildHealthOrbLine() << "\n";
    hud << "[Main] ESC exit | P reset room\n";
    hud << "Move A/D  Jump SPACE  Basic J  Up I  Down K  Wave L  UpWave O  Slam M  Heal R\n";
    hud << "Facing " << (facing == game::FacingDirection::Right ? "Right" : "Left")
        << " | Heal Ready " << (framesSinceLastDamage >= kHealLockoutFrames ? "YES" : "NO")
        << " | HKD " << stats.hkd << "\n";
    hud << "Last Action: " << lastAction << "\n";
    hud << "Last Result: " << lastResult << "\n\n";
    return hud.str();
}

void Player::overlayRender(std::string& renderMap, const std::string& gameplayMap) const {
    const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
    if (playerPosition.x < 0 || playerPosition.y < 0) {
        return;
    }

    if (healCast.active) {
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

    if (meleeVisual.active) {
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

    if (upWaveCast.active) {
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

    if (downSlamCast.active) {
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

    if (isVisible()) {
        placeGlyph(renderMap, playerPosition, '@');
    } else {
        placeGlyph(renderMap, playerPosition, ' ');
    }

    if (rewardPopup.active) {
        const int riseOffset = rewardPopup.elapsedFrames / 8;
        const std::string amountText = "+" + std::to_string(rewardPopup.amount);
        const game::Position popupPosition(
                playerPosition.x - static_cast<int>(amountText.length() / 2),
                playerPosition.y - 2 - riseOffset);
        placeText(renderMap, popupPosition, amountText);
    }
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

void Player::updateUpWaveCast(const std::string& gameplayMap, game::GroundEnemy& groundEnemy) {
    if (!upWaveCast.active) {
        return;
    }

    upWaveCast.elapsedFrames++;
    const int stage = upWaveStage();

    if (!upWaveCast.damageResolved && stage >= 3) {
        const std::vector<game::Position> damageCells = buildUpWaveDamageCells(gameplayMap, stage >= 4);
        for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
            if (applyDamageToEnemyAtPosition(
                    groundEnemy,
                    damageCells[cellIndex],
                    game::DamageInfo(kPlayerAttackDamage, game::DamageType::SoulWaveUp, "player", true))) {
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

void Player::updateDownSlamCast(const std::string& gameplayMap, game::GroundEnemy& groundEnemy) {
    if (!downSlamCast.active) {
        return;
    }

    downSlamCast.elapsedFrames++;
    const int stage = downSlamStage();

    if (!downSlamCast.damageResolved && stage >= 2) {
        const std::vector<game::Position> damageCells = buildDownSlamDamageCells();
        for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
            applyDamageToEnemyAtPosition(
                    groundEnemy,
                    damageCells[cellIndex],
                    game::DamageInfo(kPlayerAttackDamage, game::DamageType::SoulSlam, "player", true));
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

void Player::updateProjectiles(const std::string& gameplayMap, game::GroundEnemy& groundEnemy) {
    std::vector<VisualProjectile> activeProjectiles;

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
        if (applyDamageToEnemyAtPosition(
                groundEnemy,
                projectile.position,
                game::DamageInfo(kPlayerAttackDamage, damageType, "player", true))) {
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

bool Player::isEnemyInMeleeRange(const game::Position& playerPosition,
                                 const game::Position& enemyPosition) const {
    const int deltaX = enemyPosition.x - playerPosition.x;
    const int deltaY = std::abs(enemyPosition.y - playerPosition.y);

    if (deltaY > 1) {
        return false;
    }

    if (facing == game::FacingDirection::Right) {
        return deltaX >= 1 && deltaX <= kAttackRange;
    }

    return deltaX <= -1 && deltaX >= -kAttackRange;
}

bool Player::applyDamageToEnemyAtPosition(game::GroundEnemy& groundEnemy,
                                          const game::Position& targetPosition,
                                          const game::DamageInfo& damageInfo) {
    if (!groundEnemy.isAlive()) {
        return false;
    }

    const game::Position enemyPosition = groundEnemy.getPosition();
    if (enemyPosition.x == targetPosition.x && enemyPosition.y == targetPosition.y) {
        const bool wasAlive = groundEnemy.isAlive();
        groundEnemy.takeDamage(damageInfo);
        if (wasAlive && !groundEnemy.isAlive()) {
            grantKillReward(groundEnemy.getHkdReward());
        }
        return true;
    }

    return false;
}

bool Player::applyDamageToEnemyInMeleeRange(game::GroundEnemy& groundEnemy,
                                            const game::Position& playerPosition,
                                            const game::DamageInfo& damageInfo) {
    if (!groundEnemy.isAlive()) {
        return false;
    }

    if (!isEnemyInMeleeRange(playerPosition, groundEnemy.getPosition())) {
        return false;
    }

    const bool wasAlive = groundEnemy.isAlive();
    groundEnemy.takeDamage(damageInfo);
    if (wasAlive && !groundEnemy.isAlive()) {
        grantKillReward(groundEnemy.getHkdReward());
    }
    return true;
}

void Player::grantKillReward(int amount) {
    if (amount <= 0) {
        return;
    }

    stats.hkd += amount;
    rewardPopup.active = true;
    rewardPopup.elapsedFrames = 0;
    rewardPopup.totalFrames = 24;
    rewardPopup.amount = amount;
    lastResult = "Enemy defeated. HKD +" + std::to_string(amount) + ".";
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

    for (int dy = -1; dy >= -5; --dy) {
        const game::Position position(upWaveCast.origin.x, upWaveCast.origin.y + dy);
        if (!isInsidePlayableArea(gameplayMap, position) || tileAt(gameplayMap, position) == '=') {
            break;
        }
        cells.push_back(position);
    }

    if (includeCrown) {
        for (int dx = -1; dx <= 1; ++dx) {
            const game::Position position(upWaveCast.origin.x + dx, upWaveCast.origin.y - 4);
            if (!isInsidePlayableArea(gameplayMap, position) || tileAt(gameplayMap, position) == '=') {
                continue;
            }
            cells.push_back(position);
        }
    }

    return cells;
}

std::vector<game::Position> Player::buildDownSlamDamageCells() const {
    std::vector<game::Position> cells;

    for (int y = downSlamCast.origin.y + 1; y <= downSlamCast.impact.y - 1; ++y) {
        cells.push_back(game::Position(downSlamCast.origin.x, y));
    }

    for (int dx = -1; dx <= 1; ++dx) {
        cells.push_back(game::Position(downSlamCast.impact.x + dx, downSlamCast.impact.y));
    }

    return cells;
}

std::vector<std::string> Player::buildSoulVesselLines() const {
    const int totalFillUnits = 3;
    const int fillUnits = stats.soul.maximum == 0
            ? 0
            : (stats.soul.current * totalFillUnits) / stats.soul.maximum;

    char fillGlyph = 'o';
    if (healCast.active) {
        const char pulseGlyphs[] = {'o', 'O', '*', 'O'};
        const int pulseIndex = (healCast.elapsedFrames / 5) % 4;
        fillGlyph = pulseGlyphs[pulseIndex];
    }

    std::string fill = "   ";
    if (fillUnits >= 3) {
        fill = std::string(3, fillGlyph);
    } else if (fillUnits == 2) {
        fill = std::string(2, fillGlyph) + " ";
    } else if (fillUnits == 1) {
        fill = std::string(" ") + fillGlyph + " ";
    }

    std::vector<std::string> lines;
    lines.push_back("SOUL");
    lines.push_back("    /-\\");
    lines.push_back("   |" + fill + "|");
    lines.push_back("    \\_/");
    lines.push_back("   " + std::to_string(stats.soul.current) + "/" + std::to_string(stats.soul.maximum));
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
