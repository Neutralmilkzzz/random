#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "enemy/Enemy.h"
#include "input/KeyStateManager.h"
#include "player/Player.h"
#include "shared/GameTypes.h"
#include "world/MapDrawer.h"

namespace {

const int kFrameMs = 16;
const int kBlinkFrames = 125;
const int kHealLockoutFrames = 188;
const int kAttackRange = 2;
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
const int kPlayerAttackDamage = 1;

const std::string kArenaMap =
        "============================================================================\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                    T                                                     =\n"
        "=                                                                          =\n"
        "=   @                                                                      =\n"
        "============================================================================";

const std::vector<game::Position> kGroundEnemySpawnPoints = {
        game::Position(18, 9),
        game::Position(26, 9),
        game::Position(34, 9),
        game::Position(42, 9)
};

struct VisualProjectile {
    game::Position position;
    int dx;
    int dy;
    int remainingFrames;
    int totalFrames;
    std::string label;
};

struct VisualEffect {
    std::vector<game::Position> cells;
    int remainingFrames;
    char glyph;
};

struct SpellVisualCell {
    int dx;
    int dy;
    char glyph;
};

enum class MeleeVisualType {
    Horizontal,
    Up,
    Down
};

struct HealCastState {
    bool active;
    int elapsedFrames;
    int totalFrames;

    HealCastState()
        : active(false),
          elapsedFrames(0),
          totalFrames(kHealCastFrames) {
    }
};

struct UpWaveCastState {
    bool active;
    bool damageResolved;
    int elapsedFrames;
    int totalFrames;
    game::Position origin;

    UpWaveCastState()
        : active(false),
          damageResolved(false),
          elapsedFrames(0),
          totalFrames(kUpWaveCastFrames),
          origin(-1, -1) {
    }
};

struct DownSlamCastState {
    bool active;
    bool damageResolved;
    int elapsedFrames;
    int totalFrames;
    game::Position origin;
    game::Position impact;

    DownSlamCastState()
        : active(false),
          damageResolved(false),
          elapsedFrames(0),
          totalFrames(kDownSlamCastFrames),
          origin(-1, -1),
          impact(-1, -1) {
    }
};

struct MeleeVisualState {
    bool active;
    int elapsedFrames;
    int totalFrames;
    MeleeVisualType type;
    game::Position origin;
    game::FacingDirection facing;

    MeleeVisualState()
        : active(false),
          elapsedFrames(0),
          totalFrames(kMeleeVisualFrames),
          type(MeleeVisualType::Horizontal),
          origin(-1, -1),
          facing(game::FacingDirection::Right) {
    }
};

struct DeathAnimationState {
    bool active;
    int elapsedFrames;
    game::Position center;

    DeathAnimationState()
        : active(false),
          elapsedFrames(0),
          center(-1, -1) {
    }
};

struct SandboxEnemy {
    int id;
    game::GroundEnemy groundEnemy;

    SandboxEnemy(int enemyId, const game::Position& spawnPosition)
        : id(enemyId),
          groundEnemy("player_sandbox_ground_enemy_" + std::to_string(enemyId), spawnPosition) {
    }
};

struct SandboxState {
    game::CharacterStats stats;
    game::HitFeedbackState hitFeedback;
    game::FacingDirection facing;
    bool infiniteSoulMode;
    int dummyHitCount;
    int dummySoulChargeCount;
    int selfDamageCount;
    int successfulHealCount;
    int failedHealCount;
    int groundEnemySpawns;
    int defeatedGroundEnemies;
    int nextEnemyId;
    int framesSinceLastDamage;
    int blinkFramesRemaining;
    HealCastState healCast;
    UpWaveCastState upWaveCast;
    DownSlamCastState downSlamCast;
    MeleeVisualState meleeVisual;
    DeathAnimationState deathAnimation;
    std::string lastAction;
    std::string lastResult;
    std::vector<VisualProjectile> projectiles;
    std::vector<VisualEffect> effects;
    std::unordered_map<int, bool> previousKeys;

    SandboxState()
        : facing(game::FacingDirection::Right),
          infiniteSoulMode(true),
          dummyHitCount(0),
          dummySoulChargeCount(0),
          selfDamageCount(0),
          successfulHealCount(0),
          failedHealCount(0),
          groundEnemySpawns(0),
          defeatedGroundEnemies(0),
          nextEnemyId(1),
          framesSinceLastDamage(kHealLockoutFrames),
          blinkFramesRemaining(0),
          lastAction("Ready"),
          lastResult("A/D move, SPACE jump, 1/2 spawn ground enemies, ESC exit.") {
        stats.soul.maximum = kSoulMeterMax;
        stats.soul.current = stats.soul.maximum;
    }
};

bool isKeyDown(const KeyStateManager& keyStateManager, int keyCode) {
    const std::unordered_map<int, bool>::const_iterator it = keyStateManager.keyStates.find(keyCode);
    return it != keyStateManager.keyStates.end() && it->second;
}

bool wasKeyDown(const SandboxState& state, int keyCode) {
    const std::unordered_map<int, bool>::const_iterator it = state.previousKeys.find(keyCode);
    return it != state.previousKeys.end() && it->second;
}

bool isJustPressed(const KeyStateManager& keyStateManager, const SandboxState& state, int keyCode) {
    return isKeyDown(keyStateManager, keyCode) && !wasKeyDown(state, keyCode);
}

float healProgress(const SandboxState& state) {
    if (!state.healCast.active || state.healCast.totalFrames == 0) {
        return 0.0f;
    }

    return static_cast<float>(state.healCast.elapsedFrames) / static_cast<float>(state.healCast.totalFrames);
}

int upWaveStage(const SandboxState& state) {
    if (!state.upWaveCast.active || state.upWaveCast.totalFrames <= 0) {
        return -1;
    }

    int stage = (state.upWaveCast.elapsedFrames * 6) / state.upWaveCast.totalFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 5) {
        stage = 5;
    }
    return stage;
}

int downSlamStage(const SandboxState& state) {
    if (!state.downSlamCast.active || state.downSlamCast.totalFrames <= 0) {
        return -1;
    }

    int stage = (state.downSlamCast.elapsedFrames * 4) / state.downSlamCast.totalFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 3) {
        stage = 3;
    }
    return stage;
}

int meleeVisualStage(const SandboxState& state) {
    if (!state.meleeVisual.active || state.meleeVisual.totalFrames <= 0) {
        return -1;
    }

    int stage = (state.meleeVisual.elapsedFrames * 2) / state.meleeVisual.totalFrames;
    if (stage < 0) {
        stage = 0;
    }
    if (stage > 1) {
        stage = 1;
    }
    return stage;
}

int deathAnimationTotalFrames() {
    return kDeathFreezeFrames +
           kDeathDotFrames +
           kDeathExpandFrames +
           kDeathFullWhiteFrames +
           kDeathTextFrames;
}

