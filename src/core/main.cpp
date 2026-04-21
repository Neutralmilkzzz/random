#include <chrono>
#include <cmath>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/GameSession.h"
#include "enemy/Enemy.h"
#include "input/KeyStateManager.h"
#include "npc/NpcSystem.h"
#include "player/Player.h"
#include "save/SaveSystem.h"
#include "world/MapDrawer.h"
#include "world/WorldSystem.h"

namespace {

const int kFrameMs = 16;
const int kTitleScreenWidth = 76;
const int kTitleScreenHeight = 24;
const int kTitleUnavailableHintFrames = 90;
const int kTitleStartTransitionFrames = 24;
const int kFlyingProjectileStepFrames = 3;
const char* kDefaultRuntimePrompt = "Move with A/D and SPACE. Press E near doors or NPCs.";

struct EnemyProjectile {
    game::Position position;
    int dx;
    int dy;
    int stepCooldown;
    int remainingFrames;

    EnemyProjectile()
        : dx(0),
          dy(0),
          stepCooldown(0),
          remainingFrames(0) {
    }
};

struct RuntimeNpcInfo {
    std::string displayName;
    char glyph;
    bool opensShop;
    std::string interactionText;
};

struct RuntimeState {
    std::string currentMapId;
    std::string currentSpawnPointId;
    std::string lastInteraction;
    std::string prompt;
    bool shopOpen;
    std::string shopNpcId;
    std::vector<game::ShopOffer> shopOffers;
    int shopSelection;
    std::unordered_map<int, bool> previousKeys;

