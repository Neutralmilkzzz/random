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
const int kTitleScreenWidth = 76;
const int kTitleScreenHeight = 24;
const int kTitleUnavailableHintFrames = 90;
const int kTitleStartTransitionFrames = 24;
const game::Position kGroundEnemySpawnPosition(24, 14);
const game::Position kFlyingEnemySpawnPosition(40, 6);
const int kFlyingProjectileStepFrames = 3;
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

struct EnemyProjectile {
    game::Position position;
    int dx;
    int dy;
    int stepCooldown;
    int remainingFrames;
};

enum class TitleMenuSelection {
    NewGame,
    ContinueGame
};

enum class TitleScreenState {
    TitleScreen,
    ContinueUnavailable,
    StartTransition
};

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

bool isKeyDown(const KeyStateManager& keyStateManager, int keyCode) {
    const std::unordered_map<int, bool>::const_iterator it = keyStateManager.keyStates.find(keyCode);
    return it != keyStateManager.keyStates.end() && it->second;
}

bool wasKeyDown(const std::unordered_map<int, bool>& previousKeys, int keyCode) {
    const std::unordered_map<int, bool>::const_iterator it = previousKeys.find(keyCode);
    return it != previousKeys.end() && it->second;
}

bool isJustPressed(const KeyStateManager& keyStateManager,
                   const std::unordered_map<int, bool>& previousKeys,
                   int keyCode) {
    return isKeyDown(keyStateManager, keyCode) && !wasKeyDown(previousKeys, keyCode);
}

std::vector<std::string> createTitleCanvas() {
    std::vector<std::string> lines;
    lines.push_back(std::string(kTitleScreenWidth, '='));
    for (int row = 0; row < kTitleScreenHeight - 2; ++row) {
        lines.push_back("=" + std::string(kTitleScreenWidth - 2, ' ') + "=");
    }
    lines.push_back(std::string(kTitleScreenWidth, '='));
    return lines;
}

void placeCenteredLine(std::vector<std::string>& lines, int row, const std::string& text) {
    if (row < 0 || row >= static_cast<int>(lines.size()) || lines[row].size() < 3) {
        return;
    }

    const int innerWidth = static_cast<int>(lines[row].size()) - 2;
    int start = 1 + (innerWidth - static_cast<int>(text.size())) / 2;
    if (start < 1) {
        start = 1;
    }

    for (size_t index = 0; index < text.size() && start + static_cast<int>(index) < static_cast<int>(lines[row].size()) - 1; ++index) {
        lines[row][start + static_cast<int>(index)] = text[index];
    }
}

void placeTitleParticle(std::vector<std::string>& lines, int row, int col, char glyph) {
    if (row <= 0 || row >= static_cast<int>(lines.size()) - 1) {
        return;
    }
    if (col <= 0 || col >= static_cast<int>(lines[row].size()) - 1) {
        return;
    }
    lines[row][col] = glyph;
}

std::string buildTitleScreenMap(TitleMenuSelection selection,
                                TitleScreenState state,
                                int animationFrame) {
    std::vector<std::string> lines = createTitleCanvas();

    const char particleA = (animationFrame / 24) % 3 == 1 ? '\'' : '.';
    const char particleB = (animationFrame / 30) % 4 == 2 ? '\'' : '.';
    const char particleC = (animationFrame / 36) % 5 == 3 ? '\'' : '.';
    placeTitleParticle(lines, 3, 31, particleA);
    placeTitleParticle(lines, 3, 45, particleB);
    placeTitleParticle(lines, 18, 20, particleB);
    placeTitleParticle(lines, 18, 57, particleA);
    placeTitleParticle(lines, 20, 38, particleC);

    placeCenteredLine(lines, 5, "#####   #   #  #####  ######");
    placeCenteredLine(lines, 6, "  #     #   #  #        ##  ");
    placeCenteredLine(lines, 7, "  #     #####  #####    ##  ");
    placeCenteredLine(lines, 8, "  #     #   #  #        ##  ");
    placeCenteredLine(lines, 9, "  #     #   #  #####    ##  ");
    placeCenteredLine(lines, 11, "FALLEN SOUL");

    const std::string newGameLine = selection == TitleMenuSelection::NewGame
            ? (((animationFrame / 20) % 2 == 0) ? "> NEW GAME" : ">> NEW GAME")
            : "  NEW GAME";
    const std::string continueLine = selection == TitleMenuSelection::ContinueGame
            ? (((animationFrame / 20) % 2 == 0) ? "> CONTINUE (NO SAVE)" : ">> CONTINUE (NO SAVE)")
            : "  CONTINUE (NO SAVE)";

    placeCenteredLine(lines, 14, newGameLine);
    placeCenteredLine(lines, 15, continueLine);
    placeCenteredLine(lines, 17, "------------------");

    if (state == TitleScreenState::ContinueUnavailable) {
        placeCenteredLine(lines, 18, "NO SAVE DATA TO CONTINUE");
    } else if (state == TitleScreenState::StartTransition) {
        placeCenteredLine(lines, 18, "ENTERING...");
    } else {
        placeCenteredLine(lines, 18, "W/S SELECT  ENTER CONFIRM  ESC EXIT");
    }

    std::ostringstream frame;
    for (size_t index = 0; index < lines.size(); ++index) {
        frame << lines[index];
        if (index + 1 < lines.size()) {
            frame << "\n";
        }
    }
    return frame.str();
}