bool isDeathTextPhase(const SandboxState& state) {
    return state.deathAnimation.elapsedFrames >=
           (kDeathFreezeFrames + kDeathDotFrames + kDeathExpandFrames + kDeathFullWhiteFrames);
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

std::vector<SpellVisualCell> buildUpWaveVisualCells(int stage) {
    std::vector<SpellVisualCell> cells;

    switch (stage) {
    case 0:
        cells.push_back({-1, -2, '.'});
        cells.push_back({0, -2, '^'});
        cells.push_back({1, -2, '.'});
        cells.push_back({-1, -1, '/'});
        cells.push_back({1, -1, '\\'});
        break;
    case 1:
        cells.push_back({0, -3, '^'});
        cells.push_back({-1, -2, '.'});
        cells.push_back({0, -2, '*'});
        cells.push_back({1, -2, '.'});
        cells.push_back({0, -1, '!'});
        break;
    case 2:
        cells.push_back({0, -4, '^'});
        cells.push_back({-1, -3, '.'});
        cells.push_back({0, -3, '!'});
        cells.push_back({1, -3, '.'});
        cells.push_back({0, -2, '!'});
        cells.push_back({0, -1, '!'});
        break;
    case 3:
        cells.push_back({-1, -4, '\\'});
        cells.push_back({0, -4, '|'});
        cells.push_back({1, -4, '/'});
        cells.push_back({-2, -3, '<'});
        cells.push_back({-1, -3, '-'});
        cells.push_back({0, -3, '!'});
        cells.push_back({1, -3, '-'});
        cells.push_back({2, -3, '>'});
        cells.push_back({-1, -2, '\\'});
        cells.push_back({0, -2, '|'});
        cells.push_back({1, -2, '/'});
        cells.push_back({0, -1, '!'});
        break;
    case 4:
        cells.push_back({-2, -5, '\\'});
        cells.push_back({-1, -5, '^'});
        cells.push_back({0, -5, '^'});
        cells.push_back({1, -5, '^'});
        cells.push_back({2, -5, '/'});
        cells.push_back({-3, -4, '<'});
        cells.push_back({-2, -4, '-'});
        cells.push_back({-1, -4, '*'});
        cells.push_back({0, -4, '*'});
        cells.push_back({1, -4, '*'});
        cells.push_back({2, -4, '-'});
        cells.push_back({3, -4, '>'});
        cells.push_back({-1, -3, '\\'});
        cells.push_back({0, -3, '|'});
        cells.push_back({1, -3, '/'});
        cells.push_back({-1, -2, '\\'});
        cells.push_back({0, -2, '|'});
        cells.push_back({1, -2, '/'});
        cells.push_back({0, -1, '!'});
        break;
    case 5:
    default:
        cells.push_back({-1, -4, '.'});
        cells.push_back({0, -4, '^'});
        cells.push_back({1, -4, '.'});
        cells.push_back({-1, -3, '\\'});
        cells.push_back({0, -3, '|'});
        cells.push_back({1, -3, '/'});
        cells.push_back({0, -2, '!'});
        cells.push_back({0, -1, '.'});
        break;
    }

    return cells;
}

std::vector<SpellVisualCell> buildHorizontalWaveVisualCells(const VisualProjectile& projectile) {
    std::vector<SpellVisualCell> cells;
    if (projectile.dx == 0) {
        return cells;
    }

    const int age = projectile.totalFrames - projectile.remainingFrames;
    const bool facingRight = projectile.dx > 0;

    if (age <= 2) {
        if (facingRight) {
            cells.push_back({-2, 0, '-'});
            cells.push_back({-1, 0, '='});
            cells.push_back({0, 0, '>'});
        } else {
            cells.push_back({0, 0, '<'});
            cells.push_back({1, 0, '='});
            cells.push_back({2, 0, '-'});
        }
        return cells;
    }

    if (projectile.remainingFrames <= 4) {
        if (facingRight) {
            cells.push_back({-2, 0, '~'});
            cells.push_back({-1, 0, '~'});
            cells.push_back({0, 0, '>'});
        } else {
            cells.push_back({0, 0, '<'});
            cells.push_back({1, 0, '~'});
            cells.push_back({2, 0, '~'});
        }
        return cells;
    }

    if (facingRight) {
        cells.push_back({-3, 0, '~'});
        cells.push_back({-2, 0, '='});
        cells.push_back({-1, 0, '='});
        cells.push_back({0, 0, '>'});
        cells.push_back({-2, -1, '.'});
        cells.push_back({-2, 1, '\''});
    } else {
        cells.push_back({0, 0, '<'});
        cells.push_back({1, 0, '='});
        cells.push_back({2, 0, '='});
        cells.push_back({3, 0, '~'});
        cells.push_back({2, -1, '.'});
        cells.push_back({2, 1, '\''});
    }

    return cells;
}

std::vector<game::Position> buildUpWaveDamageCells(const std::string& map,
                                                   const game::Position& origin,
                                                   bool includeCrown) {
    std::vector<game::Position> cells;

    for (int dy = -1; dy >= -5; --dy) {
        const game::Position position(origin.x, origin.y + dy);
        if (!isInsidePlayableArea(map, position) || tileAt(map, position) == '=') {
            break;
        }
        cells.push_back(position);
    }

    if (includeCrown) {
        for (int dx = -1; dx <= 1; ++dx) {
            const game::Position position(origin.x + dx, origin.y - 4);
            if (!isInsidePlayableArea(map, position) || tileAt(map, position) == '=') {
                continue;
            }
            cells.push_back(position);
        }
    }

    return cells;
}

game::Position findDownSlamImpactPosition(const std::string& map, const game::Position& origin) {
    game::Position impact = origin;

    for (int y = origin.y + 1; y < 200; ++y) {
        const game::Position candidate(origin.x, y);
        if (!isInsidePlayableArea(map, candidate)) {
            break;
        }
        if (tileAt(map, candidate) == '=') {
            break;
        }
        impact = candidate;
    }

    return impact;
}

std::vector<SpellVisualCell> buildDownSlamVisualCells(const SandboxState& state) {
    std::vector<SpellVisualCell> cells;
    const int stage = downSlamStage(state);
    const int descentLength = std::max(1, state.downSlamCast.impact.y - state.downSlamCast.origin.y);

    switch (stage) {
    case 0:
        cells.push_back({0, 1, '.'});
        cells.push_back({-1, 2, '\\'});
        cells.push_back({0, 2, '|'});
        cells.push_back({1, 2, '/'});
        cells.push_back({0, 3, '!'});
        break;
    case 1:
        for (int step = 1; step <= std::min(3, descentLength); ++step) {
            cells.push_back({0, step, '!'});
        }
        break;
    case 2: {
        const int impactDy = state.downSlamCast.impact.y - state.downSlamCast.origin.y;
        cells.push_back({0, std::max(1, impactDy - 1), '!'});
        cells.push_back({-1, impactDy - 1, '\\'});
        cells.push_back({0, impactDy - 1, '|'});
        cells.push_back({1, impactDy - 1, '/'});
        cells.push_back({-3, impactDy, '<'});
        cells.push_back({-2, impactDy, '-'});
        cells.push_back({-1, impactDy, '*'});
        cells.push_back({0, impactDy, '*'});
        cells.push_back({1, impactDy, '*'});
        cells.push_back({2, impactDy, '-'});
        cells.push_back({3, impactDy, '>'});
        break;
    }
    case 3:
    default: {
        const int impactDy = state.downSlamCast.impact.y - state.downSlamCast.origin.y;
        cells.push_back({0, std::max(1, impactDy - 1), '.'});
        cells.push_back({-1, impactDy - 1, '\\'});
        cells.push_back({0, impactDy - 1, '|'});
        cells.push_back({1, impactDy - 1, '/'});
        cells.push_back({0, impactDy, '.'});
        break;
    }
    }

    return cells;
}

std::vector<SpellVisualCell> buildMeleeVisualCells(const SandboxState& state) {
    std::vector<SpellVisualCell> cells;
    if (!state.meleeVisual.active) {
        return cells;
    }

    const int stage = meleeVisualStage(state);
    switch (state.meleeVisual.type) {
    case MeleeVisualType::Horizontal:
        if (state.meleeVisual.facing == game::FacingDirection::Right) {
            cells.push_back({1, 0, stage == 0 ? '-' : '='});
            cells.push_back({2, 0, '/'});
        } else {
            cells.push_back({-2, 0, '\\'});
            cells.push_back({-1, 0, stage == 0 ? '-' : '='});
        }
        break;
    case MeleeVisualType::Up:
        if (stage == 0) {
            cells.push_back({0, -1, '^'});
        } else {
            cells.push_back({0, -2, '^'});
            cells.push_back({-1, -1, '/'});
            cells.push_back({0, -1, '|'});
            cells.push_back({1, -1, '\\'});
        }
        break;
    case MeleeVisualType::Down:
        if (stage == 0) {
            cells.push_back({0, 1, '!'});
        } else {
            cells.push_back({0, 1, 'v'});
            cells.push_back({0, 2, '!'});
        }
        break;
    }

    return cells;
}

void startMeleeVisual(SandboxState& state,
                      MeleeVisualType type,
                      const game::Position& origin,
                      game::FacingDirection facing) {
    state.meleeVisual.active = true;
    state.meleeVisual.elapsedFrames = 0;
    state.meleeVisual.totalFrames = kMeleeVisualFrames;
    state.meleeVisual.type = type;
    state.meleeVisual.origin = origin;
    state.meleeVisual.facing = facing;
}

std::vector<game::Position> buildDownSlamDamageCells(const SandboxState& state) {
    std::vector<game::Position> cells;

    for (int y = state.downSlamCast.origin.y + 1; y <= state.downSlamCast.impact.y - 1; ++y) {
        cells.push_back(game::Position(state.downSlamCast.origin.x, y));
    }

    for (int dx = -1; dx <= 1; ++dx) {
        cells.push_back(game::Position(state.downSlamCast.impact.x + dx, state.downSlamCast.impact.y));
    }

    return cells;
}

bool isEnemyAt(const std::vector<SandboxEnemy>& enemies, const game::Position& position, int ignoreId) {
    for (size_t index = 0; index < enemies.size(); ++index) {
        const game::Position enemyPosition = enemies[index].groundEnemy.getPosition();
        if (enemies[index].id != ignoreId &&
            enemies[index].groundEnemy.isAlive() &&
            enemyPosition.x == position.x &&
            enemyPosition.y == position.y) {
            return true;
        }
    }

    return false;
}

bool canEnemyOccupy(const std::string& gameplayMap,
                    const std::vector<SandboxEnemy>& enemies,
                    const game::Position& playerPosition,
                    const game::Position& targetPosition,
                    int ignoreId) {
    if (!isInsidePlayableArea(gameplayMap, targetPosition)) {
        return false;
    }

    if (tileAt(gameplayMap, targetPosition) != ' ') {
        return false;
    }

    if (targetPosition.x == playerPosition.x && targetPosition.y == playerPosition.y) {
        return false;
    }

    return !isEnemyAt(enemies, targetPosition, ignoreId);
}

std::string buildCollisionMap(const std::string& gameplayMap, const std::vector<SandboxEnemy>& enemies) {
    std::string collisionMap = gameplayMap;
    for (size_t index = 0; index < enemies.size(); ++index) {
        if (enemies[index].groundEnemy.isAlive()) {
            placeGlyph(collisionMap, enemies[index].groundEnemy.getPosition(), 'g');
        }
    }
    return collisionMap;
}

bool canSpendSoul(SandboxState& state, int soulCost) {
    if (state.infiniteSoulMode) {
        return true;
    }

    if (state.stats.soul.current < soulCost) {
        return false;
    }

    state.stats.soul.current -= soulCost;
    return true;
}

bool isDummyInMeleeRange(const game::Position& playerPosition,
                         const game::Position& dummyPosition,
                         game::FacingDirection facing) {
    if (dummyPosition.x < 0 || dummyPosition.y < 0) {
        return false;
    }

    const int deltaX = dummyPosition.x - playerPosition.x;
    const int deltaY = std::abs(dummyPosition.y - playerPosition.y);

    if (deltaY > 1) {
        return false;
    }

    if (facing == game::FacingDirection::Right) {
        return deltaX >= 1 && deltaX <= kAttackRange;
    }

    return deltaX <= -1 && deltaX >= -kAttackRange;
}

bool isEnemyInMeleeRange(const game::Position& playerPosition,
                         const game::Position& enemyPosition,
                         game::FacingDirection facing) {
    return isDummyInMeleeRange(playerPosition, enemyPosition, facing);
}

void registerDummyHit(SandboxState& state,
                      const std::string& attackName,
                      bool grantsSoul,
                      int soulGainAmount) {
    state.dummyHitCount++;
    state.lastAction = attackName;

    if (grantsSoul) {
        state.dummySoulChargeCount++;
        if (!state.infiniteSoulMode) {
            state.stats.soul.current = std::min(state.stats.soul.maximum, state.stats.soul.current + soulGainAmount);
        }
        state.lastResult = "Wooden dummy hit. Soul gain simulated.";
    } else {
        state.lastResult = "Wooden dummy hit.";
    }
}

void registerGroundEnemyHit(SandboxState& state,
                            const std::string& attackName,
                            int hitCount,
                            bool grantsSoul,
                            int soulGainAmount,
                            int defeatedCount) {
    state.lastAction = attackName;

    if (grantsSoul && !state.infiniteSoulMode) {
        state.stats.soul.current = std::min(state.stats.soul.maximum, state.stats.soul.current + soulGainAmount * hitCount);
    }

    std::ostringstream result;
    result << "Hit " << hitCount << (hitCount == 1 ? " ground enemy." : " ground enemies.");

    if (grantsSoul) {
        result << " Soul gain simulated.";
    }

    if (defeatedCount > 0) {
        result << " Defeated " << defeatedCount << ".";
    }

    state.lastResult = result.str();
}

int applyDamageToGroundEnemies(std::vector<SandboxEnemy>& enemies,
                               const game::Position& playerPosition,
                               game::FacingDirection facing,
                               const game::DamageInfo& damageInfo) {
    int hitCount = 0;

    for (size_t index = 0; index < enemies.size(); ++index) {
        if (!enemies[index].groundEnemy.isAlive()) {
            continue;
        }

        if (isEnemyInMeleeRange(playerPosition, enemies[index].groundEnemy.getPosition(), facing)) {
            enemies[index].groundEnemy.takeDamage(damageInfo);
            hitCount++;
        }
    }

    return hitCount;
}

bool applyDamageToEnemyAtPosition(std::vector<SandboxEnemy>& enemies,
                                  const game::Position& targetPosition,
                                  const game::DamageInfo& damageInfo) {
    for (size_t index = 0; index < enemies.size(); ++index) {
        if (!enemies[index].groundEnemy.isAlive()) {
            continue;
        }

        const game::Position enemyPosition = enemies[index].groundEnemy.getPosition();
        if (enemyPosition.x == targetPosition.x && enemyPosition.y == targetPosition.y) {
            enemies[index].groundEnemy.takeDamage(damageInfo);
            return true;
        }
    }

    return false;
}

void removeDefeatedGroundEnemies(std::vector<SandboxEnemy>& enemies, SandboxState& state) {
    std::vector<SandboxEnemy> survivors;
    int defeatedNow = 0;

    for (size_t index = 0; index < enemies.size(); ++index) {
        if (!enemies[index].groundEnemy.shouldDespawn()) {
            survivors.push_back(enemies[index]);
        } else {
            defeatedNow++;
        }
    }

    if (defeatedNow > 0) {
        state.defeatedGroundEnemies += defeatedNow;
    }

    enemies.swap(survivors);
}

bool spawnGroundEnemy(std::vector<SandboxEnemy>& enemies,
                      SandboxState& state,
                      const std::string& gameplayMap,
                      const game::Position& playerPosition,
                      const game::Position& spawnPosition) {
    if (!canEnemyOccupy(gameplayMap, enemies, playerPosition, spawnPosition, -1)) {
        return false;
    }

    enemies.push_back(SandboxEnemy(state.nextEnemyId++, spawnPosition));
    state.groundEnemySpawns++;
    return true;
}

void updateGroundEnemies(std::vector<SandboxEnemy>& enemies,
                         const std::string& gameplayMap,
                         const game::Position& playerPosition) {
    for (size_t index = 0; index < enemies.size(); ++index) {
        const game::Position previousPosition = enemies[index].groundEnemy.getPosition();
        enemies[index].groundEnemy.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
        const game::Position nextPosition = enemies[index].groundEnemy.getPosition();

        if (enemies[index].groundEnemy.isAlive() &&
            (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
            !canEnemyOccupy(gameplayMap, enemies, playerPosition, nextPosition, enemies[index].id)) {
            enemies[index].groundEnemy.setPosition(previousPosition);
        }
    }
}

void applySelfDamage(SandboxState& state) {
    state.lastAction = "Self Damage";
    if (state.stats.health.current > 1) {
        state.stats.health.current--;
        state.selfDamageCount++;
        state.framesSinceLastDamage = 0;
        state.blinkFramesRemaining = kBlinkFrames;
        state.hitFeedback.blinking = true;
        state.lastResult = "Lost 1 mask. Heal is locked for 3 seconds.";
        return;
    }

    state.lastResult = "Health is already at the safe minimum for healing tests.";
}

void castHorizontalWave(SandboxState& state, const game::Position& playerPosition) {
    state.lastAction = "Horizontal Soul Wave";
    if (!canSpendSoul(state, 33)) {
        state.lastResult = "Not enough soul.";
        return;
    }

    VisualProjectile projectile;
    projectile.position = game::Position(playerPosition.x + (state.facing == game::FacingDirection::Right ? 1 : -1), playerPosition.y);
    projectile.dx = state.facing == game::FacingDirection::Right ? 1 : -1;
    projectile.dy = 0;
    projectile.remainingFrames = 28;
    projectile.totalFrames = projectile.remainingFrames;
    projectile.label = "Horizontal wave";
    state.projectiles.push_back(projectile);
    state.lastResult = state.infiniteSoulMode ? "Cast in infinite soul mode." : "Consumed 33 soul.";
}

void castUpWave(SandboxState& state, const game::Position& playerPosition) {
    state.lastAction = "Up Soul Wave";
    if (state.upWaveCast.active) {
        state.lastResult = "Up wave is already active.";
        return;
    }

    if (!canSpendSoul(state, 33)) {
        state.lastResult = "Not enough soul.";
        return;
    }

    state.upWaveCast.active = true;
    state.upWaveCast.damageResolved = false;
    state.upWaveCast.elapsedFrames = 0;
    state.upWaveCast.totalFrames = kUpWaveCastFrames;
    state.upWaveCast.origin = playerPosition;
    state.lastResult = state.infiniteSoulMode ? "Soul gathers above the head." : "Soul gathers upward. Consumed 33 soul.";
}

void castDownSlam(SandboxState& state,
                  const std::string& map,
                  const game::Position& playerPosition,
                  const game::Position& dummyPosition,
                  std::vector<SandboxEnemy>& enemies) {
    (void)dummyPosition;
    (void)enemies;
    state.lastAction = "Down Slam";
    if (state.downSlamCast.active) {
        state.lastResult = "Down slam is already active.";
        return;
    }

    if (!canSpendSoul(state, 33)) {
        state.lastResult = "Not enough soul.";
        return;
    }

    state.downSlamCast.active = true;
    state.downSlamCast.damageResolved = false;
    state.downSlamCast.elapsedFrames = 0;
    state.downSlamCast.totalFrames = kDownSlamCastFrames;
    state.downSlamCast.origin = playerPosition;
    state.downSlamCast.impact = findDownSlamImpactPosition(map, playerPosition);
    state.lastResult = state.infiniteSoulMode ? "Black force gathers below the feet." : "Black force gathers below. Consumed 33 soul.";
}

void startHealCast(SandboxState& state) {
    state.lastAction = "Heal";
    if (state.healCast.active) {
        state.failedHealCount++;
        state.lastResult = "Already channeling heal.";
        return;
    }

    if (state.stats.health.current >= state.stats.health.maximum) {
        state.failedHealCount++;
        state.lastResult = "Health is already full.";
        return;
    }

    if (state.framesSinceLastDamage < kHealLockoutFrames) {
        state.failedHealCount++;
        state.lastResult = "Heal interrupted: wait 3 seconds after taking damage.";
        return;
    }

    if (!canSpendSoul(state, 33)) {
        state.failedHealCount++;
        state.lastResult = "Not enough soul.";
        return;
    }

    state.healCast.active = true;
    state.healCast.elapsedFrames = 0;
    state.healCast.totalFrames = kHealCastFrames;
    state.lastResult = state.infiniteSoulMode ? "Channeling heal in infinite soul mode." : "Channeling heal. Soul consumed.";
}

void updateHealCast(SandboxState& state) {
    if (!state.healCast.active) {
        return;
    }

    state.healCast.elapsedFrames++;
    state.lastAction = "Heal";

    if (state.healCast.elapsedFrames >= state.healCast.totalFrames) {
        state.healCast.active = false;
        state.stats.health.current = std::min(state.stats.health.maximum, state.stats.health.current + 1);
        state.successfulHealCount++;
        state.lastResult = "Heal completed. Restored 1 mask.";
        return;
    }

    state.lastResult = "Channeling soul heal...";
}

void updateUpWaveCast(SandboxState& state,
                      const std::string& map,
                      const game::Position& dummyPosition,
                      std::vector<SandboxEnemy>& enemies) {
    if (!state.upWaveCast.active) {
        return;
    }

    state.upWaveCast.elapsedFrames++;
    const int stage = upWaveStage(state);

    if (!state.upWaveCast.damageResolved && stage >= 3) {
        int enemyHits = 0;
        const std::vector<game::Position> damageCells = buildUpWaveDamageCells(map, state.upWaveCast.origin, stage >= 4);

        for (size_t enemyIndex = 0; enemyIndex < enemies.size(); ++enemyIndex) {
            if (!enemies[enemyIndex].groundEnemy.isAlive()) {
                continue;
            }

            const game::Position enemyPosition = enemies[enemyIndex].groundEnemy.getPosition();
            for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
                if (enemyPosition.x == damageCells[cellIndex].x && enemyPosition.y == damageCells[cellIndex].y) {
                    enemies[enemyIndex].groundEnemy.takeDamage(
                            game::DamageInfo(kPlayerAttackDamage, game::DamageType::SoulWaveUp, "player", true));
                    enemyHits++;
                    break;
                }
            }
        }

        if (enemyHits > 0) {
            const int defeatedBefore = state.defeatedGroundEnemies;
            removeDefeatedGroundEnemies(enemies, state);
            registerGroundEnemyHit(
                    state,
                    "Up Soul Wave",
                    enemyHits,
                    false,
                    0,
                    state.defeatedGroundEnemies - defeatedBefore);
        } else {
            for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
                if (dummyPosition.x == damageCells[cellIndex].x && dummyPosition.y == damageCells[cellIndex].y) {
                    registerDummyHit(state, "Up Soul Wave", false, 0);
                    break;
                }
            }
        }

        state.upWaveCast.damageResolved = true;
    }

    if (state.upWaveCast.elapsedFrames >= state.upWaveCast.totalFrames) {
        state.upWaveCast.active = false;
        if (state.lastAction == "Up Soul Wave" &&
            (state.lastResult == "Soul gathers above the head." ||
             state.lastResult == "Soul gathers upward. Consumed 33 soul.")) {
            state.lastResult = "Soul crown dissipated.";
        }
        return;
    }

    if (state.lastAction != "Up Soul Wave" || !state.upWaveCast.damageResolved) {
        state.lastAction = "Up Soul Wave";
        if (stage <= 1) {
            state.lastResult = "Soul gathers above the head.";
        } else if (stage <= 3) {
            state.lastResult = "Soul column surges upward.";
        } else {
            state.lastResult = "Soul crown bursts overhead.";
        }
    }
}

