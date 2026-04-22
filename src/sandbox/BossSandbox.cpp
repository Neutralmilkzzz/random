#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "combat/CombatSystem.h"
#include "combat/CombatTuning.h"
#include "enemy/Enemy.h"
#include "input/KeyStateManager.h"
#include "player/Player.h"
#include "shared/GameTypes.h"
#include "world/MapDrawer.h"

namespace {

const int kFrameMs = 16;
const int kPlayerInvulnerabilityFrames = 45;
const int kProjectileStepFrames = 3;
const int kImpactFrames = 6;
const int kMeteorWarningFrames = 18;
const int kMeteorBlastFrames = 6;

const std::string kArenaTemplate =
        "============================================================================\n"
        "=                                                                          =\n"
        "=                 ==========                                               =\n"
        "=                                                                          =\n"
        "=                                           ===========                    =\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=   @                                                                      =\n"
        "=                                                                          =\n"
        "============================================================================";

const game::Position kMeleeBossSpawn(58, 8);
const game::Position kRangedBossSpawn(56, 3);

enum class SandboxBossType {
    None,
    Melee,
    Ranged
};

struct GlyphCell {
    game::Position position;
    char glyph;

    GlyphCell(const game::Position& cellPosition = game::Position(), char cellGlyph = ' ')
        : position(cellPosition),
          glyph(cellGlyph) {
    }
};

struct BossProjectile {
    game::Position position;
    int dx;
    int dy;
    int stepCooldown;
    int remainingFrames;

    BossProjectile()
        : dx(0),
          dy(0),
          stepCooldown(kProjectileStepFrames),
          remainingFrames(45) {
    }
};

struct BossImpact {
    std::vector<GlyphCell> activeGlyphs;
    std::vector<GlyphCell> fadeGlyphs;
    std::vector<game::Position> damageCells;
    int activeFramesRemaining;
    int fadeFramesRemaining;
    std::string label;

    BossImpact()
        : activeFramesRemaining(0),
          fadeFramesRemaining(0) {
    }
};

struct MeteorStrike {
    std::vector<GlyphCell> warningGlyphs;
    std::vector<GlyphCell> blastGlyphs;
    std::vector<game::Position> damageCells;
    int warningFramesRemaining;
    int blastFramesRemaining;
    bool damageResolved;

    MeteorStrike()
        : warningFramesRemaining(kMeteorWarningFrames),
          blastFramesRemaining(0),
          damageResolved(false) {
    }
};

struct SandboxBoss {
    SandboxBossType type;
    bool active;
    game::MeleeBoss meleeBoss;
    game::RangedBoss rangedBoss;

    SandboxBoss()
        : type(SandboxBossType::None),
          active(false) {
    }
};

struct SandboxState {
    game::CharacterStats playerStats;
    game::CombatSystem combatSystem;
    bool invincible;
    bool freezeAi;
    bool playerBlinking;
    int invulnerabilityFramesRemaining;
    int bossDefeats;
    int manualHitsLanded;
    int totalHkdEarned;
    std::string lastEvent;
    std::vector<BossProjectile> projectiles;
    std::vector<BossImpact> impacts;
    std::vector<MeteorStrike> meteors;
    std::unordered_map<int, bool> previousKeys;

