#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

struct VisualProjectile {
    game::Position position;
    int dx;
    int dy;
    int remainingFrames;
    char glyph;
    std::string label;
};

struct VisualEffect {
    std::vector<game::Position> cells;
    int remainingFrames;
    char glyph;
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
    int framesSinceLastDamage;
    int blinkFramesRemaining;
    HealCastState healCast;
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
          framesSinceLastDamage(kHealLockoutFrames),
          blinkFramesRemaining(0),
          lastAction("Ready"),
          lastResult("A/D move, SPACE jump, ESC exit.") {
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
    projectile.glyph = '=';
    projectile.label = "Horizontal wave";
    state.projectiles.push_back(projectile);
    state.lastResult = state.infiniteSoulMode ? "Cast in infinite soul mode." : "Consumed 33 soul.";
}

void castUpWave(SandboxState& state, const game::Position& playerPosition) {
    state.lastAction = "Up Soul Wave";
    if (!canSpendSoul(state, 33)) {
        state.lastResult = "Not enough soul.";
        return;
    }

    VisualProjectile projectile;
    projectile.position = game::Position(playerPosition.x, playerPosition.y - 1);
    projectile.dx = 0;
    projectile.dy = -1;
    projectile.remainingFrames = 18;
    projectile.glyph = '|';
    projectile.label = "Up wave";
    state.projectiles.push_back(projectile);
    state.lastResult = state.infiniteSoulMode ? "Cast in infinite soul mode." : "Consumed 33 soul.";
}

void castDownSlam(SandboxState& state,
                  const std::string& map,
                  const game::Position& playerPosition,
                  const game::Position& dummyPosition) {
    state.lastAction = "Down Slam";
    if (!canSpendSoul(state, 33)) {
        state.lastResult = "Not enough soul.";
        return;
    }

    VisualEffect effect;
    effect.remainingFrames = 8;
    effect.glyph = '!';

    for (int y = playerPosition.y + 1; y < dummyPosition.y + 2; ++y) {
        game::Position cell(playerPosition.x, y);
        if (!isInsidePlayableArea(map, cell)) {
            break;
        }
        if (tileAt(map, cell) == '=') {
            break;
        }
        effect.cells.push_back(cell);
    }

    state.effects.push_back(effect);

    if (dummyPosition.x == playerPosition.x && dummyPosition.y > playerPosition.y) {
        registerDummyHit(state, "Down Slam", false, 0);
        return;
    }

    state.lastResult = state.infiniteSoulMode ? "Cast in infinite soul mode." : "Consumed 33 soul.";
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

void updateProjectiles(SandboxState& state, const std::string& map, const game::Position& dummyPosition) {
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

std::vector<std::string> buildSoulVesselLines(const SandboxState& state) {
    const int rimWidth = 8;
    const int upperWidth = 10;
    const int lowerWidth = 10;
    const int baseWidth = 8;
    const int totalFillUnits = rimWidth + upperWidth + lowerWidth + baseWidth;
    const int fillUnits = state.stats.soul.maximum == 0
            ? 0
            : (state.stats.soul.current * totalFillUnits) / state.stats.soul.maximum;

    int remainingFill = fillUnits;

    const int baseFill = std::min(baseWidth, remainingFill);
    remainingFill -= baseFill;
    const int lowerFill = std::min(lowerWidth, remainingFill);
    remainingFill -= lowerFill;
    const int upperFill = std::min(upperWidth, remainingFill);
    remainingFill -= upperFill;
    const int rimFill = std::min(rimWidth, remainingFill);

    char fillGlyph = '#';
    if (state.healCast.active) {
        const char pulseGlyphs[] = {'@', '*', '#', '*'};
        const int pulseIndex = (state.healCast.elapsedFrames / 5) % 4;
        fillGlyph = pulseGlyphs[pulseIndex];
    }

    std::vector<std::string> lines;
    lines.push_back("SOUL");
    lines.push_back("      .-''''-.");
    lines.push_back("   .-'        '-.");
    lines.push_back("  /  " + std::string(rimFill, fillGlyph) + std::string(rimWidth - rimFill, ' ') + "  \\");
    lines.push_back(" |   " + std::string(upperFill, fillGlyph) + std::string(upperWidth - upperFill, ' ') + "   |");
    lines.push_back(" |   " + std::string(lowerFill, fillGlyph) + std::string(lowerWidth - lowerFill, ' ') + "   |");
    lines.push_back("  \\  " + std::string(baseFill, fillGlyph) + std::string(baseWidth - baseFill, ' ') + "  /");
    lines.push_back("   '-.________.-'");
    lines.push_back("      " + std::to_string(state.stats.soul.current) + "/" + std::to_string(state.stats.soul.maximum));
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

std::string buildHud(const SandboxState& state) {
    std::ostringstream hud;
    const std::vector<std::string> soulLines = buildSoulVesselLines(state);
    for (size_t index = 0; index < soulLines.size(); ++index) {
        hud << soulLines[index] << "\n";
    }
    hud << buildHealthOrbLine(state) << "\n";
    hud << "HP Debug " << state.stats.health.current << "/" << state.stats.health.maximum
        << " | Soul Debug " << state.stats.soul.current << "/" << state.stats.soul.maximum << "\n";
    hud << "[HeroSandbox] ESC exit | P infinite soul (" << (state.infiniteSoulMode ? "ON" : "OFF") << ")\n";
    hud << "Move A/D  Jump SPACE  Basic J  Up I  Down K  Wave L  UpWave O  Slam M  Heal R  SelfDamage H\n";
    hud << "Facing " << (state.facing == game::FacingDirection::Right ? "Right" : "Left")
        << " | Heal Ready " << (state.framesSinceLastDamage >= kHealLockoutFrames ? "YES" : "NO") << "\n";
    hud << "Dummy Hits " << state.dummyHitCount
        << " | Soul Charge Hits " << state.dummySoulChargeCount
        << " | Self Damage " << state.selfDamageCount
        << " | Heals " << state.successfulHealCount
        << " | Heal Fails " << state.failedHealCount << "\n";
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

std::string buildRenderMap(const std::string& gameplayMap, const SandboxState& state) {
    std::string renderMap = gameplayMap;
    const game::Position dummyPosition = findGlyphPosition(gameplayMap, 'T');

    for (size_t index = 0; index < state.projectiles.size(); ++index) {
        placeGlyph(renderMap, state.projectiles[index].position, state.projectiles[index].glyph);
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

    if (state.blinkFramesRemaining > 0 && ((state.blinkFramesRemaining / 5) % 2 == 0)) {
        const game::Position playerPosition = findGlyphPosition(gameplayMap, '@');
        if (playerPosition.x >= 0 && playerPosition.y >= 0) {
            placeGlyph(renderMap, playerPosition, ' ');
        }
    }

    return renderMap;
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    SandboxState state;
    std::string gameplayMap = kArenaMap;

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        const bool playerLockedByHeal = state.healCast.active;
        if (!playerLockedByHeal) {
            if (isKeyDown(keyStateManager, 'a') || isKeyDown(keyStateManager, 'A')) {
                state.facing = game::FacingDirection::Left;
            } else if (isKeyDown(keyStateManager, 'd') || isKeyDown(keyStateManager, 'D')) {
                state.facing = game::FacingDirection::Right;
            }

            player.move(gameplayMap);
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

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'p') || isJustPressed(keyStateManager, state, 'P'))) {
            state.infiniteSoulMode = !state.infiniteSoulMode;
            state.lastAction = "Toggle Infinite Soul";
            state.lastResult = state.infiniteSoulMode ? "Infinite soul enabled." : "Infinite soul disabled.";
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'h') || isJustPressed(keyStateManager, state, 'H'))) {
            applySelfDamage(state);
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'j') || isJustPressed(keyStateManager, state, 'J'))) {
            state.lastAction = "Basic Attack";
            if (dummyInMeleeRange) {
                registerDummyHit(state, "Basic Attack", true, 11);
            } else {
                state.lastResult = "No target in range.";
            }
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'i') || isJustPressed(keyStateManager, state, 'I'))) {
            state.lastAction = "Up Slash";
            if (dummyInMeleeRange) {
                registerDummyHit(state, "Up Slash", false, 0);
            } else {
                state.lastResult = "Slash triggered. No target in range.";
            }
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'k') || isJustPressed(keyStateManager, state, 'K'))) {
            state.lastAction = "Down Slash";
            if (dummyInMeleeRange) {
                registerDummyHit(state, "Down Slash", false, 0);
            } else {
                state.lastResult = "Slash triggered. No target in range.";
            }
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'l') || isJustPressed(keyStateManager, state, 'L'))) {
            castHorizontalWave(state, playerPosition);
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'o') || isJustPressed(keyStateManager, state, 'O'))) {
            castUpWave(state, playerPosition);
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'm') || isJustPressed(keyStateManager, state, 'M'))) {
            castDownSlam(state, gameplayMap, playerPosition, dummyPosition);
        }

        if (!playerLockedByHeal && (isJustPressed(keyStateManager, state, 'r') || isJustPressed(keyStateManager, state, 'R'))) {
            startHealCast(state);
        }

        updateProjectiles(state, gameplayMap, dummyPosition);
        updateEffects(state);
        updateHealCast(state);

        if (state.infiniteSoulMode) {
            state.stats.soul.current = state.stats.soul.maximum;
        }

        mapDrawer.currentmap = buildHud(state) + buildRenderMap(gameplayMap, state);
        mapDrawer.draw();

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