void updateDownSlamCast(SandboxState& state,
                        const std::string& map,
                        const game::Position& dummyPosition,
                        std::vector<SandboxEnemy>& enemies) {
    if (!state.downSlamCast.active) {
        return;
    }

    state.downSlamCast.elapsedFrames++;
    const int stage = downSlamStage(state);

    if (!state.downSlamCast.damageResolved && stage >= 2) {
        int enemyHits = 0;
        const std::vector<game::Position> damageCells = buildDownSlamDamageCells(state);

        for (size_t enemyIndex = 0; enemyIndex < enemies.size(); ++enemyIndex) {
            if (!enemies[enemyIndex].groundEnemy.isAlive()) {
                continue;
            }

            const game::Position enemyPosition = enemies[enemyIndex].groundEnemy.getPosition();
            for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
                if (enemyPosition.x == damageCells[cellIndex].x && enemyPosition.y == damageCells[cellIndex].y) {
                    enemies[enemyIndex].groundEnemy.takeDamage(
                            game::DamageInfo(kPlayerAttackDamage, game::DamageType::SoulSlam, "player", true));
                    enemyHits++;
                    break;
                }
            }
        }

        if (enemyHits > 0) {
            const int defeatedBefore = state.defeatedGroundEnemies;
            removeDefeatedGroundEnemies(enemies, state);
            registerGroundEnemyHit(
                    state,
                    "Down Slam",
                    enemyHits,
                    false,
                    0,
                    state.defeatedGroundEnemies - defeatedBefore);
        } else {
            for (size_t cellIndex = 0; cellIndex < damageCells.size(); ++cellIndex) {
                if (dummyPosition.x == damageCells[cellIndex].x && dummyPosition.y == damageCells[cellIndex].y) {
                    registerDummyHit(state, "Down Slam", false, 0);
                    break;
                }
            }
        }

        state.downSlamCast.damageResolved = true;
    }

    if (state.downSlamCast.elapsedFrames >= state.downSlamCast.totalFrames) {
        state.downSlamCast.active = false;
        if (state.lastAction == "Down Slam" &&
            (state.lastResult == "Black force gathers below the feet." ||
             state.lastResult == "Black force gathers below. Consumed 33 soul.")) {
            state.lastResult = "Impact haze dissipated.";
        }
        return;
    }

    if (state.lastAction != "Down Slam" || !state.downSlamCast.damageResolved) {
        state.lastAction = "Down Slam";
        if (stage == 0) {
            state.lastResult = "Black force gathers below the feet.";
        } else if (stage == 1) {
            state.lastResult = "The black spike drives downward.";
        } else if (stage == 2) {
            state.lastResult = "The ground bursts from the impact.";
        } else {
            state.lastResult = "Dark shockwave fades away.";
        }
    }

    (void)map;
}