bool runTitleScreen(MapDrawer& mapDrawer, KeyStateManager& keyStateManager) {
    TitleMenuSelection selection = TitleMenuSelection::NewGame;
    TitleScreenState state = TitleScreenState::TitleScreen;
    int stateFrames = 0;
    int animationFrame = 0;
    std::unordered_map<int, bool> previousKeys;
    const bool hasSaveData = false;

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            return false;
        }

        if (state == TitleScreenState::StartTransition) {
            stateFrames++;
            mapDrawer.currentmap = buildTitleScreenMap(selection, state, animationFrame++);
            mapDrawer.draw();
            previousKeys = keyStateManager.keyStates;
            if (stateFrames >= kTitleStartTransitionFrames) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
            continue;
        }

        if (isJustPressed(keyStateManager, previousKeys, 'w') || isJustPressed(keyStateManager, previousKeys, 'W')) {
            selection = TitleMenuSelection::NewGame;
            if (state == TitleScreenState::ContinueUnavailable) {
                state = TitleScreenState::TitleScreen;
            }
        } else if (isJustPressed(keyStateManager, previousKeys, 's') || isJustPressed(keyStateManager, previousKeys, 'S')) {
            selection = TitleMenuSelection::ContinueGame;
            if (state == TitleScreenState::ContinueUnavailable) {
                state = TitleScreenState::TitleScreen;
            }
        }

        if (isJustPressed(keyStateManager, previousKeys, 0x0D)) {
            if (selection == TitleMenuSelection::NewGame) {
                state = TitleScreenState::StartTransition;
                stateFrames = 0;
            } else if (hasSaveData) {
                state = TitleScreenState::StartTransition;
                stateFrames = 0;
            } else {
                state = TitleScreenState::ContinueUnavailable;
                stateFrames = 0;
            }
        }

        if (state == TitleScreenState::ContinueUnavailable) {
            stateFrames++;
            if (stateFrames >= kTitleUnavailableHintFrames) {
                state = TitleScreenState::TitleScreen;
                stateFrames = 0;
            }
        }

        mapDrawer.currentmap = buildTitleScreenMap(selection, state, animationFrame++);
        mapDrawer.draw();
        previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }
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
                    const game::Position& targetPosition,
                    const game::Position& otherEnemyPosition,
                    bool otherEnemyBlocks) {
    if (!isInsidePlayableArea(terrainMap, targetPosition)) {
        return false;
    }

    if (tileAt(terrainMap, targetPosition) != ' ') {
        return false;
    }

    if (targetPosition.x == playerPosition.x && targetPosition.y == playerPosition.y) {
        return false;
    }

    if (otherEnemyBlocks &&
        targetPosition.x == otherEnemyPosition.x &&
        targetPosition.y == otherEnemyPosition.y) {
        return false;
    }

    return true;
}

std::string buildCollisionMap(const std::string& terrainMap,
                              const game::Position& playerPosition,
                              const game::GroundEnemy& groundEnemy,
                              const game::FlyingEnemy& flyingEnemy) {
    std::string collisionMap = terrainMap;
    placeGlyph(collisionMap, playerPosition, '@');

    if (groundEnemy.isAlive()) {
        placeGlyph(collisionMap, groundEnemy.getPosition(), 'g');
    }

    if (flyingEnemy.isAlive()) {
        placeGlyph(collisionMap, flyingEnemy.getPosition(), 'f');
    }

    return collisionMap;
}

std::string buildWorldStatus(const game::GroundEnemy& groundEnemy,
                             const game::FlyingEnemy& flyingEnemy) {
    std::ostringstream status;
    status << "Ground Enemy ";

    if (groundEnemy.shouldDespawn()) {
        status << "cleared";
    } else if (groundEnemy.isAlive()) {
        status << "active";
    } else {
        status << "dying";
    }

    status << " | HP " << groundEnemy.getStats().health.current << "/" << groundEnemy.getStats().health.maximum;
    status << " || Flying Enemy ";

    if (flyingEnemy.shouldDespawn()) {
        status << "cleared";
    } else if (flyingEnemy.isAlive()) {
        status << "active";
    } else {
        status << "dying";
    }

    status << " | HP " << flyingEnemy.getStats().health.current << "/" << flyingEnemy.getStats().health.maximum << "\n\n";
    return status.str();
}

void spawnFlyingProjectile(const game::FlyingEnemy& enemy,
                           const game::Position& playerPosition,
                           std::vector<EnemyProjectile>& enemyProjectiles) {
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
    enemyProjectiles.push_back(projectile);
}