    SandboxState()
        : invincible(false),
          freezeAi(false),
          playerBlinking(false),
          invulnerabilityFramesRemaining(0),
          bossDefeats(0),
          manualHitsLanded(0),
          totalHkdEarned(0),
          lastEvent("Ready. Press 1/2 to spawn a boss, H/J to damage-test.") {
        playerStats.health = game::ResourcePool(5, 5);
        playerStats.soul = game::ResourcePool(33, 99);
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
    const size_t height = std::count(map.begin(), map.end(), '\n') + 1;

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

void placeGlyphCells(std::string& map, const std::vector<GlyphCell>& glyphs) {
    for (size_t index = 0; index < glyphs.size(); ++index) {
        placeGlyph(map, glyphs[index].position, glyphs[index].glyph);
    }
}

GlyphCell toGlyphCell(const game::BossVisualGlyph& glyph) {
    return GlyphCell(glyph.position, glyph.glyph);
}

BossImpact toBossImpact(const game::BossAttackVisual& visual) {
    BossImpact impact;
    impact.damageCells = visual.damageCells;
    impact.activeFramesRemaining = visual.activeFrames;
    impact.fadeFramesRemaining = visual.fadeFrames;
    impact.label = visual.label;

    for (size_t index = 0; index < visual.activeGlyphs.size(); ++index) {
        impact.activeGlyphs.push_back(toGlyphCell(visual.activeGlyphs[index]));
    }
    for (size_t index = 0; index < visual.fadeGlyphs.size(); ++index) {
        impact.fadeGlyphs.push_back(toGlyphCell(visual.fadeGlyphs[index]));
    }

    return impact;
}

game::Boss& getBoss(SandboxBoss& boss) {
    return boss.type == SandboxBossType::Melee
            ? static_cast<game::Boss&>(boss.meleeBoss)
            : static_cast<game::Boss&>(boss.rangedBoss);
}

const game::Boss& getBoss(const SandboxBoss& boss) {
    return boss.type == SandboxBossType::Melee
            ? static_cast<const game::Boss&>(boss.meleeBoss)
            : static_cast<const game::Boss&>(boss.rangedBoss);
}

std::string bossTypeLabel(const SandboxBoss& boss) {
    if (!boss.active) {
        return "None";
    }

    return boss.type == SandboxBossType::Melee ? "Melee Boss" : "Ranged Boss";
}

std::string bossStateLabel(game::BossState state) {
    switch (state) {
    case game::BossState::Dormant:
        return "Dormant";
    case game::BossState::Intro:
        return "Intro";
    case game::BossState::Positioning:
        return "Positioning";
    case game::BossState::AttackStartup:
        return "Startup";
    case game::BossState::AttackRecovery:
        return "Recovery";
    case game::BossState::Staggered:
        return "Staggered";
    case game::BossState::Dead:
        return "Dead";
    }

    return "Unknown";
}

bool canBossOccupy(const std::string& terrainMap,
                   const game::Position& playerPosition,
                   const game::Position& targetPosition) {
    if (!isInsidePlayableArea(terrainMap, targetPosition)) {
        return false;
    }

    if (tileAt(terrainMap, targetPosition) != ' ') {
        return false;
    }

    return !(targetPosition.x == playerPosition.x && targetPosition.y == playerPosition.y);
}

std::string buildCollisionMap(const std::string& terrainMap,
                              const game::Position& playerPosition,
                              const SandboxBoss& boss) {
    std::string collisionMap = terrainMap;
    if (boss.active && getBoss(boss).isAlive()) {
        placeGlyph(collisionMap, getBoss(boss).getPosition(), getBoss(boss).getRenderGlyph());
    }
    placeGlyph(collisionMap, playerPosition, '@');
    return collisionMap;
}

std::string buildRenderMap(const std::string& terrainMap,
                           const game::Position& playerPosition,
                           const SandboxBoss& boss,
                           const SandboxState& state) {
    std::string renderMap = terrainMap;

    for (size_t index = 0; index < state.impacts.size(); ++index) {
        placeGlyphCells(renderMap,
                        state.impacts[index].activeFramesRemaining > 0
                                ? state.impacts[index].activeGlyphs
                                : state.impacts[index].fadeGlyphs);
    }

    for (size_t index = 0; index < state.projectiles.size(); ++index) {
        placeGlyph(renderMap, state.projectiles[index].position, '*');
    }

    for (size_t index = 0; index < state.meteors.size(); ++index) {
        placeGlyphCells(renderMap,
                        state.meteors[index].warningFramesRemaining > 0
                                ? state.meteors[index].warningGlyphs
                                : state.meteors[index].blastGlyphs);
    }

    if (boss.active && getBoss(boss).isRenderable()) {
        const game::Boss& activeBoss = getBoss(boss);
        const std::vector<game::BossVisualGlyph> bodyVisual = activeBoss.buildBodyVisual();
        for (size_t index = 0; index < bodyVisual.size(); ++index) {
            placeGlyph(renderMap, bodyVisual[index].position, bodyVisual[index].glyph);
        }

        if (activeBoss.getState() == game::BossState::AttackStartup) {
            const std::vector<game::BossVisualGlyph> startupVisual = activeBoss.buildStartupVisual();
            for (size_t index = 0; index < startupVisual.size(); ++index) {
                placeGlyph(renderMap, startupVisual[index].position, startupVisual[index].glyph);
            }
        }
    }

    if (!state.playerBlinking || ((state.invulnerabilityFramesRemaining / 4) % 2 == 0)) {
        placeGlyph(renderMap, playerPosition, '@');
    }

    return renderMap;
}

std::string buildHud(const SandboxBoss& boss, const SandboxState& state) {
    std::ostringstream hud;
    hud << "[BossSandbox] ESC exit | 1 melee demo | 2 ranged proto | H light hit(1) | J heavy hit(1)\n";
    hud << "R reset | C clear boss | I invincible (" << (state.invincible ? "ON" : "OFF") << ")"
        << " | P freeze AI (" << (state.freezeAi ? "ON" : "OFF") << ")\n";
    hud << "Player HP " << state.playerStats.health.current << "/" << state.playerStats.health.maximum
        << " | Soul " << state.playerStats.soul.current << "/" << state.playerStats.soul.maximum
        << " | HKD " << state.playerStats.hkd
        << " | Boss Defeats " << state.bossDefeats
        << " | Manual Hits " << state.manualHitsLanded << "\n";

    if (boss.active) {
        const game::Boss& activeBoss = getBoss(boss);
        hud << bossTypeLabel(boss)
            << " | HP " << activeBoss.getStats().health.current << "/" << activeBoss.getStats().health.maximum
            << " | State " << bossStateLabel(activeBoss.getState())
            << " | Stagger " << activeBoss.getStaggerDamage() << "/" << activeBoss.getStaggerThreshold();
        if (activeBoss.isStaggerWindowActive()) {
            hud << " | Window " << activeBoss.getStaggerWindowRemaining() << "s";
        }
        hud << "\n";
    } else {
        hud << "Boss: none active.\n";
    }

    hud << "Boss Shots " << state.projectiles.size()
        << " | Impacts " << state.impacts.size()
        << " | Meteors " << state.meteors.size()
        << " | Total HKD Earned " << state.totalHkdEarned << "\n";
    hud << "Last Event: " << state.lastEvent << "\n\n";
    return hud.str();
}

void resetSandbox(std::string& terrainMap,
                  game::Position& playerPosition,
                  SandboxBoss& boss,
                  SandboxState& state) {
    terrainMap = kArenaTemplate;
    playerPosition = findGlyphPosition(terrainMap, '@');
    placeGlyph(terrainMap, playerPosition, ' ');

    boss = SandboxBoss();
    state = SandboxState();
    state.lastEvent = "Boss sandbox reset.";
}

void damagePlayer(SandboxState& state, const std::string& sourceLabel) {
    if (state.invincible) {
        state.lastEvent = sourceLabel + " hit the player, but invincible mode ignored the damage.";
        return;
    }

    if (state.invulnerabilityFramesRemaining > 0) {
        return;
    }

    if (state.playerStats.health.current > 0) {
        state.playerStats.health.current--;
    }

    state.playerBlinking = true;
    state.invulnerabilityFramesRemaining = kPlayerInvulnerabilityFrames;

    if (state.playerStats.health.current <= 0) {
        state.playerStats.health.current = 0;
        state.lastEvent = sourceLabel + " defeated the player. Press R to reset.";
    } else {
        state.lastEvent = sourceLabel + " dealt 1 damage to the player.";
    }
}

void applyImpactIfPlayerInside(const BossImpact& impact,
                               const game::Position& playerPosition,
                               SandboxState& state) {
    for (size_t index = 0; index < impact.damageCells.size(); ++index) {
        if (impact.damageCells[index].x == playerPosition.x && impact.damageCells[index].y == playerPosition.y) {
            damagePlayer(state, impact.label);
            return;
        }
    }
}

void spawnFireballBurst(const game::BossAttackSignal& signal,
                        SandboxState& state) {
    for (int dy = -1; dy <= 1; ++dy) {
        BossProjectile projectile;
        projectile.position = signal.origin;
        projectile.dx = signal.facingDirection == game::FacingDirection::Right ? 1 : -1;
        projectile.dy = dy;
        state.projectiles.push_back(projectile);
    }
}

void spawnMeteorDrop(const std::string& terrainMap,
                     const game::Position& playerPosition,
                     SandboxState& state) {
    for (int offset = -1; offset <= 1; ++offset) {
        const game::Position target(playerPosition.x + offset, playerPosition.y);
        if (!isInsidePlayableArea(terrainMap, target) || tileAt(terrainMap, target) != ' ') {
            continue;
        }

        MeteorStrike strike;
        strike.damageCells.push_back(target);
        strike.damageCells.push_back(game::Position(target.x, target.y - 1));
        strike.warningGlyphs.push_back(GlyphCell(game::Position(target.x - 1, target.y), '^'));
        strike.warningGlyphs.push_back(GlyphCell(target, '!'));
        strike.warningGlyphs.push_back(GlyphCell(game::Position(target.x + 1, target.y), '^'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x - 1, target.y - 1), '\\'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x, target.y - 1), '|'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x + 1, target.y - 1), '/'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x - 1, target.y), '-'));
        strike.blastGlyphs.push_back(GlyphCell(target, '*'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x + 1, target.y), '-'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x - 1, target.y + 1), '/'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x, target.y + 1), '|'));
        strike.blastGlyphs.push_back(GlyphCell(game::Position(target.x + 1, target.y + 1), '\\'));
        state.meteors.push_back(strike);
    }
}