void updateMeleeVisual(SandboxState& state) {
    if (!state.meleeVisual.active) {
        return;
    }

    state.meleeVisual.elapsedFrames++;
    if (state.meleeVisual.elapsedFrames >= state.meleeVisual.totalFrames) {
        state.meleeVisual.active = false;
    }
}

void updateProjectiles(SandboxState& state,
                       const std::string& map,
                       const game::Position& dummyPosition,
                       std::vector<SandboxEnemy>& enemies) {
    std::vector<VisualProjectile> activeProjectiles;

    for (size_t index = 0; index < state.projectiles.size(); ++index) {
        VisualProjectile projectile = state.projectiles[index];
        projectile.position.x += projectile.dx;
        projectile.position.y += projectile.dy;
        projectile.remainingFrames--;

        if (projectile.remainingFrames <= 0) {
            continue;
        }

        if (!isInsidePlayableArea(map, projectile.position)) {
            continue;
        }

        const char tile = tileAt(map, projectile.position);
        if (tile == '=') {
            continue;
        }

        const game::DamageType damageType = projectile.dy < 0
                ? game::DamageType::SoulWaveUp
                : game::DamageType::SoulWaveHorizontal;
        if (applyDamageToEnemyAtPosition(
                enemies,
                projectile.position,
                game::DamageInfo(kPlayerAttackDamage, damageType, "player", true))) {
            state.lastAction = projectile.label;
            state.lastResult = "Projectile hit 1 ground enemy.";
            continue;
        }

        if (projectile.position.x == dummyPosition.x && projectile.position.y == dummyPosition.y) {
            registerDummyHit(state, projectile.label, false, 0);
            continue;
        }

        activeProjectiles.push_back(projectile);
    }

    state.projectiles.swap(activeProjectiles);
}