    RuntimeState()
        : currentMapId("spawn_village"),
          currentSpawnPointId("player_start"),
          lastInteraction("Main world ready."),
          prompt(kDefaultRuntimePrompt),
          shopOpen(false),
          shopSelection(0) {
    }
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

enum class TitleScreenResult {
    Exit,
    NewGame,
    ContinueGame
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

bool isAdjacent(const game::Position& a, const game::Position& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1;
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

    for (size_t index = 0; index < text.size() &&
                           start + static_cast<int>(index) < static_cast<int>(lines[row].size()) - 1;
         ++index) {
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
                                int animationFrame,
                                bool hasSaveData) {
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
    const std::string continueLabel = hasSaveData ? "CONTINUE" : "CONTINUE (NO SAVE)";
    const std::string continueLine = selection == TitleMenuSelection::ContinueGame
            ? (((animationFrame / 20) % 2 == 0) ? "> " + continueLabel : ">> " + continueLabel)
            : "  " + continueLabel;

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

TitleScreenResult runTitleScreen(MapDrawer& mapDrawer,
                                 KeyStateManager& keyStateManager,
                                 bool hasSaveData) {
    TitleMenuSelection selection = TitleMenuSelection::NewGame;
    TitleScreenState state = TitleScreenState::TitleScreen;
    int stateFrames = 0;
    int animationFrame = 0;
    std::unordered_map<int, bool> previousKeys;

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            return TitleScreenResult::Exit;
        }

        if (state == TitleScreenState::StartTransition) {
            stateFrames++;
            mapDrawer.currentmap = buildTitleScreenMap(selection, state, animationFrame++, hasSaveData);
            mapDrawer.draw();
            previousKeys = keyStateManager.keyStates;
            if (stateFrames >= kTitleStartTransitionFrames) {
                return selection == TitleMenuSelection::NewGame
                        ? TitleScreenResult::NewGame
                        : TitleScreenResult::ContinueGame;
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

        mapDrawer.currentmap = buildTitleScreenMap(selection, state, animationFrame++, hasSaveData);
        mapDrawer.draw();
        previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }
}

std::string joinTerrainRows(const std::vector<std::string>& rows) {
    std::ostringstream stream;
    for (size_t index = 0; index < rows.size(); ++index) {
        stream << rows[index];
        if (index + 1 < rows.size()) {
            stream << '\n';
        }
    }
    return stream.str();
}

game::Position findSpawnPoint(const game::MapDefinition& mapDefinition, const std::string& spawnPointId) {
    for (size_t index = 0; index < mapDefinition.spawnPoints.size(); ++index) {
        if (mapDefinition.spawnPoints[index].id == spawnPointId) {
            return mapDefinition.spawnPoints[index].position;
        }
    }

    if (!mapDefinition.spawnPoints.empty()) {
        return mapDefinition.spawnPoints.front().position;
    }

    return game::Position(-1, -1);
}

bool isGroundEnemyTemplate(const std::string& enemyTemplateId) {
    return enemyTemplateId.find("flying") == std::string::npos;
}

RuntimeNpcInfo getNpcInfo(const std::string& npcId) {
    if (npcId == "village_elder" || npcId == "village_chief") {
        RuntimeNpcInfo npc;
        npc.displayName = "Chief";
        npc.glyph = 'L';
        npc.opensShop = false;
        npc.interactionText = "Head right and clear the next rooms. The upper shortcut opens later.";
        return npc;
    }

    if (npcId == "doctor") {
        RuntimeNpcInfo npc;
        npc.displayName = "Doctor";
        npc.glyph = 'D';
        npc.opensShop = false;
        npc.interactionText = "Doctor hook is ready. Full-heal behavior will be wired here.";
        return npc;
    }

    if (npcId == "merchant" || npcId == "merchant_shop") {
        RuntimeNpcInfo npc;
        npc.displayName = "Merchant";
        npc.glyph = 'M';
        npc.opensShop = false;
        npc.interactionText = "Merchant hook is ready. Shop UI is not wired into main yet.";
        return npc;
    }

    if (npcId == "event_marker") {
        RuntimeNpcInfo npc;
        npc.displayName = "Event Marker";
        npc.glyph = '?';
        npc.opensShop = false;
        npc.interactionText = "Event hook placeholder.";
        return npc;
    }

    RuntimeNpcInfo npc;
    npc.displayName = "NPC";
    npc.glyph = 'N';
    npc.opensShop = false;
    npc.interactionText = "NPC placeholder.";
    return npc;
}

std::string buildTerrainMap(const game::MapDefinition& mapDefinition,
                            const std::string& spawnPointId,
                            game::Position& playerPosition) {
    std::string terrainMap = joinTerrainRows(mapDefinition.terrainRows);
    playerPosition = findSpawnPoint(mapDefinition, spawnPointId);

    const game::Position glyphPosition = findGlyphPosition(terrainMap, '@');
    if (glyphPosition.x >= 0 && glyphPosition.y >= 0) {
        placeGlyph(terrainMap, glyphPosition, ' ');
    }

    return terrainMap;
}

void spawnEnemiesFromMap(const game::MapDefinition& mapDefinition,
                         std::vector<game::GroundEnemy>& groundEnemies,
                         std::vector<game::FlyingEnemy>& flyingEnemies) {
    groundEnemies.clear();
    flyingEnemies.clear();

    for (size_t index = 0; index < mapDefinition.enemySpawns.size(); ++index) {
        if (isGroundEnemyTemplate(mapDefinition.enemySpawns[index].enemyTemplateId)) {
            groundEnemies.push_back(
                    game::GroundEnemy("ground_enemy_" + std::to_string(index + 1),
                                      mapDefinition.enemySpawns[index].position));
        } else {
            flyingEnemies.push_back(
                    game::FlyingEnemy("flying_enemy_" + std::to_string(index + 1),
                                      mapDefinition.enemySpawns[index].position));
        }
    }
}

void loadMapState(const game::MapDefinition& mapDefinition,
                  const std::string& spawnPointId,
                  RuntimeState& state,
                  game::SaveData& saveData,
                  std::string& terrainMap,
                  game::Position& playerPosition,
                  std::vector<game::GroundEnemy>& groundEnemies,
                  std::vector<game::FlyingEnemy>& flyingEnemies,
                  std::vector<EnemyProjectile>& enemyProjectiles) {
    terrainMap = buildTerrainMap(mapDefinition, spawnPointId, playerPosition);
    spawnEnemiesFromMap(mapDefinition, groundEnemies, flyingEnemies);
    enemyProjectiles.clear();
    saveData.currentMapId = mapDefinition.id;
    saveData.respawnMapId = spawnPointId;
    saveData.hasActiveRun = true;
    state.currentMapId = mapDefinition.id;
    state.currentSpawnPointId = spawnPointId;
    state.shopOpen = false;
    state.shopNpcId.clear();
    state.shopOffers.clear();
    state.shopSelection = 0;
}

void syncSaveDataWithRuntime(const Player& player,
                             const RuntimeState& state,
                             const game::GameSession& session,
                             game::SaveData& saveData) {
    saveData.playerStats = player.getStats();
    saveData.currentMapId = state.currentMapId;
    saveData.respawnMapId = state.currentSpawnPointId;
    saveData.completionTimeSeconds = session.getRunStatistics().totalPlaySeconds;
    saveData.hasActiveRun = true;
}

std::string buildCollisionMap(const std::string& terrainMap,
                              const game::Position& playerPosition,
                              const game::MapDefinition& mapDefinition,
                              const std::vector<game::GroundEnemy>& groundEnemies,
                              const std::vector<game::FlyingEnemy>& flyingEnemies) {
    std::string collisionMap = terrainMap;

    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        placeGlyph(collisionMap,
                   mapDefinition.npcPlacements[index].position,
                   getNpcInfo(mapDefinition.npcPlacements[index].npcId).glyph);
    }

    for (size_t index = 0; index < groundEnemies.size(); ++index) {
        if (groundEnemies[index].isAlive()) {
            placeGlyph(collisionMap, groundEnemies[index].getPosition(), 'g');
        }
    }

    for (size_t index = 0; index < flyingEnemies.size(); ++index) {
        if (flyingEnemies[index].isAlive()) {
            placeGlyph(collisionMap, flyingEnemies[index].getPosition(), 'f');
        }
    }

    placeGlyph(collisionMap, playerPosition, '@');
    return collisionMap;
}

std::string buildRenderMap(const std::string& terrainMap,
                           const game::Position& playerPosition,
                           const game::MapDefinition& mapDefinition,
                           const std::vector<game::GroundEnemy>& groundEnemies,
                           const std::vector<game::FlyingEnemy>& flyingEnemies,
                           const std::vector<EnemyProjectile>& enemyProjectiles) {
    std::string renderMap = terrainMap;

    for (size_t index = 0; index < mapDefinition.transitions.size(); ++index) {
        placeGlyph(renderMap, mapDefinition.transitions[index].triggerPosition, 'D');
    }

    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        placeGlyph(renderMap,
                   mapDefinition.npcPlacements[index].position,
                   getNpcInfo(mapDefinition.npcPlacements[index].npcId).glyph);
    }

    for (size_t index = 0; index < groundEnemies.size(); ++index) {
        if (!groundEnemies[index].isRenderable()) {
            continue;
        }

        const game::Position enemyPosition = groundEnemies[index].getPosition();
        placeGlyph(renderMap, enemyPosition, groundEnemies[index].getRenderGlyph());

        if (groundEnemies[index].getState() == game::GroundEnemyState::AttackStartup) {
            const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
            placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
            placeGlyph(renderMap, warningRow, '!');
            placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
        }
    }

    for (size_t index = 0; index < flyingEnemies.size(); ++index) {
        if (!flyingEnemies[index].isRenderable()) {
            continue;
        }

        const game::Position enemyPosition = flyingEnemies[index].getPosition();
        placeGlyph(renderMap, enemyPosition, flyingEnemies[index].getRenderGlyph());

        if (flyingEnemies[index].getState() == game::FlyingEnemyState::AttackStartup) {
            const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
            placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
            placeGlyph(renderMap, warningRow, '!');
            placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
        }
    }

    for (size_t index = 0; index < enemyProjectiles.size(); ++index) {
        placeGlyph(renderMap, enemyProjectiles[index].position, '*');
    }

    placeGlyph(renderMap, playerPosition, '@');
    return renderMap;
}

const game::NpcPlacement* findInteractableNpc(const game::MapDefinition& mapDefinition,
                                              const game::Position& playerPosition) {
    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        if (isAdjacent(playerPosition, mapDefinition.npcPlacements[index].position)) {
            return &mapDefinition.npcPlacements[index];
        }
    }
    return 0;
}

const game::MapTransition* findInteractableTransition(const game::MapDefinition& mapDefinition,
                                                      const game::Position& playerPosition) {
    for (size_t index = 0; index < mapDefinition.transitions.size(); ++index) {
        if (isAdjacent(playerPosition, mapDefinition.transitions[index].triggerPosition)) {
            return &mapDefinition.transitions[index];
        }
    }
    return 0;
}

void updatePrompt(const game::MapDefinition& mapDefinition,
                  const game::Position& playerPosition,
                  RuntimeState& state) {
    if (state.shopOpen) {
        state.prompt = "Shop open. W/S select, J buy, E close.";
        return;
    }

    const game::NpcPlacement* npcPlacement = findInteractableNpc(mapDefinition, playerPosition);
    if (npcPlacement != 0) {
        const RuntimeNpcInfo npcInfo = getNpcInfo(npcPlacement->npcId);
        state.prompt = "Press E to interact with " + npcInfo.displayName + ".";
        return;
    }

    const game::MapTransition* transition = findInteractableTransition(mapDefinition, playerPosition);
    if (transition != 0) {
        state.prompt = "Press E to use door.";
        return;
    }

    state.prompt = kDefaultRuntimePrompt;
}

std::string buildBottomFooter(const RuntimeState& state) {
    std::ostringstream footer;
    footer << "\n";
    footer << "A/D Move  SPACE Jump  W/S Aim  J Attack  K Spell  R Heal  P Reset  ESC Exit";
    if (state.prompt != kDefaultRuntimePrompt) {
        footer << "\n" << state.prompt;
    }
    if (!state.lastInteraction.empty()) {
        footer << "\n" << state.lastInteraction;
    }
    if (state.shopOpen) {
        for (size_t index = 0; index < state.shopOffers.size(); ++index) {
            footer << "\n"
                   << (static_cast<int>(index) == state.shopSelection ? "> " : "  ")
                   << state.shopOffers[index].name
                   << " - " << state.shopOffers[index].hkdCost << " HKD";
        }
        if (state.shopSelection >= 0 &&
            state.shopSelection < static_cast<int>(state.shopOffers.size())) {
            footer << "\n"
                   << state.shopOffers[static_cast<size_t>(state.shopSelection)].description;
        }
    }
    return footer.str();
}

void applyNpcInteraction(const std::string& npcId,
                         Player& player,
                         RuntimeState& state,
                         game::SaveData& saveData) {
    game::CharacterStats stats = player.getStats();
    const game::NpcInteractionResult result = game::interactWithNpc(npcId, stats);
    player.restoreSavedStats(stats);
    saveData.playerStats = stats;

    state.lastInteraction = result.speaker + ": " + result.text;
    state.shopOpen = result.opensShop;
    state.shopNpcId = result.opensShop ? npcId : std::string();
    state.shopOffers = result.offers;
    state.shopSelection = 0;
}

void applyShopPurchase(Player& player,
                       RuntimeState& state,
                       game::SaveData& saveData) {
    if (!state.shopOpen ||
        state.shopSelection < 0 ||
        state.shopSelection >= static_cast<int>(state.shopOffers.size())) {
        return;
    }

    game::CharacterStats stats = player.getStats();
    const game::ShopOffer& selectedOffer = state.shopOffers[static_cast<size_t>(state.shopSelection)];
    const game::NpcInteractionResult result =
            game::purchaseNpcOffer(state.shopNpcId, selectedOffer.id, stats);
    player.restoreSavedStats(stats);
    saveData.playerStats = stats;
    state.lastInteraction = result.speaker + ": " + result.text;
}

bool canEnemyOccupy(const std::string& terrainMap,
                    const game::Position& playerPosition,
                    const std::vector<game::GroundEnemy>& groundEnemies,
                    const std::vector<game::FlyingEnemy>& flyingEnemies,
                    const game::Position& targetPosition,
                    bool checkingGround,
                    int ignoreIndex) {
    if (!isInsidePlayableArea(terrainMap, targetPosition)) {
        return false;
    }

    if (tileAt(terrainMap, targetPosition) != ' ') {
        return false;
    }

    if (targetPosition.x == playerPosition.x && targetPosition.y == playerPosition.y) {
        return false;
    }

    for (size_t index = 0; index < groundEnemies.size(); ++index) {
        if (checkingGround && static_cast<int>(index) == ignoreIndex) {
            continue;
        }
        if (!groundEnemies[index].isAlive()) {
            continue;
        }

        const game::Position enemyPosition = groundEnemies[index].getPosition();
        if (enemyPosition.x == targetPosition.x && enemyPosition.y == targetPosition.y) {
            return false;
        }
    }

    for (size_t index = 0; index < flyingEnemies.size(); ++index) {
        if (!checkingGround && static_cast<int>(index) == ignoreIndex) {
            continue;
        }
        if (!flyingEnemies[index].isAlive()) {
            continue;
        }

        const game::Position enemyPosition = flyingEnemies[index].getPosition();
        if (enemyPosition.x == targetPosition.x && enemyPosition.y == targetPosition.y) {
            return false;
        }
    }

    return true;
}

int findClosestGroundEnemyIndex(const std::vector<game::GroundEnemy>& groundEnemies,
                                const game::Position& playerPosition) {
    int bestIndex = -1;
    int bestDistance = 1000000;

    for (size_t index = 0; index < groundEnemies.size(); ++index) {
        if (!groundEnemies[index].isAlive()) {
            continue;
        }

        const game::Position enemyPosition = groundEnemies[index].getPosition();
        const int distance = std::abs(enemyPosition.x - playerPosition.x) +
                             std::abs(enemyPosition.y - playerPosition.y);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }

    return bestIndex;
}

int findClosestFlyingEnemyIndex(const std::vector<game::FlyingEnemy>& flyingEnemies,
                                const game::Position& playerPosition) {
    int bestIndex = -1;
    int bestDistance = 1000000;

    for (size_t index = 0; index < flyingEnemies.size(); ++index) {
        if (!flyingEnemies[index].isAlive()) {
            continue;
        }

        const game::Position enemyPosition = flyingEnemies[index].getPosition();
        const int distance = std::abs(enemyPosition.x - playerPosition.x) +
                             std::abs(enemyPosition.y - playerPosition.y);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }

    return bestIndex;
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

void removeDefeatedEnemies(std::vector<game::GroundEnemy>& groundEnemies,
                           std::vector<game::FlyingEnemy>& flyingEnemies) {
    std::vector<game::GroundEnemy> livingGround;
    for (size_t index = 0; index < groundEnemies.size(); ++index) {
        if (!groundEnemies[index].shouldDespawn()) {
            livingGround.push_back(groundEnemies[index]);
        }
    }
    groundEnemies.swap(livingGround);

    std::vector<game::FlyingEnemy> livingFlying;
    for (size_t index = 0; index < flyingEnemies.size(); ++index) {
        if (!flyingEnemies[index].shouldDespawn()) {
            livingFlying.push_back(flyingEnemies[index]);
        }
    }
    flyingEnemies.swap(livingFlying);
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    game::GameSession gameSession;
    game::SaveSystem saveSystem;
    game::MapLoader mapLoader;
    RuntimeState state;
    game::SaveData saveData;
    std::string terrainMap;
    game::Position playerPosition;
    std::vector<game::GroundEnemy> groundEnemies;
    std::vector<game::FlyingEnemy> flyingEnemies;
    std::vector<EnemyProjectile> enemyProjectiles;
    game::GroundEnemy dummyGroundEnemy("dummy_ground", game::Position(-100, -100));
    game::FlyingEnemy dummyFlyingEnemy("dummy_flying", game::Position(-100, -100));
    game::MapDefinition currentMapDefinition;

    const TitleScreenResult titleResult = runTitleScreen(mapDrawer, keyStateManager, saveSystem.hasSave());
    if (titleResult == TitleScreenResult::Exit) {
        return 0;
    }

    if (titleResult == TitleScreenResult::ContinueGame) {
        saveData = saveSystem.load();
        if (!gameSession.continueRun(saveData)) {
            return 0;
        }
    } else {
        if (!gameSession.startNewRun(game::Difficulty::Normal)) {
            return 0;
        }
        saveData = gameSession.getActiveSave();
        saveSystem.saveOnMapEntry(saveData);
    }

    player.restoreSavedStats(saveData.playerStats);
    currentMapDefinition = mapLoader.loadMap(saveData.currentMapId.empty() ? "spawn_village" : saveData.currentMapId);
    loadMapState(currentMapDefinition,
                 saveData.respawnMapId.empty() ? "player_start" : saveData.respawnMapId,
                 state,
                 saveData,
                 terrainMap,
                 playerPosition,
                 groundEnemies,
                 flyingEnemies,
                 enemyProjectiles);
    syncSaveDataWithRuntime(player, state, gameSession, saveData);
    gameSession.setActiveSave(saveData);
    saveSystem.saveOnMapEntry(saveData);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        if (isJustPressed(keyStateManager, state.previousKeys, 'p') ||
            isJustPressed(keyStateManager, state.previousKeys, 'P')) {
            player.restoreSavedStats(saveData.playerStats);
            currentMapDefinition = mapLoader.loadMap(saveData.currentMapId);
            loadMapState(currentMapDefinition,
                         saveData.respawnMapId,
                         state,
                         saveData,
                         terrainMap,
                         playerPosition,
                         groundEnemies,
                         flyingEnemies,
                         enemyProjectiles);
            state.lastInteraction = "Room reset to current save.";
        }

        updatePrompt(currentMapDefinition, playerPosition, state);

        if (state.shopOpen && !state.shopOffers.empty()) {
            if (isJustPressed(keyStateManager, state.previousKeys, 'w') ||
                isJustPressed(keyStateManager, state.previousKeys, 'W')) {
                state.shopSelection--;
                if (state.shopSelection < 0) {
                    state.shopSelection = static_cast<int>(state.shopOffers.size()) - 1;
                }
            } else if (isJustPressed(keyStateManager, state.previousKeys, 's') ||
                       isJustPressed(keyStateManager, state.previousKeys, 'S')) {
                state.shopSelection++;
                if (state.shopSelection >= static_cast<int>(state.shopOffers.size())) {
                    state.shopSelection = 0;
                }
            }
        }

        if (state.shopOpen &&
            (isJustPressed(keyStateManager, state.previousKeys, 'j') ||
             isJustPressed(keyStateManager, state.previousKeys, 'J'))) {
            applyShopPurchase(player, state, saveData);
            syncSaveDataWithRuntime(player, state, gameSession, saveData);
            gameSession.setActiveSave(saveData);
            saveSystem.saveOnMapEntry(saveData);
        }

        if (isJustPressed(keyStateManager, state.previousKeys, 'e') ||
            isJustPressed(keyStateManager, state.previousKeys, 'E')) {
            if (state.shopOpen) {
                state.shopOpen = false;
                state.shopNpcId.clear();
                state.shopOffers.clear();
                state.shopSelection = 0;
                state.lastInteraction = "Closed interaction panel.";
            } else {
                const game::NpcPlacement* npcPlacement = findInteractableNpc(currentMapDefinition, playerPosition);
                if (npcPlacement != 0) {
                    applyNpcInteraction(npcPlacement->npcId, player, state, saveData);
                    syncSaveDataWithRuntime(player, state, gameSession, saveData);
                    gameSession.setActiveSave(saveData);
                    saveSystem.saveOnMapEntry(saveData);
                } else {
                    const game::MapTransition* transition = findInteractableTransition(currentMapDefinition, playerPosition);
                    if (transition != 0) {
                        currentMapDefinition = mapLoader.loadMap(transition->toMapId);
                        loadMapState(currentMapDefinition,
                                     transition->spawnPointId,
                                     state,
                                     saveData,
                                     terrainMap,
                                     playerPosition,
                                     groundEnemies,
                                     flyingEnemies,
                                     enemyProjectiles);
                        syncSaveDataWithRuntime(player, state, gameSession, saveData);
                        gameSession.setActiveSave(saveData);
                        saveSystem.saveOnMapEntry(saveData);
                        state.lastInteraction = "Entered " + currentMapDefinition.displayName + ".";
                    }
                }
            }
        }

        if (!state.shopOpen) {
            std::string collisionMap = buildCollisionMap(terrainMap,
                                                         playerPosition,
                                                         currentMapDefinition,
                                                         groundEnemies,
                                                         flyingEnemies);
            if (player.isAlive() && !player.isMovementLocked()) {
                player.move(collisionMap);
                const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');
                if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0) {
                    playerPosition = movedPlayerPosition;
                }
            }
        }

        std::string gameplayMap = terrainMap;
        placeGlyph(gameplayMap, playerPosition, '@');

        const int activeGroundEnemyIndex = findClosestGroundEnemyIndex(groundEnemies, playerPosition);
        const int activeFlyingEnemyIndex = findClosestFlyingEnemyIndex(flyingEnemies, playerPosition);

        if (activeGroundEnemyIndex >= 0 && activeFlyingEnemyIndex >= 0) {
            player.updateCombat(gameplayMap,
                                groundEnemies[static_cast<size_t>(activeGroundEnemyIndex)],
                                flyingEnemies[static_cast<size_t>(activeFlyingEnemyIndex)]);
        } else if (activeGroundEnemyIndex >= 0) {
            player.updateCombat(gameplayMap,
                                groundEnemies[static_cast<size_t>(activeGroundEnemyIndex)],
                                dummyFlyingEnemy);
        } else if (activeFlyingEnemyIndex >= 0) {
            player.updateCombat(gameplayMap,
                                dummyGroundEnemy,
                                flyingEnemies[static_cast<size_t>(activeFlyingEnemyIndex)]);
        } else {
            player.updateCombat(gameplayMap, dummyGroundEnemy, dummyFlyingEnemy);
        }

        for (size_t index = 0; index < groundEnemies.size(); ++index) {
            const game::Position previousPosition = groundEnemies[index].getPosition();
            groundEnemies[index].updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
            const game::Position nextPosition = groundEnemies[index].getPosition();

            if (groundEnemies[index].isAlive() &&
                (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                !canEnemyOccupy(terrainMap,
                                playerPosition,
                                groundEnemies,
                                flyingEnemies,
                                nextPosition,
                                true,
                                static_cast<int>(index))) {
                groundEnemies[index].setPosition(previousPosition);
            }

            if (player.isAlive() && groundEnemies[index].consumeAttackTrigger()) {
                if (groundEnemies[index].isTouchingPlayer(playerPosition) ||
                    groundEnemies[index].isInAttackRange(playerPosition)) {
                    player.receiveDamage("Ground enemy", playerPosition);
                }
            }
        }

        for (size_t index = 0; index < flyingEnemies.size(); ++index) {
            const game::Position previousPosition = flyingEnemies[index].getPosition();
            flyingEnemies[index].updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
            const game::Position nextPosition = flyingEnemies[index].getPosition();

            if (flyingEnemies[index].isAlive() &&
                (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                !canEnemyOccupy(terrainMap,
                                playerPosition,
                                groundEnemies,
                                flyingEnemies,
                                nextPosition,
                                false,
                                static_cast<int>(index))) {
                flyingEnemies[index].setPosition(previousPosition);
            }

            if (flyingEnemies[index].consumeProjectileTrigger()) {
                spawnFlyingProjectile(flyingEnemies[index], playerPosition, enemyProjectiles);
            }
        }

        updateEnemyProjectiles(terrainMap, playerPosition, enemyProjectiles, player);
        removeDefeatedEnemies(groundEnemies, flyingEnemies);
        gameSession.update(static_cast<float>(kFrameMs) / 1000.0f);
        syncSaveDataWithRuntime(player, state, gameSession, saveData);
        gameSession.setActiveSave(saveData);

        std::string renderMap = buildRenderMap(terrainMap,
                                               playerPosition,
                                               currentMapDefinition,
                                               groundEnemies,
                                               flyingEnemies,
                                               enemyProjectiles);
        player.overlayRender(renderMap, gameplayMap);
        mapDrawer.currentmap = player.buildHud(currentMapDefinition.displayName) +
                               renderMap +
                               buildBottomFooter(state);
        mapDrawer.draw();

        if (player.consumeResetRequest()) {
            gameSession.markPlayerDead();
            saveData = saveSystem.restoreAfterDeath(gameSession.getActiveSave());
            gameSession.setActiveSave(saveData);
            player.restoreSavedStats(saveData.playerStats);
            currentMapDefinition = mapLoader.loadMap(saveData.currentMapId);
            loadMapState(currentMapDefinition,
                         saveData.respawnMapId,
                         state,
                         saveData,
                         terrainMap,
                         playerPosition,
                         groundEnemies,
                         flyingEnemies,
                         enemyProjectiles);
            state.lastInteraction = "Respawned at current save point.";
        }

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