void resolveBossSignal(const std::string& terrainMap,
                       const game::Position& playerPosition,
                       SandboxBoss& boss,
                       const game::BossAttackSignal& signal,
                       SandboxState& state) {
    game::Boss& activeBoss = getBoss(boss);
    state.combatSystem.queueAttack(activeBoss.getAttackForType(signal.type));

    const game::BossAttackVisual attackVisual = activeBoss.buildResolvedAttackVisual(signal);
    if (!attackVisual.activeGlyphs.empty() ||
        !attackVisual.fadeGlyphs.empty() ||
        !attackVisual.damageCells.empty()) {
        BossImpact impact = toBossImpact(attackVisual);
        applyImpactIfPlayerInside(impact, playerPosition, state);
        state.impacts.push_back(impact);

        if (attackVisual.hasLandingPosition &&
            isInsidePlayableArea(terrainMap, attackVisual.landingPosition) &&
            tileAt(terrainMap, attackVisual.landingPosition) == ' ' &&
            !(attackVisual.landingPosition.x == playerPosition.x &&
              attackVisual.landingPosition.y == playerPosition.y)) {
            activeBoss.setPosition(attackVisual.landingPosition);
        }

        if (signal.type == game::BossAttackType::SweepSlash) {
            state.lastEvent = "Boss released a sweep slash.";
        } else if (signal.type == game::BossAttackType::DashSlash) {
            state.lastEvent = "Boss burst forward with a dash slash.";
        } else if (signal.type == game::BossAttackType::JumpSlash) {
            state.lastEvent = "Boss leapt in with a jump slash.";
        }
        return;
    }

    if (signal.type == game::BossAttackType::FireballBurst) {
        spawnFireballBurst(signal, state);
        state.lastEvent = "Ranged boss launched a fireball burst.";
        return;
    }

    if (signal.type == game::BossAttackType::MeteorDrop) {
        spawnMeteorDrop(terrainMap, playerPosition, state);
        state.lastEvent = "Ranged boss marked a meteor drop.";
    }
}

