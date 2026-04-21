#include <chrono>
#include <sstream>
#include <string>
#include <thread>

#include "enemy/Enemy.h"
#include "input/KeyStateManager.h"
#include "player/Player.h"
#include "world/MapDrawer.h"

namespace {

const int kFrameMs = 16;
const game::Position kGroundEnemySpawnPosition(24, 14);
const std::string kMainTerrainWithPlayer =
        "============================================================================\n"
        "=                                                                          =\n"
        "=                                                                          =\n"
        "=                                          ==========                      =\n"
        "=                                                                          =\n"
        "=                                    ===                                   =\n"
        "=                                                                          =\n"
        "=                              ===                                         =\n"
        "=                                                                          =\n"
        "=                       ===                                                =\n"
        "=                                                                          =\n"
        "=                 ===                                                      =\n"
        "=                                                                          =\n"
        "=         =======                                                          =\n"
        "=  @                                                                       =\n"
        "============================================================================\n"
        "============================================================================\n"
        "============================================================================";

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

std::string buildTerrainMap(const std::string& sourceMap, game::Position& playerPosition) {
    std::string terrainMap = sourceMap;
    playerPosition = findGlyphPosition(terrainMap, '@');
    if (playerPosition.x >= 0 && playerPosition.y >= 0) {
        placeGlyph(terrainMap, playerPosition, ' ');
    }
    return terrainMap;
}

bool canEnemyOccupy(const std::string& terrainMap,
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
                              const game::GroundEnemy& groundEnemy) {
    std::string collisionMap = terrainMap;
    placeGlyph(collisionMap, playerPosition, '@');

    if (groundEnemy.isAlive()) {
        placeGlyph(collisionMap, groundEnemy.getPosition(), 'g');
    }

    return collisionMap;
}

std::string buildWorldStatus(const game::GroundEnemy& groundEnemy) {
    std::ostringstream status;
    status << "Ground Enemy ";

    if (groundEnemy.shouldDespawn()) {
        status << "cleared";
    } else if (groundEnemy.isAlive()) {
        status << "active";
    } else {
        status << "dying";
    }

    status << " | Enemy HP " << groundEnemy.getStats().health.current << "/" << groundEnemy.getStats().health.maximum << "\n\n";
    return status.str();
}

void resetRuntime(std::string& terrainMap,
                  game::Position& playerPosition,
                  game::GroundEnemy& groundEnemy,
                  Player& player) {
    terrainMap = buildTerrainMap(kMainTerrainWithPlayer, playerPosition);
    groundEnemy = game::GroundEnemy("main_ground_enemy", kGroundEnemySpawnPosition);
    player.resetRuntimeState();
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    std::string terrainMap;
    game::Position playerPosition;
    game::GroundEnemy groundEnemy;

    resetRuntime(terrainMap, playerPosition, groundEnemy, player);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (keyStateManager.keyStates[0x1B]) {
            break;
        }

        if (keyStateManager.keyStates['p'] || keyStateManager.keyStates['P']) {
            resetRuntime(terrainMap, playerPosition, groundEnemy, player);
        }

        std::string collisionMap = buildCollisionMap(terrainMap, playerPosition, groundEnemy);
        if (player.isAlive() && !player.isMovementLocked()) {
            player.move(collisionMap);
            const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');
            if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0) {
                playerPosition = movedPlayerPosition;
            }
        }

        std::string gameplayMap = terrainMap;
        placeGlyph(gameplayMap, playerPosition, '@');

        player.updateCombat(gameplayMap, groundEnemy);

        if (groundEnemy.isRenderable() && player.isAlive()) {
            const game::Position previousPosition = groundEnemy.getPosition();
            groundEnemy.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
            const game::Position nextPosition = groundEnemy.getPosition();

            if (groundEnemy.isAlive() &&
                (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                !canEnemyOccupy(terrainMap, playerPosition, nextPosition)) {
                groundEnemy.setPosition(previousPosition);
            }

            if (groundEnemy.consumeAttackTrigger()) {
                if (groundEnemy.isTouchingPlayer(playerPosition) || groundEnemy.isInAttackRange(playerPosition)) {
                    player.receiveDamage("Ground enemy");
                }
            }
        }

        std::string renderMap = terrainMap;
        if (groundEnemy.isRenderable()) {
            const game::Position enemyPosition = groundEnemy.getPosition();
            placeGlyph(renderMap, enemyPosition, groundEnemy.getRenderGlyph());

            if (groundEnemy.getState() == game::GroundEnemyState::AttackStartup) {
                const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
                placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
                placeGlyph(renderMap, warningRow, '!');
                placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
            }
        }

        player.overlayRender(renderMap, gameplayMap);
        mapDrawer.currentmap = player.buildHud() + buildWorldStatus(groundEnemy) + renderMap;
        mapDrawer.draw();

        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