void updateEffects(SandboxState& state) {
    std::vector<VisualEffect> activeEffects;

    for (size_t index = 0; index < state.effects.size(); ++index) {
        VisualEffect effect = state.effects[index];
        effect.remainingFrames--;
        if (effect.remainingFrames > 0) {
            activeEffects.push_back(effect);
        }
    }

    state.effects.swap(activeEffects);
}

void triggerDeathAnimation(SandboxState& state, const game::Position& center) {
    if (state.deathAnimation.active) {
        return;
    }

    state.deathAnimation.active = true;
    state.deathAnimation.elapsedFrames = 0;
    state.deathAnimation.center = center;
    state.projectiles.clear();
    state.effects.clear();
    state.healCast.active = false;
    state.upWaveCast.active = false;
    state.downSlamCast.active = false;
    state.meleeVisual.active = false;
    state.lastAction = "Instant Death";
    state.lastResult = "Death animation triggered. World frozen.";
}

bool updateDeathAnimation(SandboxState& state) {
    if (!state.deathAnimation.active) {
        return false;
    }

    state.deathAnimation.elapsedFrames++;
    if (state.deathAnimation.elapsedFrames >= deathAnimationTotalFrames()) {
        state.deathAnimation = DeathAnimationState();
        return true;
    }

    return false;
}

