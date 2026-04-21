#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "enemy/Enemy.h"
#include "input/KeyStateManager.h"
#include "player/Player.h"
#include "world/MapDrawer.h"
#include "world/WorldSystem.h"

namespace {

const int kFrameMs = 16;
const int kDefaultMapWidth = 76;
const int kDefaultMapHeight = 10;
const char* kDefaultEditorMapId = "editor_map";
const char* kMapRepositoryRoot = "data/maps/";
const char* kMapIndexPath = "data/maps/index.txt";
const char* kGroundEnemyTemplateId = "ground_enemy_basic";
const char* kFlyingEnemyTemplateId = "flying_enemy_basic";
const int kFlyingProjectileStepFrames = 3;

enum class BrushMode {
    Platform,
    GroundEnemy,
    FlyingEnemy,
    Spawn,
    Erase
};

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

struct EditorState {
    struct PendingDoorLink {
        bool active;
        std::string sourceMapId;
        game::Position sourcePosition;

        PendingDoorLink()
            : active(false),
              sourcePosition(-1, -1) {
        }
    };

    std::vector<std::string> mapIds;
    size_t currentMapIndex;
    game::MapDefinition mapDefinition;
    game::Position cursor;
    BrushMode brushMode;
    std::string lastAction;
    std::unordered_map<int, bool> previousKeys;
    PendingDoorLink pendingDoorLink;

    EditorState()
        : currentMapIndex(0),
          cursor(3, 3),
          brushMode(BrushMode::Platform),
          lastAction("Map engine ready. Use I/J/K/L to move, E to paint.") {
    }
};

struct DemoState {
    bool active;
    std::string terrainMap;
    game::Position playerPosition;
    std::vector<game::GroundEnemy> groundEnemies;
    std::vector<game::FlyingEnemy> flyingEnemies;
    std::vector<EnemyProjectile> enemyProjectiles;
    game::GroundEnemy dummyGroundEnemy;
    game::FlyingEnemy dummyFlyingEnemy;

