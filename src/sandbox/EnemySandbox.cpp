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
const int kPlayerInvulnerabilityFrames = 45;
const int kFlyingProjectileStepFrames = 3;
const int kMaxEnemies = 16;

const std::string kArenaTemplate =
        "============================================================================\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                   ========                                               =\n"
        "=                                                                          =\n"
        "=                                                 ========                 =\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=   @                                                                      =\n"
        "============================================================================";

enum class SandboxEnemyType {
    Ground,
    Flying
};

struct SandboxEnemy {
    int id;
    SandboxEnemyType type;
    game::GroundEnemy groundEnemy;
    game::FlyingEnemy flyingEnemy;
};

struct EnemyProjectile {
    game::Position position;
    int dx;
    int dy;
    int stepCooldown;
    int remainingFrames;
};

struct SandboxState {
    game::CharacterStats stats;
    bool invincible;
    bool freezeAi;
    bool playerBlinking;
    int invulnerabilityFramesRemaining;
    int hitsTaken;
    int groundSpawns;
    int flyingSpawns;
    int totalSpawned;
    int nextEnemyId;
    std::string lastEvent;
    std::vector<EnemyProjectile> enemyProjectiles;
    std::unordered_map<int, bool> previousKeys;

    SandboxState()
        : invincible(false),
          freezeAi(false),
          playerBlinking(false),
          invulnerabilityFramesRemaining(0),
          hitsTaken(0),
          groundSpawns(0),
          flyingSpawns(0),
          totalSpawned(0),
          nextEnemyId(1),
          lastEvent("Ready. Press 1/2/3 to spawn enemies.") {
    }
};

const std::vector<game::Position> kGroundSpawnPoints = {
    game::Position(20, 8),
    game::Position(34, 8),
    game::Position(48, 8),
    game::Position(62, 8)
};

