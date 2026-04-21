#include <chrono>
#include <cmath>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "input/KeyStateManager.h"
#include "player/Player.h"
#include "save/SaveSystem.h"
#include "world/MapDrawer.h"
#include "world/WorldSystem.h"

namespace {

const int kFrameMs = 16;

struct SandboxNpcInfo {
    std::string id;
    std::string displayName;
    char glyph;
    bool opensShop;
    std::string interactionText;
};

struct DecorationCell {
    game::Position position;
    char glyph;
    bool solid;

    DecorationCell(const game::Position& positionValue = game::Position(),
                   char glyphValue = ' ',
                   bool solidValue = false)
        : position(positionValue),
          glyph(glyphValue),
          solid(solidValue) {
    }
};

struct SandboxState {
    std::string currentMapId;
    std::string currentSpawnPointId;
    std::string lastInteraction;
    std::string prompt;
    bool shopOpen;
    std::unordered_map<int, bool> previousKeys;

    SandboxState()
        : currentMapId("spawn_village"),
          currentSpawnPointId("village_entry"),
          lastInteraction("Village sandbox ready."),
          prompt("Walk around with A/D and SPACE. Press E near NPCs or doors."),
          shopOpen(false) {
    }
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

void placeGlyph(std::string& map, const game::Position& position, char glyph) {
    if (!isInsidePlayableArea(map, position)) {
        return;
    }

    map[indexFromPosition(map, position)] = glyph;
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

std::string buildTerrainMap(const game::MapDefinition& mapDefinition, const std::string& spawnPointId, game::Position& playerPosition) {
    std::string terrainMap = joinTerrainRows(mapDefinition.terrainRows);

    playerPosition = findSpawnPoint(mapDefinition, spawnPointId);
    if (playerPosition.x < 0 || playerPosition.y < 0) {
        playerPosition = findGlyphPosition(terrainMap, '@');
    }

    const game::Position glyphPosition = findGlyphPosition(terrainMap, '@');
    if (glyphPosition.x >= 0 && glyphPosition.y >= 0) {
        placeGlyph(terrainMap, glyphPosition, ' ');
    }

    return terrainMap;
}

bool isAdjacent(const game::Position& a, const game::Position& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1;
}

SandboxNpcInfo getNpcInfo(const std::string& npcId) {
    if (npcId == "village_elder") {
        SandboxNpcInfo npc;
        npc.id = npcId;
        npc.displayName = "Elder";
        npc.glyph = 'L';
        npc.opensShop = false;
        npc.interactionText = "老人主动开口了。（具体对话内容待补）";
        return npc;
    }

    if (npcId == "doctor") {
        SandboxNpcInfo npc;
        npc.id = npcId;
        npc.displayName = "Doctor";
        npc.glyph = 'D';
        npc.opensShop = false;
        npc.interactionText = "医生看了你一眼，准备接诊。（具体对话内容待补）";
        return npc;
    }

    SandboxNpcInfo npc;
    npc.id = npcId;
    npc.displayName = "Merchant";
    npc.glyph = 'M';
    npc.opensShop = true;
    npc.interactionText = "商人已经开门营业了。商店内容先用占位面板代替。";
    return npc;
}

std::vector<DecorationCell> buildDecorations(const game::MapDefinition& mapDefinition) {
    std::vector<DecorationCell> decorations;

    if (mapDefinition.id == "spawn_village") {
        for (int x = 48; x <= 60; ++x) {
            decorations.push_back(DecorationCell(game::Position(x, 4), '^', true));
        }

        for (int y = 5; y <= 8; ++y) {
            decorations.push_back(DecorationCell(game::Position(49, y), '|', true));
            decorations.push_back(DecorationCell(game::Position(60, y), '|', true));
        }

        for (int x = 50; x <= 59; ++x) {
            decorations.push_back(DecorationCell(game::Position(x, 5), '=', true));
        }

        for (int x = 50; x <= 54; ++x) {
            decorations.push_back(DecorationCell(game::Position(x, 8), '=', true));
        }
        for (int x = 56; x <= 59; ++x) {
            decorations.push_back(DecorationCell(game::Position(x, 8), '=', true));
        }

        decorations.push_back(DecorationCell(game::Position(55, 8), 'H', false));
        decorations.push_back(DecorationCell(game::Position(52, 6), 'o', false));
        decorations.push_back(DecorationCell(game::Position(57, 6), 'o', false));
    } else if (mapDefinition.id == "village_house_interior") {
        decorations.push_back(DecorationCell(game::Position(6, 8), 'H', false));
        decorations.push_back(DecorationCell(game::Position(18, 7), '=', true));
        decorations.push_back(DecorationCell(game::Position(19, 7), '=', true));
        decorations.push_back(DecorationCell(game::Position(20, 7), '=', true));
    }

    return decorations;
}

std::string buildCollisionMap(const std::string& terrainMap,
                              const game::Position& playerPosition,
                              const game::MapDefinition& mapDefinition) {
    std::string collisionMap = terrainMap;

    const std::vector<DecorationCell> decorations = buildDecorations(mapDefinition);
    for (size_t index = 0; index < decorations.size(); ++index) {
        if (decorations[index].solid || decorations[index].glyph == 'H') {
            placeGlyph(collisionMap, decorations[index].position, decorations[index].glyph);
        }
    }

    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        placeGlyph(collisionMap,
                   mapDefinition.npcPlacements[index].position,
                   getNpcInfo(mapDefinition.npcPlacements[index].npcId).glyph);
    }

    placeGlyph(collisionMap, playerPosition, '@');
    return collisionMap;
}

std::string buildRenderMap(const std::string& terrainMap,
                           const game::Position& playerPosition,
                           const game::MapDefinition& mapDefinition) {
    std::string renderMap = terrainMap;

    const std::vector<DecorationCell> decorations = buildDecorations(mapDefinition);
    for (size_t index = 0; index < decorations.size(); ++index) {
        placeGlyph(renderMap, decorations[index].position, decorations[index].glyph);
    }

    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        placeGlyph(renderMap,
                   mapDefinition.npcPlacements[index].position,
                   getNpcInfo(mapDefinition.npcPlacements[index].npcId).glyph);
    }