void updateEnemyProjectiles(const std::string& terrainMap,
                            const game::Position& playerPosition,
                            std::vector<EnemyProjectile>& enemyProjectiles,
                            Player& player) {
    std::vector<EnemyProjectile> nextProjectiles;
    nextProjectiles.reserve(enemyProjectiles.size());

    for (size_t index = 0; index < enemyProjectiles.size(); ++index) {
        EnemyProjectile projectile = enemyProjectiles[index];
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
            player.receiveDamage("Flying fireball", playerPosition);
            continue;
        }

        nextProjectiles.push_back(projectile);
    }

    enemyProjectiles.swap(nextProjectiles);
}

void resetRuntime(std::string& terrainMap,
                  game::Position& playerPosition,
                  game::GroundEnemy& groundEnemy,
                  game::FlyingEnemy& flyingEnemy,
                  std::vector<EnemyProjectile>& enemyProjectiles,
                  Player& player) {
    terrainMap = buildTerrainMap(kMainTerrainWithPlayer, playerPosition);
    groundEnemy = game::GroundEnemy("main_ground_enemy", kGroundEnemySpawnPosition);
    flyingEnemy = game::FlyingEnemy("main_flying_enemy", kFlyingEnemySpawnPosition);
    enemyProjectiles.clear();
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
    game::FlyingEnemy flyingEnemy;
    std::vector<EnemyProjectile> enemyProjectiles;

    if (!runTitleScreen(mapDrawer, keyStateManager)) {
        return 0;
    }

    resetRuntime(terrainMap, playerPosition, groundEnemy, flyingEnemy, enemyProjectiles, player);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (keyStateManager.keyStates[0x1B]) {
            break;
        }

        if (keyStateManager.keyStates['p'] || keyStateManager.keyStates['P']) {
            resetRuntime(terrainMap, playerPosition, groundEnemy, flyingEnemy, enemyProjectiles, player);
        }

        std::string collisionMap = buildCollisionMap(terrainMap, playerPosition, groundEnemy, flyingEnemy);
        if (player.isAlive() && !player.isMovementLocked()) {
            player.move(collisionMap);
            const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');
            if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0) {
                playerPosition = movedPlayerPosition;
            }
        }

        std::string gameplayMap = terrainMap;
        placeGlyph(gameplayMap, playerPosition, '@');

        player.updateCombat(gameplayMap, groundEnemy, flyingEnemy);

        if (groundEnemy.isRenderable() && player.isAlive()) {
            const game::Position previousPosition = groundEnemy.getPosition();
            groundEnemy.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
            const game::Position nextPosition = groundEnemy.getPosition();

            if (groundEnemy.isAlive() &&
                (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                !canEnemyOccupy(terrainMap, playerPosition, nextPosition, flyingEnemy.getPosition(), flyingEnemy.isAlive())) {
                groundEnemy.setPosition(previousPosition);
            }

            if (groundEnemy.consumeAttackTrigger()) {
                if (groundEnemy.isTouchingPlayer(playerPosition) || groundEnemy.isInAttackRange(playerPosition)) {
                    player.receiveDamage("Ground enemy", playerPosition);
                }
            }
        }

        if (flyingEnemy.isRenderable() && player.isAlive()) {
            const game::Position previousPosition = flyingEnemy.getPosition();
            flyingEnemy.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
            const game::Position nextPosition = flyingEnemy.getPosition();

            if (flyingEnemy.isAlive() &&
                (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                !canEnemyOccupy(terrainMap, playerPosition, nextPosition, groundEnemy.getPosition(), groundEnemy.isAlive())) {
                flyingEnemy.setPosition(previousPosition);
            }

            if (flyingEnemy.consumeProjectileTrigger()) {
                spawnFlyingProjectile(flyingEnemy, playerPosition, enemyProjectiles);
            }
        }

        updateEnemyProjectiles(terrainMap, playerPosition, enemyProjectiles, player);

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

        if (flyingEnemy.isRenderable()) {
            const game::Position enemyPosition = flyingEnemy.getPosition();
            placeGlyph(renderMap, enemyPosition, flyingEnemy.getRenderGlyph());

            if (flyingEnemy.getState() == game::FlyingEnemyState::AttackStartup) {
                const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
                placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
                placeGlyph(renderMap, warningRow, '!');
                placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
            }
        }

        for (size_t index = 0; index < enemyProjectiles.size(); ++index) {
            placeGlyph(renderMap, enemyProjectiles[index].position, '*');
        }

        player.overlayRender(renderMap, gameplayMap);
        mapDrawer.currentmap = player.buildHud() + buildWorldStatus(groundEnemy, flyingEnemy) + renderMap;
        mapDrawer.draw();

        if (player.consumeResetRequest()) {
            resetRuntime(terrainMap, playerPosition, groundEnemy, flyingEnemy, enemyProjectiles, player);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