    DemoState()
        : active(false),
          playerPosition(-1, -1),
          dummyGroundEnemy("editor_dummy_ground", game::Position(-100, -100)),
          dummyFlyingEnemy("editor_dummy_flying", game::Position(-100, -100)) {
        dummyGroundEnemy.setPosition(game::Position(-100, -100));
        dummyGroundEnemy.setSpawnPosition(game::Position(-100, -100));
        dummyFlyingEnemy.setPosition(game::Position(-100, -100));
        dummyFlyingEnemy.setSpawnPosition(game::Position(-100, -100));
    }
};

bool isKeyDown(const KeyStateManager& keyStateManager, int keyCode) {
    const std::unordered_map<int, bool>::const_iterator it = keyStateManager.keyStates.find(keyCode);
    return it != keyStateManager.keyStates.end() && it->second;
}

bool wasKeyDown(const EditorState& state, int keyCode) {
    const std::unordered_map<int, bool>::const_iterator it = state.previousKeys.find(keyCode);
    return it != state.previousKeys.end() && it->second;
}

bool isJustPressed(const KeyStateManager& keyStateManager, const EditorState& state, int keyCode) {
    return isKeyDown(keyStateManager, keyCode) && !wasKeyDown(state, keyCode);
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream input(path.c_str());
    std::string line;

    if (!input.is_open()) {
        return lines;
    }

    while (std::getline(input, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        lines.push_back(line);
    }

    return lines;
}

std::string mapPathForId(const std::string& mapId) {
    return std::string(kMapRepositoryRoot) + mapId + ".map";
}

std::vector<std::string> loadMapIds() {
    std::vector<std::string> result;
    const std::vector<std::string> lines = readLines(kMapIndexPath);
    for (size_t index = 0; index < lines.size(); ++index) {
        if (lines[index].empty() || lines[index][0] == '#') {
            continue;
        }
        result.push_back(lines[index]);
    }

    if (result.empty()) {
        result.push_back(kDefaultEditorMapId);
    }

    return result;
}

void ensureMapIndexEntry(const std::string& mapId) {
    const std::vector<std::string> lines = readLines(kMapIndexPath);
    if (std::find(lines.begin(), lines.end(), mapId) != lines.end()) {
        return;
    }

    std::ofstream output(kMapIndexPath, std::ios::app);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open map index for writing.");
    }

    if (!lines.empty()) {
        output << "\n";
    }
    output << mapId;
}

std::vector<std::string> createBlankTerrainRows() {
    std::vector<std::string> rows;

    rows.push_back(std::string(kDefaultMapWidth, '='));
    for (int row = 1; row < kDefaultMapHeight - 1; ++row) {
        rows.push_back("=" + std::string(kDefaultMapWidth - 2, ' ') + "=");
    }
    rows.push_back(std::string(kDefaultMapWidth, '='));

    return rows;
}

std::string makeDisplayNameFromId(const std::string& mapId) {
    std::ostringstream stream;
    bool uppercaseNext = true;
    for (size_t index = 0; index < mapId.length(); ++index) {
        const char current = mapId[index];
        if (current == '_' || current == '-') {
            stream << ' ';
            uppercaseNext = true;
            continue;
        }

        if (uppercaseNext && current >= 'a' && current <= 'z') {
            stream << static_cast<char>(current - ('a' - 'A'));
        } else {
            stream << current;
        }
        uppercaseNext = false;
    }
    return stream.str();
}

game::MapDefinition createBlankMapDefinition(const std::string& mapId,
                                             const std::string& displayName = std::string()) {
    game::MapDefinition mapDefinition;
    mapDefinition.id = mapId;
    mapDefinition.displayName = displayName.empty() ? makeDisplayNameFromId(mapId) : displayName;
    mapDefinition.type = game::MapType::CombatRoom;
    mapDefinition.isBossRoom = false;
    mapDefinition.terrainRows = createBlankTerrainRows();
    mapDefinition.spawnPoints.push_back(game::SpawnPoint{"editor_spawn", game::Position(3, kDefaultMapHeight - 2)});
    return mapDefinition;
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

std::string mapTypeToString(game::MapType type) {
    switch (type) {
        case game::MapType::SpawnVillage:
            return "SpawnVillage";
        case game::MapType::CombatRoom:
            return "CombatRoom";
        case game::MapType::BossRoom:
            return "BossRoom";
        case game::MapType::ShopRoom:
            return "ShopRoom";
        case game::MapType::EventRoom:
            return "EventRoom";
        default:
            return "CombatRoom";
    }
}

std::string brushName(BrushMode brushMode) {
    switch (brushMode) {
        case BrushMode::Platform:
            return "Platform";
        case BrushMode::GroundEnemy:
            return "GroundEnemy";
        case BrushMode::FlyingEnemy:
            return "FlyingEnemy";
        case BrushMode::Spawn:
            return "Spawn";
        case BrushMode::Erase:
            return "Erase";
        default:
            return "Unknown";
    }
}

std::string buildDoorSpawnId(const std::string& fromMapId, const game::Position& triggerPosition) {
    std::ostringstream stream;
    stream << "door_from_" << fromMapId << "_" << triggerPosition.x << "_" << triggerPosition.y;
    return stream.str();
}

bool isInsideEditableArea(const EditorState& state, const game::Position& position) {
    if (state.mapDefinition.terrainRows.empty()) {
        return false;
    }

    const int width = static_cast<int>(state.mapDefinition.terrainRows.front().length());
    const int height = static_cast<int>(state.mapDefinition.terrainRows.size());

    return position.x >= 1 &&
           position.y >= 1 &&
           position.x < width - 1 &&
           position.y < height - 1;
}

bool isInsidePlayableArea(const std::string& map, const game::Position& position) {
    const std::string::size_type newlinePos = map.find('\n');
    const int width = newlinePos == std::string::npos ? static_cast<int>(map.length()) : static_cast<int>(newlinePos + 1);
    const int height = width == 0 ? 0 : static_cast<int>((map.length() + 1) / static_cast<size_t>(width));

    return position.x >= 0 &&
           position.y >= 0 &&
           position.x < width - 1 &&
           position.y < height;
}

std::size_t lineWidth(const std::string& map) {
    const std::size_t newlinePos = map.find('\n');
    return newlinePos == std::string::npos ? map.length() : newlinePos + 1;
}

std::size_t indexFromPosition(const std::string& map, const game::Position& position) {
    return static_cast<std::size_t>(position.y) * lineWidth(map) + static_cast<std::size_t>(position.x);
}

void placeGlyph(std::string& map, const game::Position& position, char glyph) {
    if (!isInsidePlayableArea(map, position)) {
        return;
    }

    map[indexFromPosition(map, position)] = glyph;
}

game::Position findGlyphPosition(const std::string& map, char glyph) {
    const std::size_t glyphPos = map.find(glyph);
    const std::size_t width = lineWidth(map);
    if (glyphPos == std::string::npos || width == 0) {
        return game::Position(-1, -1);
    }

    return game::Position(static_cast<int>(glyphPos % width), static_cast<int>(glyphPos / width));
}

char tileAt(const std::string& map, const game::Position& position) {
    if (!isInsidePlayableArea(map, position)) {
        return '#';
    }

    return map[indexFromPosition(map, position)];
}

int findEnemyIndexAt(const EditorState& state, const game::Position& position) {
    for (size_t index = 0; index < state.mapDefinition.enemySpawns.size(); ++index) {
        if (state.mapDefinition.enemySpawns[index].position.x == position.x &&
            state.mapDefinition.enemySpawns[index].position.y == position.y) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

int findTransitionIndexAt(const game::MapDefinition& mapDefinition, const game::Position& position) {
    for (size_t index = 0; index < mapDefinition.transitions.size(); ++index) {
        if (mapDefinition.transitions[index].triggerPosition.x == position.x &&
            mapDefinition.transitions[index].triggerPosition.y == position.y) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

bool isGroundEnemyTemplate(const std::string& enemyTemplateId) {
    return enemyTemplateId.find("flying") == std::string::npos;
}

char enemyGlyphForTemplate(const std::string& enemyTemplateId) {
    return isGroundEnemyTemplate(enemyTemplateId) ? 'g' : 'f';
}

bool isSpawnPointAt(const EditorState& state, const game::Position& position) {
    if (state.mapDefinition.spawnPoints.empty()) {
        return false;
    }

    return state.mapDefinition.spawnPoints.front().position.x == position.x &&
           state.mapDefinition.spawnPoints.front().position.y == position.y;
}

char terrainAt(const EditorState& state, const game::Position& position) {
    return state.mapDefinition.terrainRows[static_cast<size_t>(position.y)][static_cast<size_t>(position.x)];
}

void removeEnemyAt(EditorState& state, const game::Position& position) {
    const int enemyIndex = findEnemyIndexAt(state, position);
    if (enemyIndex >= 0) {
        state.mapDefinition.enemySpawns.erase(state.mapDefinition.enemySpawns.begin() + enemyIndex);
    }
}

void ensureSpawnPoint(EditorState& state) {
    if (state.mapDefinition.spawnPoints.empty()) {
        state.mapDefinition.spawnPoints.push_back(game::SpawnPoint{"editor_spawn", game::Position(3, kDefaultMapHeight - 2)});
    }
}

void clearCell(EditorState& state, const game::Position& position) {
    if (!isInsideEditableArea(state, position)) {
        return;
    }

    state.mapDefinition.terrainRows[static_cast<size_t>(position.y)][static_cast<size_t>(position.x)] = ' ';
    removeEnemyAt(state, position);
    if (isSpawnPointAt(state, position)) {
        ensureSpawnPoint(state);
        state.mapDefinition.spawnPoints.front().position = game::Position(3, kDefaultMapHeight - 2);
    }
}

void applyPlatformBrush(EditorState& state) {
    if (!isInsideEditableArea(state, state.cursor)) {
        state.lastAction = "Cursor is outside editable area.";
        return;
    }

    state.mapDefinition.terrainRows[static_cast<size_t>(state.cursor.y)][static_cast<size_t>(state.cursor.x)] = '=';
    removeEnemyAt(state, state.cursor);
    if (isSpawnPointAt(state, state.cursor)) {
        ensureSpawnPoint(state);
        state.mapDefinition.spawnPoints.front().position = game::Position(3, kDefaultMapHeight - 2);
    }
}

void applyEnemyBrush(EditorState& state, const std::string& enemyTemplateId) {
    if (!isInsideEditableArea(state, state.cursor)) {
        state.lastAction = "Cursor is outside editable area.";
        return;
    }

    if (terrainAt(state, state.cursor) == '=') {
        state.lastAction = "Enemy must be placed on empty space.";
        return;
    }

    if (isSpawnPointAt(state, state.cursor)) {
        state.lastAction = "Enemy cannot share the spawn point.";
        return;
    }

    if (findEnemyIndexAt(state, state.cursor) < 0) {
        state.mapDefinition.enemySpawns.push_back(game::EnemySpawn{enemyTemplateId, state.cursor});
    } else {
        state.mapDefinition.enemySpawns[static_cast<size_t>(findEnemyIndexAt(state, state.cursor))].enemyTemplateId = enemyTemplateId;
    }
}

void applySpawnBrush(EditorState& state) {
    if (!isInsideEditableArea(state, state.cursor)) {
        state.lastAction = "Cursor is outside editable area.";
        return;
    }

    if (terrainAt(state, state.cursor) == '=') {
        state.lastAction = "Spawn point must be on empty space.";
        return;
    }

    removeEnemyAt(state, state.cursor);
    ensureSpawnPoint(state);
    state.mapDefinition.spawnPoints.front().position = state.cursor;
}

void applyEraseBrush(EditorState& state) {
    if (!isInsideEditableArea(state, state.cursor)) {
        state.lastAction = "Cursor is outside editable area.";
        return;
    }

    clearCell(state, state.cursor);
}

void moveCursor(EditorState& state, int dx, int dy) {
    const game::Position next(state.cursor.x + dx, state.cursor.y + dy);
    if (isInsideEditableArea(state, next)) {
        state.cursor = next;
    }
}

void writeMapDefinition(const game::MapDefinition& mapDefinition) {
    std::ofstream output(mapPathForId(mapDefinition.id).c_str());
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open map file for writing: " + mapDefinition.id);
    }

    output << "id=" << mapDefinition.id << "\n";
    output << "display_name=" << mapDefinition.displayName << "\n";
    output << "type=" << mapTypeToString(mapDefinition.type) << "\n";
    output << "boss_room=" << (mapDefinition.isBossRoom ? "true" : "false") << "\n\n";

    output << "[spawn_points]\n";
    for (size_t index = 0; index < mapDefinition.spawnPoints.size(); ++index) {
        output << mapDefinition.spawnPoints[index].id << ","
               << mapDefinition.spawnPoints[index].position.x << ","
               << mapDefinition.spawnPoints[index].position.y << "\n";
    }

    output << "\n[enemy_spawns]\n";
    for (size_t index = 0; index < mapDefinition.enemySpawns.size(); ++index) {
        output << mapDefinition.enemySpawns[index].enemyTemplateId << ","
               << mapDefinition.enemySpawns[index].position.x << ","
               << mapDefinition.enemySpawns[index].position.y << "\n";
    }

    output << "\n[npc_placements]\n";
    for (size_t index = 0; index < mapDefinition.npcPlacements.size(); ++index) {
        output << mapDefinition.npcPlacements[index].npcId << ","
               << mapDefinition.npcPlacements[index].position.x << ","
               << mapDefinition.npcPlacements[index].position.y << "\n";
    }

    output << "\n[transitions]\n";
    for (size_t index = 0; index < mapDefinition.transitions.size(); ++index) {
        output << mapDefinition.transitions[index].fromMapId << ","
               << mapDefinition.transitions[index].toMapId << ","
               << mapDefinition.transitions[index].spawnPointId;
        if (mapDefinition.transitions[index].triggerPosition.x >= 0 &&
            mapDefinition.transitions[index].triggerPosition.y >= 0) {
            output << ","
                   << mapDefinition.transitions[index].triggerPosition.x << ","
                   << mapDefinition.transitions[index].triggerPosition.y;
        }
        output << "\n";
    }

    output << "\n[terrain]\n";
    for (size_t index = 0; index < mapDefinition.terrainRows.size(); ++index) {
        output << mapDefinition.terrainRows[index];
        if (index + 1 < mapDefinition.terrainRows.size()) {
            output << "\n";
        }
    }
}

game::MapDefinition loadOrCreateMapDefinition(const std::string& mapId) {
    game::MapLoader mapLoader;

    try {
        game::MapDefinition mapDefinition = mapLoader.loadMap(mapId);
        if (mapDefinition.terrainRows.empty()) {
            return createBlankMapDefinition(mapId);
        }
        return mapDefinition;
    } catch (...) {
        return createBlankMapDefinition(mapId);
    }
}

void autosave(EditorState& state, const std::string& actionLabel) {
    writeMapDefinition(state.mapDefinition);
    ensureMapIndexEntry(state.mapDefinition.id);
    state.mapIds = loadMapIds();
    const std::vector<std::string>::iterator it =
            std::find(state.mapIds.begin(), state.mapIds.end(), state.mapDefinition.id);
    if (it != state.mapIds.end()) {
        state.currentMapIndex = static_cast<size_t>(it - state.mapIds.begin());
    }
    state.lastAction = actionLabel + " Autosaved " + mapPathForId(state.mapDefinition.id);
}

void saveMapDefinition(game::MapDefinition& mapDefinition) {
    writeMapDefinition(mapDefinition);
    ensureMapIndexEntry(mapDefinition.id);
}

void setBrush(EditorState& state, BrushMode brushMode) {
    state.brushMode = brushMode;
}

void paintAtCursor(EditorState& state) {
    switch (state.brushMode) {
        case BrushMode::Platform:
            applyPlatformBrush(state);
            if (state.lastAction.find("outside editable area") == std::string::npos) {
                autosave(state, "Painted platform.");
            }
            break;
        case BrushMode::GroundEnemy:
            applyEnemyBrush(state, kGroundEnemyTemplateId);
            if (state.lastAction.find("outside editable area") == std::string::npos &&
                state.lastAction.find("must be placed") == std::string::npos &&
                state.lastAction.find("cannot share") == std::string::npos) {
                autosave(state, "Painted ground enemy spawn.");
            }
            break;
        case BrushMode::FlyingEnemy:
            applyEnemyBrush(state, kFlyingEnemyTemplateId);
            if (state.lastAction.find("outside editable area") == std::string::npos &&
                state.lastAction.find("must be placed") == std::string::npos &&
                state.lastAction.find("cannot share") == std::string::npos) {
                autosave(state, "Painted flying enemy spawn.");
            }
            break;
        case BrushMode::Spawn:
            applySpawnBrush(state);
            if (state.lastAction.find("outside editable area") == std::string::npos &&
                state.lastAction.find("must be on empty space") == std::string::npos) {
                autosave(state, "Moved spawn point.");
            }
            break;
        case BrushMode::Erase:
            applyEraseBrush(state);
            if (state.lastAction.find("outside editable area") == std::string::npos) {
                autosave(state, "Erased cell.");
            }
            break;
        default:
            break;
    }
}

size_t chooseInitialMapIndex(const std::vector<std::string>& mapIds) {
    const std::vector<std::string>::const_iterator editorIt =
            std::find(mapIds.begin(), mapIds.end(), std::string(kDefaultEditorMapId));
    return editorIt != mapIds.end() ? static_cast<size_t>(editorIt - mapIds.begin()) : 0;
}

void positionCursorForLoadedMap(EditorState& state) {
    if (!state.mapDefinition.spawnPoints.empty() &&
        isInsideEditableArea(state, state.mapDefinition.spawnPoints.front().position)) {
        state.cursor = state.mapDefinition.spawnPoints.front().position;
        return;
    }

    const game::Position defaultCursor(3, kDefaultMapHeight - 2);
    if (isInsideEditableArea(state, defaultCursor)) {
        state.cursor = defaultCursor;
        return;
    }

    state.cursor = game::Position(1, 1);
}

void loadCurrentMap(EditorState& state) {
    game::MapLoader mapLoader;
    const std::string currentMapId = state.mapIds[state.currentMapIndex];

    try {
        state.mapDefinition = mapLoader.loadMap(currentMapId);
        if (state.mapDefinition.terrainRows.empty()) {
            state.mapDefinition = createBlankMapDefinition(currentMapId);
        }
        state.lastAction = "Loaded map " + currentMapId + ".";
    } catch (...) {
        state.mapDefinition = createBlankMapDefinition(currentMapId);
        autosave(state, "Created blank map " + currentMapId + ".");
    }

    positionCursorForLoadedMap(state);
}

EditorState loadInitialEditorState() {
    EditorState state;
    state.mapIds = loadMapIds();
    state.currentMapIndex = chooseInitialMapIndex(state.mapIds);
    loadCurrentMap(state);
    return state;
}

void switchMap(EditorState& state, int direction) {
    if (state.mapIds.empty()) {
        return;
    }

    const int count = static_cast<int>(state.mapIds.size());
    const int nextIndex = (static_cast<int>(state.currentMapIndex) + direction + count) % count;
    state.currentMapIndex = static_cast<size_t>(nextIndex);
    loadCurrentMap(state);
}

std::string nextDraftMapId(const std::vector<std::string>& mapIds) {
    int suffix = 1;
    while (true) {
        const std::string candidate = "draft_map_" + std::to_string(suffix);
        if (std::find(mapIds.begin(), mapIds.end(), candidate) == mapIds.end()) {
            return candidate;
        }
        ++suffix;
    }
}

void createNewMap(EditorState& state) {
    const std::string newMapId = nextDraftMapId(state.mapIds);
    state.mapDefinition = createBlankMapDefinition(newMapId);
    autosave(state, "Created new map " + newMapId + ".");
    positionCursorForLoadedMap(state);
}

void upsertSpawnPoint(game::MapDefinition& mapDefinition,
                      const std::string& spawnPointId,
                      const game::Position& position) {
    for (size_t index = 0; index < mapDefinition.spawnPoints.size(); ++index) {
        if (mapDefinition.spawnPoints[index].id == spawnPointId) {
            mapDefinition.spawnPoints[index].position = position;
            return;
        }
    }

    mapDefinition.spawnPoints.push_back(game::SpawnPoint{spawnPointId, position});
}

void upsertTransition(game::MapDefinition& mapDefinition,
                      const std::string& toMapId,
                      const std::string& spawnPointId,
                      const game::Position& triggerPosition) {
    for (size_t index = 0; index < mapDefinition.transitions.size(); ++index) {
        if (mapDefinition.transitions[index].triggerPosition.x == triggerPosition.x &&
            mapDefinition.transitions[index].triggerPosition.y == triggerPosition.y) {
            mapDefinition.transitions[index].fromMapId = mapDefinition.id;
            mapDefinition.transitions[index].toMapId = toMapId;
            mapDefinition.transitions[index].spawnPointId = spawnPointId;
            mapDefinition.transitions[index].triggerPosition = triggerPosition;
            return;
        }
    }

    game::MapTransition transition;
    transition.fromMapId = mapDefinition.id;
    transition.toMapId = toMapId;
    transition.spawnPointId = spawnPointId;
    transition.triggerPosition = triggerPosition;
    mapDefinition.transitions.push_back(transition);
}

void toggleDoorLink(EditorState& state) {
    if (!isInsideEditableArea(state, state.cursor)) {
        state.lastAction = "Door must be placed inside editable area.";
        return;
    }

    if (!state.pendingDoorLink.active) {
        state.pendingDoorLink.active = true;
        state.pendingDoorLink.sourceMapId = state.mapDefinition.id;
        state.pendingDoorLink.sourcePosition = state.cursor;
        state.lastAction = "Door source marked. Switch to target map and press H again to complete link.";
        return;
    }

    const std::string sourceMapId = state.pendingDoorLink.sourceMapId;
    const game::Position sourcePosition = state.pendingDoorLink.sourcePosition;
    const std::string targetMapId = state.mapDefinition.id;
    const game::Position targetPosition = state.cursor;

    game::MapDefinition sourceMapDefinition = sourceMapId == state.mapDefinition.id
            ? state.mapDefinition
            : loadOrCreateMapDefinition(sourceMapId);
    game::MapDefinition targetMapDefinition = sourceMapId == targetMapId
            ? sourceMapDefinition
            : state.mapDefinition;

    const std::string targetSpawnId = buildDoorSpawnId(sourceMapId, sourcePosition);
    const std::string sourceSpawnId = buildDoorSpawnId(targetMapId, targetPosition);

    upsertSpawnPoint(targetMapDefinition, targetSpawnId, targetPosition);
    upsertSpawnPoint(sourceMapDefinition, sourceSpawnId, sourcePosition);
    upsertTransition(sourceMapDefinition, targetMapId, targetSpawnId, sourcePosition);
    upsertTransition(targetMapDefinition, sourceMapId, sourceSpawnId, targetPosition);

    saveMapDefinition(sourceMapDefinition);
    if (targetMapDefinition.id != sourceMapDefinition.id) {
        saveMapDefinition(targetMapDefinition);
    }

    if (state.mapDefinition.id == sourceMapDefinition.id) {
        state.mapDefinition = sourceMapDefinition;
    } else {
        state.mapDefinition = targetMapDefinition;
    }

    state.mapIds = loadMapIds();
    const std::vector<std::string>::iterator it =
            std::find(state.mapIds.begin(), state.mapIds.end(), state.mapDefinition.id);
    if (it != state.mapIds.end()) {
        state.currentMapIndex = static_cast<size_t>(it - state.mapIds.begin());
    }

    state.pendingDoorLink.active = false;
    state.lastAction = "Door link created: " + sourceMapId + " <-> " + targetMapId + ".";
}

std::string buildEditorRenderMap(const EditorState& state) {
    std::vector<std::string> rows = state.mapDefinition.terrainRows;

    for (size_t index = 0; index < state.mapDefinition.transitions.size(); ++index) {
        const game::Position& triggerPosition = state.mapDefinition.transitions[index].triggerPosition;
        rows[static_cast<size_t>(triggerPosition.y)]
            [static_cast<size_t>(triggerPosition.x)] = 'D';
    }

    for (size_t index = 0; index < state.mapDefinition.enemySpawns.size(); ++index) {
        rows[static_cast<size_t>(state.mapDefinition.enemySpawns[index].position.y)]
            [static_cast<size_t>(state.mapDefinition.enemySpawns[index].position.x)] =
                    enemyGlyphForTemplate(state.mapDefinition.enemySpawns[index].enemyTemplateId);
    }

    if (!state.mapDefinition.spawnPoints.empty()) {
        rows[static_cast<size_t>(state.mapDefinition.spawnPoints.front().position.y)]
            [static_cast<size_t>(state.mapDefinition.spawnPoints.front().position.x)] = '@';
    }

    char cursorGlyph = '+';
    if (isSpawnPointAt(state, state.cursor)) {
        cursorGlyph = 'S';
    } else if (findEnemyIndexAt(state, state.cursor) >= 0) {
        cursorGlyph = enemyGlyphForTemplate(
                state.mapDefinition.enemySpawns[static_cast<size_t>(findEnemyIndexAt(state, state.cursor))].enemyTemplateId) == 'g'
                ? 'G'
                : 'F';
    } else if (findTransitionIndexAt(state.mapDefinition, state.cursor) >= 0) {
        cursorGlyph = 'H';
    } else if (terrainAt(state, state.cursor) == '=') {
        cursorGlyph = '#';
    }

    rows[static_cast<size_t>(state.cursor.y)][static_cast<size_t>(state.cursor.x)] = cursorGlyph;

    std::ostringstream stream;
    for (size_t index = 0; index < rows.size(); ++index) {
        stream << rows[index];
        if (index + 1 < rows.size()) {
            stream << '\n';
        }
    }

    return stream.str();
}

std::string buildEditorHud(const EditorState& state) {
    std::ostringstream hud;
    hud << "[MapEngine] ESC exit | S demo | O prev | M next | R new map | X force save\n";
    hud << "Cursor I/J/K/L move | 1 platform | 2 ground | 4 flying | 3 spawn | C erase | E paint / drag | H link door | Autosave ON\n";
    hud << "Current Map " << state.mapDefinition.id
        << " (" << (state.currentMapIndex + 1) << "/" << state.mapIds.size() << ")"
        << " | Type " << mapTypeToString(state.mapDefinition.type)
        << " | Brush " << brushName(state.brushMode);
    if (!state.mapDefinition.spawnPoints.empty()) {
        hud << " | Spawn (" << state.mapDefinition.spawnPoints.front().position.x
            << ", " << state.mapDefinition.spawnPoints.front().position.y << ")";
    }
    int groundCount = 0;
    int flyingCount = 0;
    for (size_t index = 0; index < state.mapDefinition.enemySpawns.size(); ++index) {
        if (isGroundEnemyTemplate(state.mapDefinition.enemySpawns[index].enemyTemplateId)) {
            groundCount++;
        } else {
            flyingCount++;
        }
    }
    hud << " | Ground " << groundCount << " | Flying " << flyingCount << "\n";
    hud << "Cursor (" << state.cursor.x << ", " << state.cursor.y << ")"
        << " | Save target " << mapPathForId(state.mapDefinition.id) << "\n";
    if (state.pendingDoorLink.active) {
        hud << "Pending Door: " << state.pendingDoorLink.sourceMapId
            << " (" << state.pendingDoorLink.sourcePosition.x << ", " << state.pendingDoorLink.sourcePosition.y << ")"
            << " -> choose target and press H\n";
    }
    hud << "Legend: +=cursor on empty  #=cursor on platform  G=ground  F=flying  D=door  H=cursor on door  S=cursor on spawn\n";
    hud << "Last Action: " << state.lastAction << "\n\n";
    return hud.str();
}

bool canEnemyOccupy(const std::string& terrainMap,
                    const game::Position& playerPosition,
                    const std::vector<game::GroundEnemy>& groundEnemies,
                    const std::vector<game::FlyingEnemy>& flyingEnemies,
                    const game::Position& targetPosition,
                    bool ignoreGround,
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
        if (ignoreGround && static_cast<int>(index) == ignoreIndex) {
            continue;
        }
        if (!groundEnemies[index].isRenderable()) {
            continue;
        }
        const game::Position enemyPosition = groundEnemies[index].getPosition();
        if (enemyPosition.x == targetPosition.x && enemyPosition.y == targetPosition.y) {
            return false;
        }
    }

    for (size_t index = 0; index < flyingEnemies.size(); ++index) {
        if (!ignoreGround && static_cast<int>(index) == ignoreIndex) {
            continue;
        }
        if (!flyingEnemies[index].isRenderable()) {
            continue;
        }
        const game::Position enemyPosition = flyingEnemies[index].getPosition();
        if (enemyPosition.x == targetPosition.x && enemyPosition.y == targetPosition.y) {
            return false;
        }
    }

    return true;
}

std::string buildDemoCollisionMap(const DemoState& demoState) {
    std::string collisionMap = demoState.terrainMap;
    for (size_t index = 0; index < demoState.groundEnemies.size(); ++index) {
        if (demoState.groundEnemies[index].isRenderable()) {
            placeGlyph(collisionMap, demoState.groundEnemies[index].getPosition(), 'g');
        }
    }
    for (size_t index = 0; index < demoState.flyingEnemies.size(); ++index) {
        if (demoState.flyingEnemies[index].isRenderable()) {
            placeGlyph(collisionMap, demoState.flyingEnemies[index].getPosition(), 'f');
        }
    }
    placeGlyph(collisionMap, demoState.playerPosition, '@');
    return collisionMap;
}

int findClosestGroundEnemyIndex(const DemoState& demoState) {
    int bestIndex = -1;
    int bestDistance = 1000000;
    for (size_t index = 0; index < demoState.groundEnemies.size(); ++index) {
        if (!demoState.groundEnemies[index].isRenderable()) {
            continue;
        }

        const game::Position enemyPosition = demoState.groundEnemies[index].getPosition();
        const int distance = static_cast<int>(std::abs(enemyPosition.x - demoState.playerPosition.x) +
                                              std::abs(enemyPosition.y - demoState.playerPosition.y));
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }

    return bestIndex;
}

int findClosestFlyingEnemyIndex(const DemoState& demoState) {
    int bestIndex = -1;
    int bestDistance = 1000000;
    for (size_t index = 0; index < demoState.flyingEnemies.size(); ++index) {
        if (!demoState.flyingEnemies[index].isRenderable()) {
            continue;
        }

        const game::Position enemyPosition = demoState.flyingEnemies[index].getPosition();
        const int distance = static_cast<int>(std::abs(enemyPosition.x - demoState.playerPosition.x) +
                                              std::abs(enemyPosition.y - demoState.playerPosition.y));
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }

    return bestIndex;
}

void removeDefeatedEnemies(DemoState& demoState) {
    std::vector<game::GroundEnemy> survivors;
    for (size_t index = 0; index < demoState.groundEnemies.size(); ++index) {
        if (!demoState.groundEnemies[index].shouldDespawn()) {
            survivors.push_back(demoState.groundEnemies[index]);
        }
    }
    demoState.groundEnemies.swap(survivors);

    std::vector<game::FlyingEnemy> flyingSurvivors;
    for (size_t index = 0; index < demoState.flyingEnemies.size(); ++index) {
        if (!demoState.flyingEnemies[index].shouldDespawn()) {
            flyingSurvivors.push_back(demoState.flyingEnemies[index]);
        }
    }
    demoState.flyingEnemies.swap(flyingSurvivors);
}

void startDemoFromEditor(const EditorState& editorState, DemoState& demoState, Player& player) {
    demoState.active = true;
    demoState.terrainMap = joinTerrainRows(editorState.mapDefinition.terrainRows);
    demoState.playerPosition = editorState.mapDefinition.spawnPoints.empty()
            ? game::Position(3, kDefaultMapHeight - 2)
            : editorState.mapDefinition.spawnPoints.front().position;
    demoState.groundEnemies.clear();
    demoState.flyingEnemies.clear();
    demoState.enemyProjectiles.clear();

    for (size_t index = 0; index < editorState.mapDefinition.enemySpawns.size(); ++index) {
        if (isGroundEnemyTemplate(editorState.mapDefinition.enemySpawns[index].enemyTemplateId)) {
            demoState.groundEnemies.push_back(
                    game::GroundEnemy("editor_ground_enemy_" + std::to_string(index + 1),
                                      editorState.mapDefinition.enemySpawns[index].position));
        } else {
            demoState.flyingEnemies.push_back(
                    game::FlyingEnemy("editor_flying_enemy_" + std::to_string(index + 1),
                                      editorState.mapDefinition.enemySpawns[index].position));
        }
    }

    player.resetRuntimeState();
}

void spawnFlyingProjectile(const game::FlyingEnemy& enemy,
                           const game::Position& playerPosition,
                           DemoState& demoState) {
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
    demoState.enemyProjectiles.push_back(projectile);
}

void updateEnemyProjectiles(const std::string& terrainMap,
                            const game::Position& playerPosition,
                            DemoState& demoState,
                            Player& player) {
    std::vector<EnemyProjectile> nextProjectiles;
    nextProjectiles.reserve(demoState.enemyProjectiles.size());

    for (size_t index = 0; index < demoState.enemyProjectiles.size(); ++index) {
        EnemyProjectile projectile = demoState.enemyProjectiles[index];
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

    demoState.enemyProjectiles.swap(nextProjectiles);
}

std::string buildDemoWorldStatus(const EditorState& state, const DemoState& demoState) {
    std::ostringstream status;
    status << "[Layout Demo] X back to editor | P reset demo | Map " << state.mapDefinition.id << "\n";
    status << "Ground " << demoState.groundEnemies.size()
           << " | Flying " << demoState.flyingEnemies.size()
           << " | Projectiles " << demoState.enemyProjectiles.size()
           << " | Player Spawn (" << demoState.playerPosition.x << ", " << demoState.playerPosition.y << ")\n\n";
    return status.str();
}

std::string buildDemoRenderMap(const DemoState& demoState, const Player& player) {
    std::string renderMap = demoState.terrainMap;
    for (size_t index = 0; index < demoState.groundEnemies.size(); ++index) {
        if (!demoState.groundEnemies[index].isRenderable()) {
            continue;
        }

        const game::Position enemyPosition = demoState.groundEnemies[index].getPosition();
        placeGlyph(renderMap, enemyPosition, demoState.groundEnemies[index].getRenderGlyph());

        if (demoState.groundEnemies[index].getState() == game::GroundEnemyState::AttackStartup) {
            const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
            placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
            placeGlyph(renderMap, warningRow, '!');
            placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
        }
    }

    for (size_t index = 0; index < demoState.flyingEnemies.size(); ++index) {
        if (!demoState.flyingEnemies[index].isRenderable()) {
            continue;
        }

        const game::Position enemyPosition = demoState.flyingEnemies[index].getPosition();
        placeGlyph(renderMap, enemyPosition, demoState.flyingEnemies[index].getRenderGlyph());

        if (demoState.flyingEnemies[index].getState() == game::FlyingEnemyState::AttackStartup) {
            const game::Position warningRow(enemyPosition.x, enemyPosition.y - 1);
            placeGlyph(renderMap, game::Position(warningRow.x - 1, warningRow.y), '!');
            placeGlyph(renderMap, warningRow, '!');
            placeGlyph(renderMap, game::Position(warningRow.x + 1, warningRow.y), '!');
        }
    }

    for (size_t index = 0; index < demoState.enemyProjectiles.size(); ++index) {
        placeGlyph(renderMap, demoState.enemyProjectiles[index].position, '*');
    }

    std::string gameplayMap = demoState.terrainMap;
    placeGlyph(gameplayMap, demoState.playerPosition, '@');
    player.overlayRender(renderMap, gameplayMap);
    return renderMap;
}

} // namespace

int main() {
    MapDrawer mapDrawer;
    KeyStateManager keyStateManager;
    Player player(keyStateManager);
    EditorState state = loadInitialEditorState();
    DemoState demoState;

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isKeyDown(keyStateManager, 0x1B)) {
            break;
        }

        if (!demoState.active) {
            const game::Position previousCursor = state.cursor;

            if (isJustPressed(keyStateManager, state, 'i') || isJustPressed(keyStateManager, state, 'I')) {
                moveCursor(state, 0, -1);
            }
            if (isJustPressed(keyStateManager, state, 'k') || isJustPressed(keyStateManager, state, 'K')) {
                moveCursor(state, 0, 1);
            }
            if (isJustPressed(keyStateManager, state, 'j') || isJustPressed(keyStateManager, state, 'J')) {
                moveCursor(state, -1, 0);
            }
            if (isJustPressed(keyStateManager, state, 'l') || isJustPressed(keyStateManager, state, 'L')) {
                moveCursor(state, 1, 0);
            }

            if (isJustPressed(keyStateManager, state, '1')) {
                setBrush(state, BrushMode::Platform);
                paintAtCursor(state);
            }
            if (isJustPressed(keyStateManager, state, '2')) {
                setBrush(state, BrushMode::GroundEnemy);
                paintAtCursor(state);
            }
            if (isJustPressed(keyStateManager, state, '4')) {
                setBrush(state, BrushMode::FlyingEnemy);
                paintAtCursor(state);
            }
            if (isJustPressed(keyStateManager, state, '3')) {
                setBrush(state, BrushMode::Spawn);
                paintAtCursor(state);
            }
            if (isJustPressed(keyStateManager, state, 'c') || isJustPressed(keyStateManager, state, 'C')) {
                setBrush(state, BrushMode::Erase);
                paintAtCursor(state);
            }
            if (isJustPressed(keyStateManager, state, 'o') || isJustPressed(keyStateManager, state, 'O')) {
                switchMap(state, -1);
            }
            if (isJustPressed(keyStateManager, state, 'm') || isJustPressed(keyStateManager, state, 'M')) {
                switchMap(state, 1);
            }
            if (isJustPressed(keyStateManager, state, 'r') || isJustPressed(keyStateManager, state, 'R')) {
                createNewMap(state);
            }
            if (isJustPressed(keyStateManager, state, 'h') || isJustPressed(keyStateManager, state, 'H')) {
                try {
                    toggleDoorLink(state);
                } catch (const std::exception& exception) {
                    state.pendingDoorLink.active = false;
                    state.lastAction = std::string("Door link failed: ") + exception.what();
                }
            }
            if (isJustPressed(keyStateManager, state, 'x') || isJustPressed(keyStateManager, state, 'X')) {
                try {
                    autosave(state, "Manual save completed.");
                } catch (const std::exception& exception) {
                    state.lastAction = std::string("Save failed: ") + exception.what();
                }
            }

            const bool cursorMoved = state.cursor.x != previousCursor.x || state.cursor.y != previousCursor.y;
            if (isKeyDown(keyStateManager, 'e') || isKeyDown(keyStateManager, 'E')) {
                if (isJustPressed(keyStateManager, state, 'e') ||
                    isJustPressed(keyStateManager, state, 'E') ||
                    cursorMoved) {
                    paintAtCursor(state);
                }
            }

            if (isJustPressed(keyStateManager, state, 's') || isJustPressed(keyStateManager, state, 'S')) {
                startDemoFromEditor(state, demoState, player);
                state.lastAction = "Started layout demo for " + state.mapDefinition.id + ".";
            }

            mapDrawer.currentmap = buildEditorHud(state) + buildEditorRenderMap(state);
        } else {
            if (isJustPressed(keyStateManager, state, 'x') || isJustPressed(keyStateManager, state, 'X')) {
                demoState.active = false;
                state.lastAction = "Returned to editor mode for " + state.mapDefinition.id + ".";
            } else {
                if (isJustPressed(keyStateManager, state, 'p') || isJustPressed(keyStateManager, state, 'P')) {
                    startDemoFromEditor(state, demoState, player);
                }

                std::string collisionMap = buildDemoCollisionMap(demoState);
                if (player.isAlive() && !player.isMovementLocked()) {
                    player.move(collisionMap);
                    const game::Position movedPlayerPosition = findGlyphPosition(collisionMap, '@');
                    if (movedPlayerPosition.x >= 0 && movedPlayerPosition.y >= 0) {
                        demoState.playerPosition = movedPlayerPosition;
                    }
                }

                std::string gameplayMap = demoState.terrainMap;
                placeGlyph(gameplayMap, demoState.playerPosition, '@');

                const int activeGroundEnemyIndex = findClosestGroundEnemyIndex(demoState);
                const int activeFlyingEnemyIndex = findClosestFlyingEnemyIndex(demoState);
                if (activeGroundEnemyIndex >= 0 && activeFlyingEnemyIndex >= 0) {
                    player.updateCombat(gameplayMap,
                                        demoState.groundEnemies[static_cast<size_t>(activeGroundEnemyIndex)],
                                        demoState.flyingEnemies[static_cast<size_t>(activeFlyingEnemyIndex)]);
                } else if (activeGroundEnemyIndex >= 0) {
                    player.updateCombat(gameplayMap,
                                        demoState.groundEnemies[static_cast<size_t>(activeGroundEnemyIndex)],
                                        demoState.dummyFlyingEnemy);
                } else if (activeFlyingEnemyIndex >= 0) {
                    player.updateCombat(gameplayMap,
                                        demoState.dummyGroundEnemy,
                                        demoState.flyingEnemies[static_cast<size_t>(activeFlyingEnemyIndex)]);
                } else {
                    player.updateCombat(gameplayMap, demoState.dummyGroundEnemy, demoState.dummyFlyingEnemy);
                }

                updateEnemyProjectiles(demoState.terrainMap, demoState.playerPosition, demoState, player);

                if (player.consumeResetRequest()) {
                    startDemoFromEditor(state, demoState, player);
                }

                for (size_t index = 0; index < demoState.groundEnemies.size(); ++index) {
                    if (!demoState.groundEnemies[index].isRenderable() || !player.isAlive()) {
                        continue;
                    }

                    const game::Position previousPosition = demoState.groundEnemies[index].getPosition();
                    demoState.groundEnemies[index].updateAI(demoState.playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
                    const game::Position nextPosition = demoState.groundEnemies[index].getPosition();

                    if (demoState.groundEnemies[index].isAlive() &&
                        (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                        !canEnemyOccupy(demoState.terrainMap,
                                        demoState.playerPosition,
                                        demoState.groundEnemies,
                                        demoState.flyingEnemies,
                                        nextPosition,
                                        true,
                                        static_cast<int>(index))) {
                        demoState.groundEnemies[index].setPosition(previousPosition);
                    }

                    if (demoState.groundEnemies[index].consumeAttackTrigger()) {
                        if (demoState.groundEnemies[index].isTouchingPlayer(demoState.playerPosition) ||
                            demoState.groundEnemies[index].isInAttackRange(demoState.playerPosition)) {
                            player.receiveDamage("Ground enemy", demoState.playerPosition);
                        }
                    }
                }

                for (size_t index = 0; index < demoState.flyingEnemies.size(); ++index) {
                    if (!demoState.flyingEnemies[index].isRenderable() || !player.isAlive()) {
                        continue;
                    }

                    const game::Position previousPosition = demoState.flyingEnemies[index].getPosition();
                    demoState.flyingEnemies[index].updateAI(demoState.playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
                    const game::Position nextPosition = demoState.flyingEnemies[index].getPosition();

                    if (demoState.flyingEnemies[index].isAlive() &&
                        (nextPosition.x != previousPosition.x || nextPosition.y != previousPosition.y) &&
                        !canEnemyOccupy(demoState.terrainMap,
                                        demoState.playerPosition,
                                        demoState.groundEnemies,
                                        demoState.flyingEnemies,
                                        nextPosition,
                                        false,
                                        static_cast<int>(index))) {
                        demoState.flyingEnemies[index].setPosition(previousPosition);
                    }

                    if (demoState.flyingEnemies[index].consumeProjectileTrigger()) {
                        spawnFlyingProjectile(demoState.flyingEnemies[index], demoState.playerPosition, demoState);
                    }
                }

                removeDefeatedEnemies(demoState);
                mapDrawer.currentmap = player.buildHud() +
                                       buildDemoWorldStatus(state, demoState) +
                                       buildDemoRenderMap(demoState, player);
            }
        }

        mapDrawer.draw();
        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