    placeGlyph(renderMap, playerPosition, '@');
    return renderMap;
}

std::string buildHud(const game::MapDefinition& mapDefinition,
                     const game::SaveData& saveData,
                     const game::Position& playerPosition,
                     const std::vector<game::MapDefinition>& loadedMaps,
                     const SandboxState& state) {
    std::ostringstream hud;
    hud << "[WorldSandbox] ESC exit | R reset\n";
    hud << "Map " << mapDefinition.displayName
        << " | Loaded Maps " << loadedMaps.size()
        << " | Spawn Points " << mapDefinition.spawnPoints.size()
        << " | Transitions " << mapDefinition.transitions.size()
        << " | NPCs " << mapDefinition.npcPlacements.size() << "\n";
    hud << "Player Pos (" << playerPosition.x << ", " << playerPosition.y << ")"
        << " | Save Map " << saveData.currentMapId
        << " | Respawn " << saveData.respawnMapId << "\n";
    hud << "Move A/D | Jump SPACE | Interact E | No enemies in this sandbox.\n";
    hud << "Prompt: " << state.prompt << "\n";
    hud << "Last Interaction: " << state.lastInteraction << "\n";
    if (state.shopOpen) {
        hud << "[Merchant Shop Placeholder]\n";
        hud << "- 货物列表待补\n";
        hud << "- 价格待补\n";
        hud << "- 按 E 再次关闭商店\n";
    }
    hud << "\n";
    return hud.str();
}

void loadMapState(const game::MapDefinition& mapDefinition,
                  const std::string& spawnPointId,
                  SandboxState& state,
                  game::SaveData& saveData,
                  std::string& terrainMap,
                  game::Position& playerPosition) {
    terrainMap = buildTerrainMap(mapDefinition, spawnPointId, playerPosition);
    saveData.currentMapId = mapDefinition.id;
    saveData.respawnMapId = spawnPointId;
    saveData.hasActiveRun = true;
    state.currentMapId = mapDefinition.id;
    state.currentSpawnPointId = spawnPointId;
    state.shopOpen = false;
}

void resetWorldSandbox(const game::MapDefinition& mapDefinition,
                       const std::string& spawnPointId,
                       SandboxState& state,
                       game::SaveData& saveData,
                       std::string& terrainMap,
                       game::Position& playerPosition) {
    state.lastInteraction = "Village sandbox ready.";
    state.prompt = "Walk around with A/D and SPACE. Press E near NPCs or doors.";
    loadMapState(mapDefinition, spawnPointId, state, saveData, terrainMap, playerPosition);
}

const game::NpcPlacement* findInteractableNpc(const game::MapDefinition& mapDefinition, const game::Position& playerPosition) {
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

bool isNpcBlockingDoorway(const game::MapDefinition& mapDefinition, const game::Position& playerPosition) {
    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        if (mapDefinition.npcPlacements[index].position.x == playerPosition.x &&
            mapDefinition.npcPlacements[index].position.y == playerPosition.y) {
            return true;
        }
    }
    return false;
}

void updatePrompt(const game::MapDefinition& mapDefinition,
                  const game::Position& playerPosition,
                  SandboxState& state) {
    const game::NpcPlacement* npcPlacement = findInteractableNpc(mapDefinition, playerPosition);
    if (npcPlacement != 0) {
        const SandboxNpcInfo npcInfo = getNpcInfo(npcPlacement->npcId);
        state.prompt = "Press E to talk to " + npcInfo.displayName + ".";
        return;
    }

    const game::MapTransition* transition = findInteractableTransition(mapDefinition, playerPosition);
    if (transition != 0) {
        if (mapDefinition.id == "spawn_village") {
            state.prompt = "Press E to enter the house.";
        } else {
            state.prompt = "Press E to leave the house.";
        }
        return;
    }

    state.prompt = "Walk around with A/D and SPACE. Press E near NPCs or doors.";
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    SandboxState state;
    game::MapLoader mapLoader;
    const std::vector<game::MapDefinition> maps = mapLoader.loadAllMaps();
    game::MapDefinition currentMapDefinition = mapLoader.loadMap("spawn_village");
    const std::string spawnPointId = currentMapDefinition.spawnPoints.empty()
            ? std::string()
            : currentMapDefinition.spawnPoints.front().id;

    game::SaveData saveData;
    std::string terrainMap;
    game::Position playerPosition;

    resetWorldSandbox(currentMapDefinition, spawnPointId, state, saveData, terrainMap, playerPosition);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        if (isJustPressed(keyStateManager, state, 'r') || isJustPressed(keyStateManager, state, 'R')) {
            currentMapDefinition = mapLoader.loadMap("spawn_village");
            resetWorldSandbox(currentMapDefinition, spawnPointId, state, saveData, terrainMap, playerPosition);
        }

        updatePrompt(currentMapDefinition, playerPosition, state);

        if (isJustPressed(keyStateManager, state, 'e') || isJustPressed(keyStateManager, state, 'E')) {
            const game::NpcPlacement* npcPlacement = findInteractableNpc(currentMapDefinition, playerPosition);
            if (npcPlacement != 0) {
                const SandboxNpcInfo npcInfo = getNpcInfo(npcPlacement->npcId);
                state.lastInteraction = npcInfo.displayName + ": " + npcInfo.interactionText;
                state.shopOpen = npcInfo.opensShop ? !state.shopOpen : false;
            } else {
                const game::MapTransition* transition = findInteractableTransition(currentMapDefinition, playerPosition);
                if (transition != 0) {
                    currentMapDefinition = mapLoader.loadMap(transition->toMapId);
                    loadMapState(currentMapDefinition,
                                 transition->spawnPointId,
                                 state,
                                 saveData,
                                 terrainMap,
                                 playerPosition);
                    state.lastInteraction = currentMapDefinition.id == "village_house_interior"
                            ? "You entered the house."
                            : "You walked back outside.";
                }
            }
        }

        if (!state.shopOpen) {
            std::string collisionMap = buildCollisionMap(terrainMap, playerPosition, currentMapDefinition);
            player.move(collisionMap);

            const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');
            if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0 &&
                !isNpcBlockingDoorway(currentMapDefinition, movedPlayerPosition)) {
                playerPosition = movedPlayerPosition;
            }
        }

        mapDrawer.currentmap = buildHud(currentMapDefinition, saveData, playerPosition, maps, state) +
                               buildRenderMap(terrainMap, playerPosition, currentMapDefinition);
        mapDrawer.draw();

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