const std::vector<game::Position> kFlyingSpawnPoints = {
    game::Position(18, 2),
    game::Position(34, 4),
    game::Position(52, 2),
    game::Position(66, 4)
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

bool isEnemyAt(const std::vector<SandboxEnemy>& enemies, const game::Position& position, int ignoreId) {
    for (size_t index = 0; index < enemies.size(); ++index) {
        const game::Position enemyPosition = enemies[index].type == SandboxEnemyType::Ground
                                                 ? enemies[index].groundEnemy.getPosition()
                                                 : enemies[index].flyingEnemy.getPosition();
        if (enemies[index].id != ignoreId &&
            enemyPosition.x == position.x &&
            enemyPosition.y == position.y) {
            return true;
        }
    }

    return false;
}

bool canEnemyOccupy(const std::string& terrainMap,
                    const std::vector<SandboxEnemy>& enemies,
                    const game::Position& playerPosition,
                    const game::Position& targetPosition,
                    int ignoreId) {
    if (!isInsidePlayableArea(terrainMap, targetPosition)) {
        return false;
    }

    if (tileAt(terrainMap, targetPosition) != ' ') {
        return false;
    }

    if (targetPosition.x == playerPosition.x && targetPosition.y == playerPosition.y) {
        return false;
    }

    return !isEnemyAt(enemies, targetPosition, ignoreId);
}

std::string buildCollisionMap(const std::string& terrainMap,
                              const game::Position& playerPosition,
                              const std::vector<SandboxEnemy>& enemies) {
    std::string collisionMap = terrainMap;
    placeGlyph(collisionMap, playerPosition, '@');

    for (size_t index = 0; index < enemies.size(); ++index) {
        const game::Position enemyPosition = enemies[index].type == SandboxEnemyType::Ground
                                                 ? enemies[index].groundEnemy.getPosition()
                                                 : enemies[index].flyingEnemy.getPosition();
        placeGlyph(collisionMap,
                   enemyPosition,
                   enemies[index].type == SandboxEnemyType::Ground ? 'g' : 'f');
    }

    return collisionMap;
}

std::string buildRenderMap(const std::string& terrainMap,
                           const game::Position& playerPosition,
                           const std::vector<SandboxEnemy>& enemies,
                           const SandboxState& state) {
    std::string renderMap = terrainMap;

    for (size_t index = 0; index < enemies.size(); ++index) {
        const game::Position enemyPosition = enemies[index].type == SandboxEnemyType::Ground
                                                 ? enemies[index].groundEnemy.getPosition()
                                                 : enemies[index].flyingEnemy.getPosition();
        if (enemies[index].type == SandboxEnemyType::Ground) {
            if (enemies[index].groundEnemy.isRenderable()) {
                placeGlyph(renderMap, enemyPosition, enemies[index].groundEnemy.getRenderGlyph());
            }
        } else {
            if (enemies[index].flyingEnemy.isRenderable()) {
                placeGlyph(renderMap, enemyPosition, enemies[index].flyingEnemy.getRenderGlyph());
            }
        }

        if (enemies[index].type == SandboxEnemyType::Ground &&
            enemies[index].groundEnemy.getState() == game::GroundEnemyState::AttackStartup) {
            const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
            placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
            placeGlyph(renderMap, warningRow, '!');
            placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
        } else if (enemies[index].type == SandboxEnemyType::Flying &&
                   enemies[index].flyingEnemy.getState() == game::FlyingEnemyState::AttackStartup) {
            const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
            placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
            placeGlyph(renderMap, warningRow, '!');
            placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
        }
    }

    for (size_t index = 0; index < state.enemyProjectiles.size(); ++index) {
        placeGlyph(renderMap, state.enemyProjectiles[index].position, '*');
    }

    if (!state.playerBlinking || ((state.invulnerabilityFramesRemaining / 4) % 2 == 0)) {
        placeGlyph(renderMap, playerPosition, '@');
    }

    return renderMap;
}

std::string buildHud(const SandboxState& state, const std::vector<SandboxEnemy>& enemies) {
    std::ostringstream hud;
    int groundCount = 0;
    int flyingCount = 0;

    for (size_t index = 0; index < enemies.size(); ++index) {
        if (enemies[index].type == SandboxEnemyType::Ground) {
            groundCount++;
        } else {
            flyingCount++;
        }
    }

    hud << "[EnemySandbox] ESC exit | 1 ground | 2 flying | 3 mixed wave | C clear | R reset\n";
    hud << "I toggle invincible (" << (state.invincible ? "ON" : "OFF") << ")"
        << " | P freeze AI (" << (state.freezeAi ? "ON" : "OFF") << ")\n";
    hud << "HP " << state.stats.health.current << "/" << state.stats.health.maximum
        << " | Hits Taken " << state.hitsTaken
        << " | Active Enemies " << enemies.size()
        << " | Ground " << groundCount
        << " | Flying " << flyingCount << "\n";
    hud << "Spawned Total " << state.totalSpawned
        << " | Ground Spawns " << state.groundSpawns
        << " | Flying Spawns " << state.flyingSpawns
        << " | Enemy Shots " << state.enemyProjectiles.size() << "\n";
    hud << "Last Event: " << state.lastEvent << "\n\n";
    return hud.str();
}

void resetSandbox(std::string& terrainMap,
                  game::Position& playerPosition,
                  std::vector<SandboxEnemy>& enemies,
                  SandboxState& state) {
    terrainMap = kArenaTemplate;
    playerPosition = findGlyphPosition(terrainMap, '@');
    placeGlyph(terrainMap, playerPosition, ' ');
    enemies.clear();
    state.stats = game::CharacterStats();
    state.invincible = false;
    state.freezeAi = false;
    state.playerBlinking = false;
    state.invulnerabilityFramesRemaining = 0;
    state.hitsTaken = 0;
    state.groundSpawns = 0;
    state.flyingSpawns = 0;
    state.totalSpawned = 0;
    state.nextEnemyId = 1;
    state.enemyProjectiles.clear();
    state.lastEvent = "Sandbox reset.";
}

bool spawnEnemy(std::vector<SandboxEnemy>& enemies,
                SandboxState& state,
                const std::string& terrainMap,
                const game::Position& playerPosition,
                const game::Position& spawnPosition,
                SandboxEnemyType type) {
    if (static_cast<int>(enemies.size()) >= kMaxEnemies) {
        state.lastEvent = "Enemy cap reached. Clear or reset before spawning more.";
        return false;
    }

    if (!canEnemyOccupy(terrainMap, enemies, playerPosition, spawnPosition, -1)) {
        state.lastEvent = "Spawn point blocked. Move the player or clear enemies.";
        return false;
    }

    SandboxEnemy enemy;
    enemy.id = state.nextEnemyId++;
    enemy.type = type;

    if (type == SandboxEnemyType::Ground) {
        enemy.groundEnemy = game::GroundEnemy("ground_enemy_" + std::to_string(enemy.id), spawnPosition);
    } else {
        enemy.flyingEnemy = game::FlyingEnemy("flying_enemy_" + std::to_string(enemy.id), spawnPosition);
    }

    enemies.push_back(enemy);

    state.totalSpawned++;
    if (type == SandboxEnemyType::Ground) {
        state.groundSpawns++;
        state.lastEvent = "Spawned 1 ground enemy.";
    } else {
        state.flyingSpawns++;
        state.lastEvent = "Spawned 1 flying enemy.";
    }

    return true;
}

void spawnMixedWave(std::vector<SandboxEnemy>& enemies,
                    SandboxState& state,
                    const std::string& terrainMap,
                    const game::Position& playerPosition) {
    int spawned = 0;

    for (size_t index = 0; index < kGroundSpawnPoints.size() && spawned < 3; ++index) {
        if (spawnEnemy(enemies, state, terrainMap, playerPosition, kGroundSpawnPoints[index], SandboxEnemyType::Ground)) {
            spawned++;
        }
    }

    for (size_t index = 0; index < kFlyingSpawnPoints.size() && spawned < 5; ++index) {
        if (spawnEnemy(enemies, state, terrainMap, playerPosition, kFlyingSpawnPoints[index], SandboxEnemyType::Flying)) {
            spawned++;
        }
    }

    if (spawned > 0) {
        state.lastEvent = "Spawned mixed pressure wave.";
    }
}

void damagePlayer(SandboxState& state, const std::string& sourceLabel) {
    if (state.invincible) {
        state.lastEvent = sourceLabel + " hit the player, but invincible mode ignored the damage.";
        return;
    }

    if (state.invulnerabilityFramesRemaining > 0) {
        return;
    }

    if (state.stats.health.current > 0) {
        state.stats.health.current--;
    }

    state.hitsTaken++;
    state.playerBlinking = true;
    state.invulnerabilityFramesRemaining = kPlayerInvulnerabilityFrames;

    if (state.stats.health.current <= 0) {
        state.stats.health.current = 0;
        state.lastEvent = sourceLabel + " defeated the player. Press R to reset the arena.";
    } else {
        state.lastEvent = sourceLabel + " dealt 1 damage to the player.";
    }
}

void spawnFlyingProjectile(const game::FlyingEnemy& enemy,
                           const game::Position& playerPosition,
                           SandboxState& state) {
    EnemyProjectile projectile;
    projectile.position = enemy.getPosition();
    if (playerPosition.x > enemy.getPosition().x) {
        projectile.dx = 1;
    } else if (playerPosition.x < enemy.getPosition().x) {
        projectile.dx = -1;
    } else {
        projectile.dx = 0;
    }

    if (playerPosition.y < enemy.getPosition().y - 1) {
        projectile.dy = -1;
    } else if (playerPosition.y > enemy.getPosition().y + 1) {
        projectile.dy = 1;
    } else {
        projectile.dy = 0;
    }

    projectile.stepCooldown = kFlyingProjectileStepFrames;
    projectile.remainingFrames = 45;
    state.enemyProjectiles.push_back(projectile);
}

void updateEnemyProjectiles(const std::string& terrainMap,
                            const game::Position& playerPosition,
                            SandboxState& state) {
    std::vector<EnemyProjectile> nextProjectiles;
    nextProjectiles.reserve(state.enemyProjectiles.size());

    for (size_t index = 0; index < state.enemyProjectiles.size(); ++index) {
        EnemyProjectile projectile = state.enemyProjectiles[index];

        if (projectile.remainingFrames <= 0) {
            continue;
        }

        projectile.remainingFrames--;

        if (projectile.stepCooldown > 0) {
            projectile.stepCooldown--;
            nextProjectiles.push_back(projectile);
            continue;
        }

        projectile.stepCooldown = kFlyingProjectileStepFrames;
        projectile.position.x += projectile.dx;
        projectile.position.y += projectile.dy;

        if (!isInsidePlayableArea(terrainMap, projectile.position) ||
            tileAt(terrainMap, projectile.position) != ' ') {
            continue;
        }

        if (projectile.position.x == playerPosition.x && projectile.position.y == playerPosition.y) {
            damagePlayer(state, "Flying fireball");
            continue;
        }

        nextProjectiles.push_back(projectile);
    }

    state.enemyProjectiles.swap(nextProjectiles);
}

void updateGroundEnemy(SandboxEnemy& enemy,
                       const std::string& terrainMap,
                       const std::vector<SandboxEnemy>& enemies,
                       const game::Position& playerPosition,
                       SandboxState& state) {
    const game::Position previousPosition = enemy.groundEnemy.getPosition();
    enemy.groundEnemy.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
    const game::Position nextPosition = enemy.groundEnemy.getPosition();

    if ((nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
        !canEnemyOccupy(terrainMap, enemies, playerPosition, nextPosition, enemy.id)) {
        enemy.groundEnemy.setPosition(previousPosition);
    }

    if (enemy.groundEnemy.consumeAttackTrigger()) {
        if (enemy.groundEnemy.isTouchingPlayer(playerPosition) ||
            enemy.groundEnemy.isInAttackRange(playerPosition)) {
            damagePlayer(state, "Ground enemy");
        }
    }
}

void updateFlyingEnemy(SandboxEnemy& enemy,
                       const std::string& terrainMap,
                       const std::vector<SandboxEnemy>& enemies,
                       const game::Position& playerPosition,
                       SandboxState& state) {
    const game::Position previousPosition = enemy.flyingEnemy.getPosition();
    enemy.flyingEnemy.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
    const game::Position nextPosition = enemy.flyingEnemy.getPosition();

    if (enemy.flyingEnemy.isAlive() &&
        (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
        !canEnemyOccupy(terrainMap, enemies, playerPosition, nextPosition, enemy.id)) {
        enemy.flyingEnemy.setPosition(previousPosition);
    }

    if (enemy.flyingEnemy.consumeProjectileTrigger()) {
        spawnFlyingProjectile(enemy.flyingEnemy, playerPosition, state);
        state.lastEvent = "Flying enemy launched a fireball.";
    }
}

void updateEnemies(std::vector<SandboxEnemy>& enemies,
                   const std::string& terrainMap,
                   const game::Position& playerPosition,
                   SandboxState& state) {
    for (size_t index = 0; index < enemies.size(); ++index) {
        SandboxEnemy enemySnapshot = enemies[index];
        if (enemySnapshot.type == SandboxEnemyType::Ground) {
            updateGroundEnemy(enemies[index], terrainMap, enemies, playerPosition, state);
        } else {
            updateFlyingEnemy(enemies[index], terrainMap, enemies, playerPosition, state);
        }
    }
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    SandboxState state;
    std::string terrainMap;
    game::Position playerPosition;
    std::vector<SandboxEnemy> enemies;

    resetSandbox(terrainMap, playerPosition, enemies, state);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        if (isJustPressed(keyStateManager, state, 'r') || isJustPressed(keyStateManager, state, 'R')) {
            resetSandbox(terrainMap, playerPosition, enemies, state);
        }

        if (isJustPressed(keyStateManager, state, 'i') || isJustPressed(keyStateManager, state, 'I')) {
            state.invincible = !state.invincible;
            state.lastEvent = state.invincible ? "Invincible mode enabled." : "Invincible mode disabled.";
        }

        if (isJustPressed(keyStateManager, state, 'p') || isJustPressed(keyStateManager, state, 'P')) {
            state.freezeAi = !state.freezeAi;
            state.lastEvent = state.freezeAi ? "Enemy AI frozen." : "Enemy AI resumed.";
        }

        if (isJustPressed(keyStateManager, state, 'c') || isJustPressed(keyStateManager, state, 'C')) {
            enemies.clear();
            state.enemyProjectiles.clear();
            state.lastEvent = "Cleared all active enemies.";
        }

        if (isJustPressed(keyStateManager, state, '1')) {
            const game::Position spawnPoint = kGroundSpawnPoints[state.groundSpawns % kGroundSpawnPoints.size()];
            spawnEnemy(enemies, state, terrainMap, playerPosition, spawnPoint, SandboxEnemyType::Ground);
        }

        if (isJustPressed(keyStateManager, state, '2')) {
            const game::Position spawnPoint = kFlyingSpawnPoints[state.flyingSpawns % kFlyingSpawnPoints.size()];
            spawnEnemy(enemies, state, terrainMap, playerPosition, spawnPoint, SandboxEnemyType::Flying);
        }

        if (isJustPressed(keyStateManager, state, '3')) {
            spawnMixedWave(enemies, state, terrainMap, playerPosition);
        }

        if (state.stats.health.current > 0) {
            std::string collisionMap = buildCollisionMap(terrainMap, playerPosition, enemies);
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

        if (!state.freezeAi && state.stats.health.current > 0) {
            updateEnemies(enemies, terrainMap, playerPosition, state);
            updateEnemyProjectiles(terrainMap, playerPosition, state);
        }

        mapDrawer.currentmap = buildHud(state, enemies) + buildRenderMap(terrainMap, playerPosition, enemies, state);
        mapDrawer.draw();

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