int deathExpansionRadius(const SandboxState& state) {
    if (!state.deathAnimation.active) {
        return -1;
    }

    const int expandElapsed = state.deathAnimation.elapsedFrames - (kDeathFreezeFrames + kDeathDotFrames);
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

void applyDeathAnimationOverlay(std::string& renderMap, const SandboxState& state) {
    if (!state.deathAnimation.active) {
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

    if (state.deathAnimation.elapsedFrames >= fullWhiteStart) {
        for (size_t index = 0; index < renderMap.length(); ++index) {
            if (renderMap[index] != '\n') {
                renderMap[index] = '@';
            }
        }
    } else if (state.deathAnimation.elapsedFrames >= kDeathFreezeFrames + kDeathDotFrames) {
        const int radius = deathExpansionRadius(state);
        const int innerRadius = std::max(0, radius - 2);
        const int midRadius = std::max(0, radius - 4);
        const int coreRadius = std::max(0, radius - 6);

        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                const int dx = x - state.deathAnimation.center.x;
                const int dy = y - state.deathAnimation.center.y;
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
    } else if (state.deathAnimation.elapsedFrames >= kDeathFreezeFrames) {
        placeGlyph(renderMap, state.deathAnimation.center, 'o');
    }

    if (state.deathAnimation.elapsedFrames >= textStart) {
        const std::string text = "YOU DEAD";
        const int textX = std::max(0, (mapWidth - static_cast<int>(text.length())) / 2);
        const int textY = std::max(0, mapHeight / 2);
        for (size_t index = 0; index < text.length(); ++index) {
            placeGlyph(renderMap, game::Position(textX + static_cast<int>(index), textY), text[index]);
        }
    }
}

void resetSandboxSession(SandboxState& state,
                         std::string& gameplayMap,
                         std::vector<SandboxEnemy>& groundEnemies,
                         Player& player) {
    const bool keepInfiniteSoul = state.infiniteSoulMode;
    state = SandboxState();
    state.infiniteSoulMode = keepInfiniteSoul;
    gameplayMap = kArenaMap;
    groundEnemies.clear();
    player.resetRuntimeState();
}

std::vector<std::string> buildSoulVesselLines(const SandboxState& state) {
    const int totalFillUnits = 3;
    const int fillUnits = state.stats.soul.maximum == 0
            ? 0
            : (state.stats.soul.current * totalFillUnits) / state.stats.soul.maximum;

    char fillGlyph = 'o';
    if (state.healCast.active) {
        const char pulseGlyphs[] = {'o', 'O', '*', 'O'};
        const int pulseIndex = (state.healCast.elapsedFrames / 5) % 4;
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
    lines.push_back("   " + std::to_string(state.stats.soul.current) + "/" + std::to_string(state.stats.soul.maximum));
    return lines;
}

std::string buildHealthOrbLine(const SandboxState& state) {
    const int displayedSlots = std::max(5, state.stats.health.maximum);
    const float progress = healProgress(state);
    std::ostringstream line;
    line << "HP   ";
    for (int slot = 0; slot < displayedSlots; ++slot) {
        if (slot < state.stats.health.current) {
            line << "(*)";
        } else if (state.healCast.active && slot == state.stats.health.current) {
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

std::string buildHud(const SandboxState& state, int activeGroundEnemies) {
    std::ostringstream hud;
    const std::vector<std::string> soulLines = buildSoulVesselLines(state);
    for (size_t index = 0; index < soulLines.size(); ++index) {
        hud << soulLines[index] << "\n";
    }
    hud << buildHealthOrbLine(state) << "\n";
    hud << "HP Debug " << state.stats.health.current << "/" << state.stats.health.maximum
        << " | Soul Debug " << state.stats.soul.current << "/" << state.stats.soul.maximum << "\n";
    hud << "[HeroSandbox] ESC exit | P infinite soul (" << (state.infiniteSoulMode ? "ON" : "OFF") << ")\n";
    hud << "Move A/D  Look W/S  Jump SPACE  Attack J  Spell K  W+J UpSlash  S+J DownSlash  W+K UpWave  S+K Slam  Heal R  SelfDamage H  Death X\n";
    hud << "Spawn Ground Enemy: 1 | Spawn Pair: 2 | Clear Enemies: C\n";
    hud << "Facing " << (state.facing == game::FacingDirection::Right ? "Right" : "Left")
        << " | Heal Ready " << (state.framesSinceLastDamage >= kHealLockoutFrames ? "YES" : "NO") << "\n";
    hud << "Dummy Hits " << state.dummyHitCount
        << " | Soul Charge Hits " << state.dummySoulChargeCount
        << " | Self Damage " << state.selfDamageCount
        << " | Heals " << state.successfulHealCount
        << " | Heal Fails " << state.failedHealCount << "\n";
    hud << "Ground Enemies Active " << activeGroundEnemies
        << " | Spawned " << state.groundEnemySpawns
        << " | Defeated " << state.defeatedGroundEnemies << "\n";
    hud << "Last Action: " << state.lastAction << "\n";
    hud << "Last Result: " << state.lastResult << "\n\n";
    return hud.str();
}

void overlayHealParticles(std::string& renderMap, const std::string& gameplayMap, const SandboxState& state) {
    if (!state.healCast.active) {
        return;
    }

    const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
    if (playerPosition.x < 0 || playerPosition.y < 0) {
        return;
    }

    const int stage = (state.healCast.elapsedFrames / 6) % 4;
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

void overlayUpWaveCast(std::string& renderMap, const std::string& gameplayMap, const SandboxState& state) {
    if (!state.upWaveCast.active) {
        return;
    }

    const int stage = upWaveStage(state);
    const std::vector<SpellVisualCell> cells = buildUpWaveVisualCells(stage);

    for (size_t index = 0; index < cells.size(); ++index) {
        const game::Position target(state.upWaveCast.origin.x + cells[index].dx,
                                    state.upWaveCast.origin.y + cells[index].dy);
        if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
            continue;
        }
        placeGlyph(renderMap, target, cells[index].glyph);
    }
}

void overlayDownSlamCast(std::string& renderMap, const std::string& gameplayMap, const SandboxState& state) {
    if (!state.downSlamCast.active) {
        return;
    }

    const std::vector<SpellVisualCell> cells = buildDownSlamVisualCells(state);
    for (size_t index = 0; index < cells.size(); ++index) {
        const game::Position target(state.downSlamCast.origin.x + cells[index].dx,
                                    state.downSlamCast.origin.y + cells[index].dy);
        if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
            continue;
        }
        placeGlyph(renderMap, target, cells[index].glyph);
    }
}

void overlayMeleeVisual(std::string& renderMap, const std::string& gameplayMap, const SandboxState& state) {
    if (!state.meleeVisual.active) {
        return;
    }

    const std::vector<SpellVisualCell> cells = buildMeleeVisualCells(state);
    for (size_t index = 0; index < cells.size(); ++index) {
        const game::Position target(state.meleeVisual.origin.x + cells[index].dx,
                                    state.meleeVisual.origin.y + cells[index].dy);
        if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
            continue;
        }
        placeGlyph(renderMap, target, cells[index].glyph);
    }
}

std::string buildRenderMap(const std::string& gameplayMap,
                           const SandboxState& state,
                           const std::vector<SandboxEnemy>& enemies) {
    std::string renderMap = gameplayMap;
    const game::Position dummyPosition = findGlyphPosition(gameplayMap, 'T');

    for (size_t index = 0; index < enemies.size(); ++index) {
        if (enemies[index].groundEnemy.isRenderable()) {
            placeGlyph(renderMap,
                       enemies[index].groundEnemy.getPosition(),
                       enemies[index].groundEnemy.getRenderGlyph());
        }
    }

    for (size_t index = 0; index < state.projectiles.size(); ++index) {
        const std::vector<SpellVisualCell> projectileCells = buildHorizontalWaveVisualCells(state.projectiles[index]);
        for (size_t cellIndex = 0; cellIndex < projectileCells.size(); ++cellIndex) {
            const game::Position target(state.projectiles[index].position.x + projectileCells[cellIndex].dx,
                                        state.projectiles[index].position.y + projectileCells[cellIndex].dy);
            if (!isInsidePlayableArea(gameplayMap, target) || tileAt(gameplayMap, target) == '=') {
                continue;
            }
            placeGlyph(renderMap, target, projectileCells[cellIndex].glyph);
        }
    }

    for (size_t effectIndex = 0; effectIndex < state.effects.size(); ++effectIndex) {
        for (size_t cellIndex = 0; cellIndex < state.effects[effectIndex].cells.size(); ++cellIndex) {
            placeGlyph(renderMap, state.effects[effectIndex].cells[cellIndex], state.effects[effectIndex].glyph);
        }
    }

    if (dummyPosition.x >= 0 && dummyPosition.y >= 0) {
        placeGlyph(renderMap, dummyPosition, 'T');
    }

    overlayHealParticles(renderMap, gameplayMap, state);
    overlayMeleeVisual(renderMap, gameplayMap, state);
    overlayUpWaveCast(renderMap, gameplayMap, state);
    overlayDownSlamCast(renderMap, gameplayMap, state);

    if (state.blinkFramesRemaining > 0 && ((state.blinkFramesRemaining / 5) % 2 == 0)) {
        const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
        if (playerPosition.x >= 0 && playerPosition.y >= 0) {
            placeGlyph(renderMap, playerPosition, ' ');
        }
    }

    applyDeathAnimationOverlay(renderMap, state);

    return renderMap;
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    SandboxState state;
    std::string gameplayMap = kArenaMap;
    std::vector<SandboxEnemy> groundEnemies;

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        const bool triggerInstantDeath = !state.deathAnimation.active &&
                                         (isJustPressed(keyStateManager, state, 'x') || isJustPressed(keyStateManager, state, 'X'));

        if (triggerInstantDeath) {
            triggerDeathAnimation(state, findGlyphPosition(gameplayMap, '@'));
        } else if (state.deathAnimation.active) {
            if (updateDeathAnimation(state)) {
                resetSandboxSession(state, gameplayMap, groundEnemies, player);
            }
        } else {
            const bool playerLockedByCast = state.healCast.active || state.downSlamCast.active;
            if (!playerLockedByCast) {
                if (isKeyDown(keyStateManager, 'a') || isKeyDown(keyStateManager, 'A')) {
                    state.facing = game::FacingDirection::Left;
                } else if (isKeyDown(keyStateManager, 'd') || isKeyDown(keyStateManager, 'D')) {
                    state.facing = game::FacingDirection::Right;
                }

                std::string collisionMap = buildCollisionMap(gameplayMap, groundEnemies);
                const game::Position previousPlayerPosition = findGlyphPosition(gameplayMap, '@');
                player.move(collisionMap);
                const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');

                if (previousPlayerPosition.x >= 0 && previousPlayerPosition.y >= 0) {
                    placeGlyph(gameplayMap, previousPlayerPosition, ' ');
                }
                if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0) {
                    placeGlyph(gameplayMap, movedPlayerPosition, '@');
                }
            }

            if (state.framesSinceLastDamage < kHealLockoutFrames) {
                state.framesSinceLastDamage++;
            }
            if (state.blinkFramesRemaining > 0) {
                state.blinkFramesRemaining--;
            } else {
                state.hitFeedback.blinking = false;
            }

            const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
            const game::Position dummyPosition = findGlyphPosition(gameplayMap, 'T');
            const bool dummyInMeleeRange = isDummyInMeleeRange(playerPosition, dummyPosition, state.facing);
            const bool aimingUp = isKeyDown(keyStateManager, 'w') || isKeyDown(keyStateManager, 'W');
            const bool aimingDown = isKeyDown(keyStateManager, 's') || isKeyDown(keyStateManager, 'S');
            const bool physicalPressed = isJustPressed(keyStateManager, state, 'j') || isJustPressed(keyStateManager, state, 'J');
            const bool spellPressed = isJustPressed(keyStateManager, state, 'k') || isJustPressed(keyStateManager, state, 'K');

            if (!playerLockedByCast && isJustPressed(keyStateManager, state, '1')) {
                const game::Position spawnPoint = kGroundEnemySpawnPoints[state.groundEnemySpawns % kGroundEnemySpawnPoints.size()];
                state.lastAction = "Spawn Ground Enemy";
                if (spawnGroundEnemy(groundEnemies, state, gameplayMap, playerPosition, spawnPoint)) {
                    state.lastResult = "Spawned 1 ground enemy.";
                } else {
                    state.lastResult = "Spawn point blocked.";
                }
            }

            if (!playerLockedByCast && isJustPressed(keyStateManager, state, '2')) {
                int spawned = 0;
                for (size_t index = 0; index < kGroundEnemySpawnPoints.size() && spawned < 2; ++index) {
                    if (spawnGroundEnemy(groundEnemies, state, gameplayMap, playerPosition, kGroundEnemySpawnPoints[index])) {
                        spawned++;
                    }
                }

                state.lastAction = "Spawn Ground Enemy Pair";
                if (spawned > 0) {
                    state.lastResult = "Spawned " + std::to_string(spawned) + " ground enemies.";
                } else {
                    state.lastResult = "All spawn points are blocked.";
                }
            }

            if (!playerLockedByCast && (isJustPressed(keyStateManager, state, 'c') || isJustPressed(keyStateManager, state, 'C'))) {
                groundEnemies.clear();
                state.lastAction = "Clear Ground Enemies";
                state.lastResult = "Cleared all spawned ground enemies.";
            }

            if (!playerLockedByCast && (isJustPressed(keyStateManager, state, 'p') || isJustPressed(keyStateManager, state, 'P'))) {
                state.infiniteSoulMode = !state.infiniteSoulMode;
                state.lastAction = "Toggle Infinite Soul";
                state.lastResult = state.infiniteSoulMode ? "Infinite soul enabled." : "Infinite soul disabled.";
            }

            if (!playerLockedByCast && (isJustPressed(keyStateManager, state, 'h') || isJustPressed(keyStateManager, state, 'H'))) {
                applySelfDamage(state);
            }

            if (!playerLockedByCast && physicalPressed) {
                if (aimingUp) {
                    startMeleeVisual(state, MeleeVisualType::Up, playerPosition, state.facing);
                    const int defeatedBefore = state.defeatedGroundEnemies;
                    const int enemyHits = applyDamageToGroundEnemies(
                            groundEnemies,
                            playerPosition,
                            state.facing,
                            game::DamageInfo(kPlayerAttackDamage, game::DamageType::UpSlash, "player", true));
                    removeDefeatedGroundEnemies(groundEnemies, state);

                    if (enemyHits > 0) {
                        registerGroundEnemyHit(state, "Up Slash", enemyHits, false, 0, state.defeatedGroundEnemies - defeatedBefore);
                    } else if (dummyInMeleeRange) {
                        registerDummyHit(state, "Up Slash", false, 0);
                    } else {
                        state.lastAction = "Up Slash";
                        state.lastResult = "Slash triggered. No target in range.";
                    }
                } else if (aimingDown) {
                    startMeleeVisual(state, MeleeVisualType::Down, playerPosition, state.facing);
                    const int defeatedBefore = state.defeatedGroundEnemies;
                    const int enemyHits = applyDamageToGroundEnemies(
                            groundEnemies,
                            playerPosition,
                            state.facing,
                            game::DamageInfo(kPlayerAttackDamage, game::DamageType::DownSlash, "player", true));
                    removeDefeatedGroundEnemies(groundEnemies, state);

                    if (enemyHits > 0) {
                        registerGroundEnemyHit(state, "Down Slash", enemyHits, false, 0, state.defeatedGroundEnemies - defeatedBefore);
                    } else if (dummyInMeleeRange) {
                        registerDummyHit(state, "Down Slash", false, 0);
                    } else {
                        state.lastAction = "Down Slash";
                        state.lastResult = "Slash triggered. No target in range.";
                    }
                } else {
                    startMeleeVisual(state, MeleeVisualType::Horizontal, playerPosition, state.facing);
                    const int defeatedBefore = state.defeatedGroundEnemies;
                    const int enemyHits = applyDamageToGroundEnemies(
                            groundEnemies,
                            playerPosition,
                            state.facing,
                            game::DamageInfo(kPlayerAttackDamage, game::DamageType::BasicAttack, "player", true));
                    removeDefeatedGroundEnemies(groundEnemies, state);
                    const int defeatedThisHit = state.defeatedGroundEnemies - defeatedBefore;

                    if (enemyHits > 0) {
                        registerGroundEnemyHit(state, "Basic Attack", enemyHits, true, 11, defeatedThisHit);
                    } else if (dummyInMeleeRange) {
                        registerDummyHit(state, "Basic Attack", true, 11);
                    } else {
                        state.lastAction = "Basic Attack";
                        state.lastResult = "No target in range.";
                    }
                }
            }

            if (!playerLockedByCast && spellPressed) {
                if (aimingUp) {
                    castUpWave(state, playerPosition);
                } else if (aimingDown) {
                    castDownSlam(state, gameplayMap, playerPosition, dummyPosition, groundEnemies);
                } else {
                    castHorizontalWave(state, playerPosition);
                }
            }

            if (!playerLockedByCast && (isJustPressed(keyStateManager, state, 'r') || isJustPressed(keyStateManager, state, 'R'))) {
                startHealCast(state);
            }

            updateDownSlamCast(state, gameplayMap, dummyPosition, groundEnemies);
            updateUpWaveCast(state, gameplayMap, dummyPosition, groundEnemies);
            updateMeleeVisual(state);
            updateProjectiles(state, gameplayMap, dummyPosition, groundEnemies);
            updateEffects(state);
            updateHealCast(state);
            updateGroundEnemies(groundEnemies, gameplayMap, playerPosition);
            removeDefeatedGroundEnemies(groundEnemies, state);

            state.stats.soul.maximum = kSoulMeterMax;
            if (!state.infiniteSoulMode && state.stats.soul.current > state.stats.soul.maximum) {
                state.stats.soul.current = state.stats.soul.maximum;
            }

            if (state.infiniteSoulMode) {
                state.stats.soul.current = state.stats.soul.maximum;
            }
        }

        mapDrawer.currentmap = buildHud(state, static_cast<int>(groundEnemies.size())) + buildRenderMap(gameplayMap, state, groundEnemies);
        mapDrawer.draw();

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