void updateImpacts(SandboxState& state) {
    std::vector<BossImpact> nextImpacts;
    nextImpacts.reserve(state.impacts.size());

    for (size_t index = 0; index < state.impacts.size(); ++index) {
        BossImpact impact = state.impacts[index];
        if (impact.activeFramesRemaining > 0) {
            impact.activeFramesRemaining--;
            nextImpacts.push_back(impact);
            continue;
        }

        if (impact.fadeFramesRemaining > 0) {
            impact.fadeFramesRemaining--;
            if (impact.fadeFramesRemaining > 0) {
                nextImpacts.push_back(impact);
            }
        }
    }

    state.impacts.swap(nextImpacts);
}

void updateProjectiles(const std::string& terrainMap,
                       const game::Position& playerPosition,
                       SandboxState& state) {
    std::vector<BossProjectile> nextProjectiles;
    nextProjectiles.reserve(state.projectiles.size());

    for (size_t index = 0; index < state.projectiles.size(); ++index) {
        BossProjectile projectile = state.projectiles[index];
        if (projectile.remainingFrames <= 0) {
            continue;
        }

        projectile.remainingFrames--;
        if (projectile.stepCooldown > 0) {
            projectile.stepCooldown--;
            nextProjectiles.push_back(projectile);
            continue;
        }

        projectile.stepCooldown = kProjectileStepFrames;
        projectile.position.x += projectile.dx;
        projectile.position.y += projectile.dy;

        if (!isInsidePlayableArea(terrainMap, projectile.position) ||
            tileAt(terrainMap, projectile.position) != ' ') {
            continue;
        }

        if (projectile.position.x == playerPosition.x && projectile.position.y == playerPosition.y) {
            damagePlayer(state, "Boss projectile");
            continue;
        }

        nextProjectiles.push_back(projectile);
    }

    state.projectiles.swap(nextProjectiles);
}

void updateMeteors(const game::Position& playerPosition, SandboxState& state) {
    std::vector<MeteorStrike> nextMeteors;
    nextMeteors.reserve(state.meteors.size());

    for (size_t index = 0; index < state.meteors.size(); ++index) {
        MeteorStrike strike = state.meteors[index];
        if (strike.warningFramesRemaining > 0) {
            strike.warningFramesRemaining--;
            if (strike.warningFramesRemaining == 0) {
                strike.blastFramesRemaining = kMeteorBlastFrames;
            }
            nextMeteors.push_back(strike);
            continue;
        }

        if (!strike.damageResolved) {
            strike.damageResolved = true;
            for (size_t cellIndex = 0; cellIndex < strike.damageCells.size(); ++cellIndex) {
                if (strike.damageCells[cellIndex].x == playerPosition.x &&
                    strike.damageCells[cellIndex].y == playerPosition.y) {
                    damagePlayer(state, "Meteor strike");
                    break;
                }
            }
        }

        if (strike.blastFramesRemaining > 0) {
            strike.blastFramesRemaining--;
            if (strike.blastFramesRemaining > 0) {
                nextMeteors.push_back(strike);
            }
        }
    }

    state.meteors.swap(nextMeteors);
}

void spawnBoss(SandboxBoss& boss,
               SandboxState& state,
               SandboxBossType type) {
    boss = SandboxBoss();
    boss.type = type;
    boss.active = true;

    if (type == SandboxBossType::Melee) {
        boss.meleeBoss = game::MeleeBoss("melee_boss_1", kMeleeBossSpawn);
        state.lastEvent = "Spawned melee boss demo: slash, dash, jump slash.";
    } else {
        boss.rangedBoss = game::RangedBoss("ranged_boss_1", kRangedBossSpawn);
        state.lastEvent = "Spawned ranged boss prototype.";
    }

    state.impacts.clear();
    state.projectiles.clear();
    state.meteors.clear();
}

void hitBoss(SandboxBoss& boss,
             SandboxState& state,
             const game::AttackDefinition& attack,
             const std::string& label) {
    if (!boss.active || !getBoss(boss).isAlive()) {
        state.lastEvent = "No live boss to hit.";
        return;
    }

    game::DamageResolution resolution = state.combatSystem.resolveAttack(getBoss(boss), attack);
    if (!resolution.hitApplied) {
        state.lastEvent = label + " did not land.";
        return;
    }

    state.manualHitsLanded++;
    if (resolution.soulGranted > 0) {
        state.playerStats.soul.current = std::min(state.playerStats.soul.maximum,
                                                  state.playerStats.soul.current + resolution.soulGranted);
    }

    if (resolution.targetDefeated) {
        game::RewardResolution reward;
        if (!getBoss(boss).consumeDefeatReward(reward)) {
            reward = state.combatSystem.buildEnemyReward(getBoss(boss));
        }
        state.combatSystem.applyReward(state.playerStats, reward);
        state.totalHkdEarned += reward.hkdGranted;
        state.bossDefeats++;
        std::ostringstream message;
        message << label << " defeated the boss. Reward +" << reward.hkdGranted << " HKD.";
        state.lastEvent = message.str();
        return;
    }

    std::ostringstream message;
    message << label << " dealt " << resolution.damageApplied << " damage.";
    if (getBoss(boss).shouldEnterStagger()) {
        message << " Stagger threshold reached.";
    }
    state.lastEvent = message.str();
}

void updateBoss(const std::string& terrainMap,
                const game::Position& playerPosition,
                SandboxBoss& boss,
                SandboxState& state) {
    if (!boss.active) {
        return;
    }

    game::Boss& activeBoss = getBoss(boss);
    const game::Position previousPosition = activeBoss.getPosition();
    activeBoss.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
    const game::Position nextPosition = activeBoss.getPosition();

    if (activeBoss.isAlive() &&
        (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
        !canBossOccupy(terrainMap, playerPosition, nextPosition)) {
        activeBoss.setPosition(previousPosition);
    }

    game::BossAttackSignal signal;
    if (activeBoss.consumeAttackSignal(signal)) {
        resolveBossSignal(terrainMap, playerPosition, boss, signal, state);
    }

    if (activeBoss.shouldDespawn()) {
        boss.active = false;
        state.lastEvent = "Boss death animation finished.";
    }
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    SandboxState state;
    SandboxBoss boss;
    std::string terrainMap;
    game::Position playerPosition;

    resetSandbox(terrainMap, playerPosition, boss, state);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        if (isJustPressed(keyStateManager, state, 'r') || isJustPressed(keyStateManager, state, 'R')) {
            resetSandbox(terrainMap, playerPosition, boss, state);
        }

        if (isJustPressed(keyStateManager, state, 'i') || isJustPressed(keyStateManager, state, 'I')) {
            state.invincible = !state.invincible;
            state.lastEvent = state.invincible ? "Invincible mode enabled." : "Invincible mode disabled.";
        }

        if (isJustPressed(keyStateManager, state, 'p') || isJustPressed(keyStateManager, state, 'P')) {
            state.freezeAi = !state.freezeAi;
            state.lastEvent = state.freezeAi ? "Boss AI frozen." : "Boss AI resumed.";
        }

        if (isJustPressed(keyStateManager, state, 'c') || isJustPressed(keyStateManager, state, 'C')) {
            boss = SandboxBoss();
            state.projectiles.clear();
            state.impacts.clear();
            state.meteors.clear();
            state.lastEvent = "Cleared active boss and hazards.";
        }

        if (isJustPressed(keyStateManager, state, '1')) {
            spawnBoss(boss, state, SandboxBossType::Melee);
        }

        if (isJustPressed(keyStateManager, state, '2')) {
            spawnBoss(boss, state, SandboxBossType::Ranged);
        }

        if (isJustPressed(keyStateManager, state, 'h') || isJustPressed(keyStateManager, state, 'H')) {
            game::AttackDefinition lightHit;
            lightHit.id = "sandbox_light_hit";
            lightHit.damage = game::DamageInfo(game::kBaseNailDamage,
                                               game::DamageType::BasicAttack,
                                               "boss_sandbox_player",
                                               true);
            lightHit.soulGainOnHit = 11;
            hitBoss(boss, state, lightHit, "Light hit");
        }

        if (isJustPressed(keyStateManager, state, 'j') || isJustPressed(keyStateManager, state, 'J')) {
            game::AttackDefinition heavyHit;
            heavyHit.id = "sandbox_heavy_hit";
            heavyHit.damage = game::DamageInfo(game::kSpellDamage,
                                               game::DamageType::SoulWaveHorizontal,
                                               "boss_sandbox_player",
                                               false);
            heavyHit.soulGainOnHit = 18;
            hitBoss(boss, state, heavyHit, "Heavy hit");
        }

        if (state.playerStats.health.current > 0) {
            std::string collisionMap = buildCollisionMap(terrainMap, playerPosition, boss);
            player.move(collisionMap);
            const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');
            if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0) {
                playerPosition = movedPlayerPosition;
            }
        }

        if (state.invulnerabilityFramesRemaining > 0) {
            state.invulnerabilityFramesRemaining--;
            if (state.invulnerabilityFramesRemaining == 0) {
                state.playerBlinking = false;
            }
        }

        if (!state.freezeAi && state.playerStats.health.current > 0) {
            updateBoss(terrainMap, playerPosition, boss, state);
            updateProjectiles(terrainMap, playerPosition, state);
            updateMeteors(playerPosition, state);
        }

        updateImpacts(state);
        state.combatSystem.update(static_cast<float>(kFrameMs) / 1000.0f);

        mapDrawer.currentmap = buildHud(boss, state) + buildRenderMap(terrainMap, playerPosition, boss, state);
        mapDrawer.draw();

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
