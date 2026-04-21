#include <algorithm>
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
const std::string kDefaultRuntimePrompt =
        std::string("A/D move, SPACE jump, ") +
        runtimeDashKeyLabel() +
        " dash, J/U/N slash, K/I/M spell, E interact.";
const char* kShortcutSpawnModule03 = "shortcut_spawn_module_03";
const char* kShortcutSpawnModule05 = "shortcut_spawn_module_05";
const char* kSkillDoubleJump = "double_jump";
const char* kBossRoom01MapId = "boss_room_01";
const char* kBossRoom01BossId = "boss_room_01_guardian";
const char* kBossRoom01BossName = "Hall Guardian";
const int kBossMeleeRange = 2;
const int kBossHorizontalWaveRange = 12;
const int kBossVerticalSpellRange = 8;

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

struct BossImpact {
    std::vector<game::BossVisualGlyph> activeGlyphs;
    std::vector<game::BossVisualGlyph> fadeGlyphs;
    std::vector<game::Position> damageCells;
    int activeFramesRemaining;
    int fadeFramesRemaining;
    std::string label;

    BossImpact()
        : activeFramesRemaining(0),
          fadeFramesRemaining(0) {
    }
};

struct BossEncounterState {
    bool active;
    bool dialogueActive;
    bool battleActive;
    bool victoryActive;
    int dialogueIndex;
    int rewardHkd;
    std::string bossId;
    std::string bossName;
    std::vector<std::string> dialogueLines;
    std::vector<BossImpact> impacts;
    game::MeleeBoss boss;

    BossEncounterState()
        : active(false),
          dialogueActive(false),
          battleActive(false),
          victoryActive(false),
          dialogueIndex(0),
          rewardHkd(0),
          bossId(kBossRoom01BossId),
          bossName(kBossRoom01BossName),
          boss(kBossRoom01BossId, game::Position()) {
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
    std::string bossStatus;
    bool pauseConfirmOpen;
    bool pauseQuitSelected;
    std::unordered_map<int, bool> previousKeys;

    RuntimeState()
        : currentMapId("spawn_village"),
          currentSpawnPointId("player_start"),
          lastInteraction("Main world ready."),
          prompt(kDefaultRuntimePrompt),
          shopOpen(false),
          shopSelection(0),
          pauseConfirmOpen(false),
          pauseQuitSelected(false) {
    }
};

enum class TitleMenuSelection {
    NewGame,
    ContinueGame
};

enum class TitleScreenState {
    TitleScreen,
    ContinueUnavailable,
    ExitConfirm,
    StartTransition
};

std::string buildMenuOptionLine(const std::string& label, bool selected) {
    return selected ? "> " + label : "  " + label;
}

std::vector<std::string> buildPauseOverlayLines(bool quitSelected) {
    std::vector<std::string> lines;
    lines.push_back("PAUSED");
    lines.push_back(buildMenuOptionLine("RESUME", !quitSelected));
    lines.push_back(buildMenuOptionLine("QUIT TO DESKTOP", quitSelected));
    lines.push_back("W/S SELECT  ENTER CONFIRM  ESC BACK");
    return lines;
}

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
                                bool hasSaveData,
                                bool exitQuitSelected) {
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
    } else if (state == TitleScreenState::ExitConfirm) {
        const std::vector<std::string> confirmLines = buildPauseOverlayLines(exitQuitSelected);
        for (size_t index = 0; index < confirmLines.size(); ++index) {
            placeCenteredLine(lines, 18 + static_cast<int>(index), confirmLines[index]);
        }
    } else {
        placeCenteredLine(lines, 18, "W/S SELECT  ENTER CONFIRM  ESC MENU");
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

        if (isJustPressed(keyStateManager, previousKeys, 0x1B)) {
            if (state == TitleScreenState::ExitConfirm) {
                state = TitleScreenState::TitleScreen;
                stateFrames = 0;
            } else if (state != TitleScreenState::StartTransition) {
                state = TitleScreenState::ExitConfirm;
                stateFrames = 0;
            }
        }

        if (state == TitleScreenState::StartTransition) {
            stateFrames++;
            mapDrawer.currentmap = buildTitleScreenMap(selection, state, animationFrame++, hasSaveData, false);
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

        if (state == TitleScreenState::ExitConfirm) {
            if (isJustPressed(keyStateManager, previousKeys, 'w') ||
                isJustPressed(keyStateManager, previousKeys, 'W') ||
                isJustPressed(keyStateManager, previousKeys, 's') ||
                isJustPressed(keyStateManager, previousKeys, 'S')) {
                stateFrames = stateFrames == 0 ? 1 : 0;
            }

            if (isJustPressed(keyStateManager, previousKeys, 0x0D)) {
                if (stateFrames != 0) {
                    return TitleScreenResult::Exit;
                }
                state = TitleScreenState::TitleScreen;
                stateFrames = 0;
            }

            mapDrawer.currentmap = buildTitleScreenMap(selection,
                                                       state,
                                                       animationFrame++,
                                                       hasSaveData,
                                                       stateFrames != 0);
            mapDrawer.draw();
            previousKeys = keyStateManager.keyStates;
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

        mapDrawer.currentmap = buildTitleScreenMap(selection, state, animationFrame++, hasSaveData, false);
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
        npc.glyph = 'O';
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

    if (mapDefinition.id == kBossRoom01MapId) {
        return;
    }

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

std::string skillIdToSaveKey(game::SkillId skillId) {
    switch (skillId) {
    case game::SkillId::DoubleJump:
        return kSkillDoubleJump;
    default:
        return std::string();
    }
}

bool hasUnlockedSkill(const game::SaveData& saveData, game::SkillId skillId) {
    const std::string key = skillIdToSaveKey(skillId);
    return !key.empty() &&
           std::find(saveData.unlockedSkillIds.begin(),
                     saveData.unlockedSkillIds.end(),
                     key) != saveData.unlockedSkillIds.end();
}

void unlockSkill(game::SaveData& saveData, game::SkillId skillId) {
    const std::string key = skillIdToSaveKey(skillId);
    if (key.empty() ||
        std::find(saveData.unlockedSkillIds.begin(), saveData.unlockedSkillIds.end(), key) !=
                saveData.unlockedSkillIds.end()) {
        return;
    }

    saveData.unlockedSkillIds.push_back(key);
}

void applyPersistentSkillsToPlayer(Player& player, const game::SaveData& saveData) {
    player.setDoubleJumpUnlocked(hasUnlockedSkill(saveData, game::SkillId::DoubleJump));
}

std::vector<game::ShopOffer> filterShopOffersForSave(const std::vector<game::ShopOffer>& offers,
                                                     const game::SaveData& saveData) {
    std::vector<game::ShopOffer> filtered;
    for (size_t index = 0; index < offers.size(); ++index) {
        if (offers[index].unlocksSkill && hasUnlockedSkill(saveData, offers[index].unlockedSkill)) {
            continue;
        }
        filtered.push_back(offers[index]);
    }
    return filtered;
}

std::string getShortcutDoorId(const game::MapTransition& transition) {
    if ((transition.fromMapId == "spawn_village" && transition.toMapId == "module_03") ||
        (transition.fromMapId == "module_03" && transition.toMapId == "spawn_village")) {
        return kShortcutSpawnModule03;
    }

    if ((transition.fromMapId == "spawn_village" && transition.toMapId == "module_05") ||
        (transition.fromMapId == "module_05" && transition.toMapId == "spawn_village")) {
        return kShortcutSpawnModule05;
    }

    return std::string();
}

bool isShortcutUnlocked(const game::SaveData& saveData, const std::string& shortcutId) {
    return !shortcutId.empty() &&
           std::find(saveData.unlockedShortcutIds.begin(),
                     saveData.unlockedShortcutIds.end(),
                     shortcutId) != saveData.unlockedShortcutIds.end();
}

bool isShortcutVillageSide(const game::MapTransition& transition) {
    return transition.fromMapId == "spawn_village" && !getShortcutDoorId(transition).empty();
}

bool isLockedVillageShortcut(const game::SaveData& saveData, const game::MapTransition& transition) {
    const std::string shortcutId = getShortcutDoorId(transition);
    return !shortcutId.empty() &&
           isShortcutVillageSide(transition) &&
           !isShortcutUnlocked(saveData, shortcutId);
}

bool isRemoteUnlockShortcut(const game::SaveData& saveData, const game::MapTransition& transition) {
    const std::string shortcutId = getShortcutDoorId(transition);
    return !shortcutId.empty() &&
           !isShortcutVillageSide(transition) &&
           !isShortcutUnlocked(saveData, shortcutId);
}

char getTransitionGlyph(const game::SaveData& saveData, const game::MapTransition& transition) {
    if (isLockedVillageShortcut(saveData, transition)) {
        return 'L';
    }

    if (isRemoteUnlockShortcut(saveData, transition)) {
        return 'U';
    }

    return 'D';
}

std::string buildTransitionPrompt(const game::SaveData& saveData, const game::MapTransition& transition) {
    if (isLockedVillageShortcut(saveData, transition)) {
        return "Press E to inspect locked shortcut.";
    }

    if (isRemoteUnlockShortcut(saveData, transition)) {
        return "Press E to unlock shortcut door.";
    }

    return "Press E to use door.";
}

bool isBossRoom01(const game::MapDefinition& mapDefinition) {
    return mapDefinition.id == kBossRoom01MapId;
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

std::vector<std::string> splitMapRows(const std::string& map) {
    std::vector<std::string> rows;
    std::istringstream stream(map);
    std::string line;
    while (std::getline(stream, line)) {
        rows.push_back(line);
    }
    return rows;
}

void overlayCenteredLines(std::string& map, int startRow, const std::vector<std::string>& lines) {
    std::vector<std::string> rows = splitMapRows(map);
    for (size_t index = 0; index < lines.size(); ++index) {
        placeCenteredLine(rows, startRow + static_cast<int>(index), lines[index]);
    }
    map = joinTerrainRows(rows);
}

bool isBossDefeated(const game::SaveData& saveData, const std::string& bossId) {
    return !bossId.empty() &&
           std::find(saveData.defeatedBossIds.begin(),
                     saveData.defeatedBossIds.end(),
                     bossId) != saveData.defeatedBossIds.end();
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

bool hasBossAnchorSpace(const std::string& terrainMap, const game::Position& anchor) {
    for (int dy = -5; dy <= 2; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            const game::Position candidate(anchor.x + dx, anchor.y + dy);
            if (!isInsidePlayableArea(terrainMap, candidate) || tileAt(terrainMap, candidate) != ' ') {
                return false;
            }
        }
    }
    return true;
}

game::Position findBossSpawnPosition(const std::string& terrainMap) {
    const int width = static_cast<int>(lineWidth(terrainMap)) - 1;
    const int height = width <= 0 ? 0 : static_cast<int>((terrainMap.length() + 1) / lineWidth(terrainMap));
    const int centerX = width / 2;

    for (int offset = 0; offset < width / 2; ++offset) {
        const int candidates[2] = {centerX - offset, centerX + offset};
        for (int candidateIndex = 0; candidateIndex < 2; ++candidateIndex) {
            const int x = candidates[candidateIndex];
            if (x <= 4 || x >= width - 4) {
                continue;
            }

            for (int y = std::max(6, height - 6); y >= 6; --y) {
                const game::Position anchor(x, y);
                if (!hasBossAnchorSpace(terrainMap, anchor)) {
                    continue;
                }
                return anchor;
            }
        }
    }

    return game::Position(std::max(6, centerX), std::max(6, height / 2));
}

BossImpact toBossImpact(const game::BossAttackVisual& visual) {
    BossImpact impact;
    impact.damageCells = visual.damageCells;
    impact.activeFramesRemaining = visual.activeFrames;
    impact.fadeFramesRemaining = visual.fadeFrames;
    impact.label = visual.label;
    impact.activeGlyphs = visual.activeGlyphs;
    impact.fadeGlyphs = visual.fadeGlyphs;
    return impact;
}

void applyBossImpactIfPlayerInside(const BossImpact& impact,
                                   Player& player,
                                   const game::CombatSystem& combatSystem) {
    game::AttackDefinition attack;
    attack.damage = game::DamageInfo(1,
                                     game::DamageType::Contact,
                                     impact.label.empty() ? "Boss attack" : impact.label,
                                     true);
    combatSystem.resolveAttackInCells(player, impact.damageCells, attack);
}

void updateBossImpacts(BossEncounterState& bossEncounter) {
    std::vector<BossImpact> nextImpacts;
    nextImpacts.reserve(bossEncounter.impacts.size());

    for (size_t index = 0; index < bossEncounter.impacts.size(); ++index) {
        BossImpact impact = bossEncounter.impacts[index];
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

    bossEncounter.impacts.swap(nextImpacts);
}

void resolveBossSignal(const std::string& terrainMap,
                       const game::Position& playerPosition,
                       BossEncounterState& bossEncounter,
                       const game::BossAttackSignal& signal,
                       RuntimeState& state,
                       Player& player,
                       const game::CombatSystem& combatSystem) {
    const game::BossAttackVisual attackVisual = bossEncounter.boss.buildResolvedAttackVisual(signal);
    if (attackVisual.activeGlyphs.empty() &&
        attackVisual.fadeGlyphs.empty() &&
        attackVisual.damageCells.empty()) {
        return;
    }

    BossImpact impact = toBossImpact(attackVisual);
    applyBossImpactIfPlayerInside(impact, player, combatSystem);
    bossEncounter.impacts.push_back(impact);

    if (attackVisual.hasLandingPosition &&
        isInsidePlayableArea(terrainMap, attackVisual.landingPosition) &&
        tileAt(terrainMap, attackVisual.landingPosition) == ' ' &&
        !(attackVisual.landingPosition.x == playerPosition.x &&
          attackVisual.landingPosition.y == playerPosition.y)) {
        bossEncounter.boss.setPosition(attackVisual.landingPosition);
    }

    if (signal.type == game::BossAttackType::SweepSlash) {
        state.lastInteraction = bossEncounter.bossName + " released a sweep slash.";
    } else if (signal.type == game::BossAttackType::DashSlash) {
        state.lastInteraction = bossEncounter.bossName + " burst forward with a dash slash.";
    } else if (signal.type == game::BossAttackType::JumpSlash) {
        state.lastInteraction = bossEncounter.bossName + " crashed down with a jump slash.";
    }
}

void initializeBossEncounter(const game::MapDefinition& mapDefinition,
                             const std::string& terrainMap,
                             const game::SaveData& saveData,
                             RuntimeState& state,
                             BossEncounterState& bossEncounter) {
    bossEncounter = BossEncounterState();
    state.bossStatus.clear();

    if (!isBossRoom01(mapDefinition) || isBossDefeated(saveData, kBossRoom01BossId)) {
        if (isBossRoom01(mapDefinition) && isBossDefeated(saveData, kBossRoom01BossId)) {
            state.lastInteraction = "The arena stands silent after the victory.";
        }
        return;
    }

    bossEncounter.active = true;
    bossEncounter.dialogueActive = true;
    bossEncounter.battleActive = false;
    bossEncounter.victoryActive = false;
    bossEncounter.dialogueIndex = 0;
    bossEncounter.rewardHkd = 0;
    bossEncounter.dialogueLines.push_back("So the village sends another soul.");
    bossEncounter.dialogueLines.push_back("Step forward. Let this hall judge your strength.");
    bossEncounter.dialogueLines.push_back("Press ENTER. The duel begins when you accept.");
    bossEncounter.boss = game::MeleeBoss(kBossRoom01BossId, findBossSpawnPosition(terrainMap));
    state.lastInteraction = bossEncounter.bossName + " waits ahead.";
}

void updateBossPrompt(RuntimeState& state, const BossEncounterState& bossEncounter) {
    if (!bossEncounter.active) {
        state.bossStatus.clear();
        return;
    }

    if (bossEncounter.dialogueActive) {
        state.prompt = "ENTER continue dialogue.";
    } else if (bossEncounter.victoryActive) {
        state.prompt = "ENTER close victory screen.";
    } else if (bossEncounter.battleActive) {
        state.prompt = "Defeat the boss. The exit stays sealed until victory.";
    }

    std::ostringstream bossStatus;
    bossStatus << bossEncounter.bossName
               << " HP "
               << bossEncounter.boss.getStats().health.current
               << "/" << bossEncounter.boss.getStats().health.maximum
               << " | State " << bossStateLabel(bossEncounter.boss.getState())
               << " | Stagger " << bossEncounter.boss.getStaggerDamage()
               << "/" << bossEncounter.boss.getStaggerThreshold();
    state.bossStatus = bossStatus.str();
}

std::string buildCollisionMap(const std::string& terrainMap,
                              const game::Position& playerPosition,
                              const game::MapDefinition& mapDefinition,
                              const std::vector<game::GroundEnemy>& groundEnemies,
                              const std::vector<game::FlyingEnemy>& flyingEnemies,
                              const BossEncounterState* bossEncounter) {
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

    if (bossEncounter != 0 &&
        bossEncounter->active &&
        bossEncounter->battleActive &&
        bossEncounter->boss.isAlive()) {
        placeGlyph(collisionMap, bossEncounter->boss.getPosition(), bossEncounter->boss.getRenderGlyph());
    }

    placeGlyph(collisionMap, playerPosition, '@');
    return collisionMap;
}

std::string buildRenderMap(const std::string& terrainMap,
                           const game::Position& playerPosition,
                           const game::MapDefinition& mapDefinition,
                           const game::SaveData& saveData,
                           const std::vector<game::GroundEnemy>& groundEnemies,
                           const std::vector<game::FlyingEnemy>& flyingEnemies,
                           const std::vector<EnemyProjectile>& enemyProjectiles,
                           const BossEncounterState* bossEncounter) {
    std::string renderMap = terrainMap;

    for (size_t index = 0; index < mapDefinition.transitions.size(); ++index) {
        placeGlyph(renderMap,
                   mapDefinition.transitions[index].triggerPosition,
                   getTransitionGlyph(saveData, mapDefinition.transitions[index]));
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

    if (bossEncounter != 0) {
        for (size_t index = 0; index < bossEncounter->impacts.size(); ++index) {
            const std::vector<game::BossVisualGlyph>& glyphs = bossEncounter->impacts[index].activeFramesRemaining > 0
                    ? bossEncounter->impacts[index].activeGlyphs
                    : bossEncounter->impacts[index].fadeGlyphs;
            for (size_t glyphIndex = 0; glyphIndex < glyphs.size(); ++glyphIndex) {
                placeGlyph(renderMap, glyphs[glyphIndex].position, glyphs[glyphIndex].glyph);
            }
        }

        if (bossEncounter->active && bossEncounter->boss.isRenderable()) {
            const std::vector<game::BossVisualGlyph> bodyVisual = bossEncounter->boss.buildBodyVisual();
            for (size_t index = 0; index < bodyVisual.size(); ++index) {
                placeGlyph(renderMap, bodyVisual[index].position, bodyVisual[index].glyph);
            }

            if (bossEncounter->boss.getState() == game::BossState::AttackStartup) {
                const std::vector<game::BossVisualGlyph> startupVisual = bossEncounter->boss.buildStartupVisual();
                for (size_t index = 0; index < startupVisual.size(); ++index) {
                    placeGlyph(renderMap, startupVisual[index].position, startupVisual[index].glyph);
                }
            }
        }
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
                  const game::SaveData& saveData,
                  const game::Position& playerPosition,
                  RuntimeState& state) {
    if (state.pauseConfirmOpen) {
        state.prompt = "Paused. W/S select, ENTER confirm, ESC back.";
        return;
    }

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
        state.prompt = buildTransitionPrompt(saveData, *transition);
        return;
    }

    state.prompt = kDefaultRuntimePrompt;
}

std::string buildBottomFooter(const RuntimeState& state, const game::SaveData& saveData) {
    std::ostringstream footer;
    footer << "\n";
    footer << "A/D Move  SPACE Jump  " << runtimeDashKeyLabel() << " Dash  E Interact";
    if (hasUnlockedSkill(saveData, game::SkillId::DoubleJump)) {
        footer << "  SPACE Double Jump";
    }
    footer << "\n";
    footer << "J Slash  U Up Slash  N Down Slash";
    footer << "\n";
    footer << "K Spell  I Up Spell  M Down Spell  R Heal  P Reset  ESC Pause";
    if (state.prompt != kDefaultRuntimePrompt) {
        footer << "\n" << state.prompt;
    }
    if (!state.lastInteraction.empty()) {
        footer << "\n" << state.lastInteraction;
    }
    if (!state.bossStatus.empty()) {
        footer << "\n" << state.bossStatus;
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
    state.shopOffers = filterShopOffersForSave(result.offers, saveData);
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
    if (result.unlockedSkillGranted) {
        unlockSkill(saveData, result.grantedSkill);
        applyPersistentSkillsToPlayer(player, saveData);
    }
    player.restoreSavedStats(stats);
    applyPersistentSkillsToPlayer(player, saveData);
    saveData.playerStats = stats;
    state.shopOffers = filterShopOffersForSave(result.offers, saveData);
    if (state.shopSelection >= static_cast<int>(state.shopOffers.size())) {
        state.shopSelection = std::max(0, static_cast<int>(state.shopOffers.size()) - 1);
    }
    state.lastInteraction = result.speaker + ": " + result.text;
}

void resolveBossVictory(Player& player,
                        RuntimeState& state,
                        game::SaveData& saveData,
                        game::GameSession& gameSession,
                        game::SaveSystem& saveSystem,
                        BossEncounterState& bossEncounter,
                        const game::CombatSystem& combatSystem) {
    game::RewardResolution reward;
    if (!bossEncounter.boss.consumeDefeatReward(reward)) {
        reward = combatSystem.buildEnemyReward(bossEncounter.boss);
    }

    game::CharacterStats rewardedStats = player.getStats();
    combatSystem.applyReward(rewardedStats, reward);
    player.restoreSavedStats(rewardedStats);
    applyPersistentSkillsToPlayer(player, saveData);

    syncSaveDataWithRuntime(player, state, gameSession, saveData);
    gameSession.setActiveSave(saveData);
    gameSession.markBossDefeated(bossEncounter.bossId);
    saveData = gameSession.getActiveSave();
    saveSystem.saveOnMapEntry(saveData);

    bossEncounter.battleActive = false;
    bossEncounter.victoryActive = true;
    bossEncounter.rewardHkd = reward.hkdGranted;
    state.lastInteraction = bossEncounter.bossName + " fell. The chamber is yours.";
}

void tryResolvePlayerHitOnBoss(const game::Position& playerPosition,
                               const game::CharacterStats& statsBeforeCombat,
                               Player& player,
                               RuntimeState& state,
                               game::SaveData& saveData,
                               game::GameSession& gameSession,
                               game::SaveSystem& saveSystem,
                               const game::CombatSystem& combatSystem,
                               const KeyStateManager& keyStateManager,
                               BossEncounterState& bossEncounter) {
    if (!bossEncounter.active ||
        !bossEncounter.battleActive ||
        !bossEncounter.boss.isAlive() ||
        bossEncounter.victoryActive ||
        playerPosition.x < 0 ||
        playerPosition.y < 0) {
        return;
    }

    const bool aimingUp = isKeyDown(keyStateManager, 'w') || isKeyDown(keyStateManager, 'W');
    const bool aimingDown = isKeyDown(keyStateManager, 's') || isKeyDown(keyStateManager, 'S');
    const bool physicalPressed = isJustPressed(keyStateManager, state.previousKeys, 'j') ||
                                 isJustPressed(keyStateManager, state.previousKeys, 'J');
    const bool spellPressed = isJustPressed(keyStateManager, state.previousKeys, 'k') ||
                              isJustPressed(keyStateManager, state.previousKeys, 'K');
    game::AttackDefinition attack;
    game::DamageResolution resolution;

    if (physicalPressed) {
        attack.damage = game::DamageInfo(1,
                                         aimingUp ? game::DamageType::UpSlash
                                                  : (aimingDown ? game::DamageType::DownSlash
                                                                : game::DamageType::BasicAttack),
                                         "player",
                                         true);
        attack.soulGainOnHit = (!aimingUp && !aimingDown) ? 11 : 0;

        if (aimingUp || aimingDown) {
            resolution = combatSystem.resolveAttackInVerticalRange(
                    player,
                    bossEncounter.boss,
                    attack,
                    aimingUp,
                    1,
                    1,
                    2);
        } else {
            resolution = combatSystem.resolveAttackInFrontRange(
                    player,
                    bossEncounter.boss,
                    attack,
                    kBossMeleeRange,
                    1);
        }
    } else if (spellPressed && player.getStats().soul.current < statsBeforeCombat.soul.current) {
        attack.damage = game::DamageInfo(1,
                                         aimingUp ? game::DamageType::SoulWaveUp
                                                  : (aimingDown ? game::DamageType::SoulSlam
                                                                : game::DamageType::SoulWaveHorizontal),
                                         "player",
                                         false);

        if (aimingUp || aimingDown) {
            resolution = combatSystem.resolveAttackInVerticalRange(
                    player,
                    bossEncounter.boss,
                    attack,
                    aimingUp,
                    1,
                    1,
                    kBossVerticalSpellRange);
        } else {
            resolution = combatSystem.resolveAttackInFrontRange(
                    player,
                    bossEncounter.boss,
                    attack,
                    kBossHorizontalWaveRange,
                    1);
        }
    }

    if (!resolution.hitApplied) {
        return;
    }

    if (resolution.targetDefeated) {
        resolveBossVictory(player, state, saveData, gameSession, saveSystem, bossEncounter, combatSystem);
        return;
    }

    if (bossEncounter.boss.shouldEnterStagger()) {
        state.lastInteraction = bossEncounter.bossName + " stagger threshold reached.";
    } else {
        state.lastInteraction = bossEncounter.bossName + " took " + std::to_string(resolution.damageApplied) + " damage.";
    }
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
                            std::vector<EnemyProjectile>& enemyProjectiles,
                            Player& player,
                            const game::CombatSystem& combatSystem) {
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

        game::AttackDefinition attack;
        attack.damage = game::DamageInfo(1, game::DamageType::Fireball, "Flying fireball", false);
        if (combatSystem.resolveAttackAtPosition(player, projectile.position, attack).hitApplied) {
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
    BossEncounterState bossEncounter;
    game::CombatSystem combatSystem;
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

    applyPersistentSkillsToPlayer(player, saveData);
    player.restoreSavedStats(saveData.playerStats);
    applyPersistentSkillsToPlayer(player, saveData);
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
    initializeBossEncounter(currentMapDefinition, terrainMap, saveData, state, bossEncounter);
    syncSaveDataWithRuntime(player, state, gameSession, saveData);
    gameSession.setActiveSave(saveData);
    saveSystem.saveOnMapEntry(saveData);

    while (true) {
        keyStateManager.clearKeys();
        keyStateManager.readKeys();

        if (isJustPressed(keyStateManager, state.previousKeys, 0x1B)) {
            if (state.pauseConfirmOpen) {
                state.pauseConfirmOpen = false;
                state.pauseQuitSelected = false;
                state.lastInteraction = "Resumed gameplay.";
            } else {
                state.pauseConfirmOpen = true;
                state.pauseQuitSelected = false;
                state.lastInteraction = "Pause menu opened.";
            }
        }

        if (state.pauseConfirmOpen) {
            if (isJustPressed(keyStateManager, state.previousKeys, 'w') ||
                isJustPressed(keyStateManager, state.previousKeys, 'W') ||
                isJustPressed(keyStateManager, state.previousKeys, 's') ||
                isJustPressed(keyStateManager, state.previousKeys, 'S')) {
                state.pauseQuitSelected = !state.pauseQuitSelected;
            }

            if (isJustPressed(keyStateManager, state.previousKeys, 0x0D)) {
                if (state.pauseQuitSelected) {
                    break;
                }
                state.pauseConfirmOpen = false;
                state.pauseQuitSelected = false;
                state.lastInteraction = "Resumed gameplay.";
            }
        }

        if (bossEncounter.active &&
            bossEncounter.dialogueActive &&
            !state.pauseConfirmOpen &&
            isJustPressed(keyStateManager, state.previousKeys, 0x0D)) {
            bossEncounter.dialogueIndex++;
            if (bossEncounter.dialogueIndex >= static_cast<int>(bossEncounter.dialogueLines.size())) {
                bossEncounter.dialogueActive = false;
                bossEncounter.battleActive = true;
                state.lastInteraction = bossEncounter.bossName + ": Then come. Let the chamber decide.";
            }
        } else if (bossEncounter.active &&
                   bossEncounter.victoryActive &&
                   !state.pauseConfirmOpen &&
                   isJustPressed(keyStateManager, state.previousKeys, 0x0D)) {
            bossEncounter.victoryActive = false;
            bossEncounter.active = false;
            state.lastInteraction = "Victory recorded. The exit is open again.";
            state.bossStatus.clear();
            gameSession.resumeGameplay();
        }

        if (!state.pauseConfirmOpen &&
            (isJustPressed(keyStateManager, state.previousKeys, 'p') ||
             isJustPressed(keyStateManager, state.previousKeys, 'P'))) {
            player.restoreSavedStats(saveData.playerStats);
            applyPersistentSkillsToPlayer(player, saveData);
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
            initializeBossEncounter(currentMapDefinition, terrainMap, saveData, state, bossEncounter);
            state.lastInteraction = "Room reset to current save.";
        }

        updatePrompt(currentMapDefinition, saveData, playerPosition, state);
        updateBossPrompt(state, bossEncounter);

        if (!state.pauseConfirmOpen && state.shopOpen && !state.shopOffers.empty()) {
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

        if (!state.pauseConfirmOpen &&
            state.shopOpen &&
            (isJustPressed(keyStateManager, state.previousKeys, 'j') ||
             isJustPressed(keyStateManager, state.previousKeys, 'J'))) {
            applyShopPurchase(player, state, saveData);
            syncSaveDataWithRuntime(player, state, gameSession, saveData);
            gameSession.setActiveSave(saveData);
            saveSystem.saveOnMapEntry(saveData);
        }

        if (!state.pauseConfirmOpen &&
            (isJustPressed(keyStateManager, state.previousKeys, 'e') ||
             isJustPressed(keyStateManager, state.previousKeys, 'E'))) {
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
                        if (isBossRoom01(currentMapDefinition) &&
                            bossEncounter.active &&
                            !bossEncounter.victoryActive) {
                            state.lastInteraction = "A dark seal bars the exit until the boss falls.";
                            state.previousKeys = keyStateManager.keyStates;
                            std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
                            continue;
                        }

                        const game::MapTransition transitionData = *transition;
                        const std::string shortcutId = getShortcutDoorId(transitionData);
                        const bool lockedFromVillage = isLockedVillageShortcut(saveData, transitionData);
                        const bool unlockFromRemote = isRemoteUnlockShortcut(saveData, transitionData);

                        if (lockedFromVillage) {
                            state.lastInteraction = "This shortcut is locked from the village side.";
                            state.previousKeys = keyStateManager.keyStates;
                            std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
                            continue;
                        }

                        if (unlockFromRemote && gameSession.unlockShortcut(shortcutId)) {
                            saveData.unlockedShortcutIds = gameSession.getActiveSave().unlockedShortcutIds;
                        }

                        currentMapDefinition = mapLoader.loadMap(transitionData.toMapId);
                        loadMapState(currentMapDefinition,
                                     transitionData.spawnPointId,
                                     state,
                                     saveData,
                                     terrainMap,
                                     playerPosition,
                                     groundEnemies,
                                     flyingEnemies,
                                     enemyProjectiles);
                        initializeBossEncounter(currentMapDefinition, terrainMap, saveData, state, bossEncounter);
                        syncSaveDataWithRuntime(player, state, gameSession, saveData);
                        gameSession.setActiveSave(saveData);
                        saveSystem.saveOnMapEntry(saveData);
                        state.lastInteraction = unlockFromRemote
                                ? "Unlocked shortcut and entered " + currentMapDefinition.displayName + "."
                                : "Entered " + currentMapDefinition.displayName + ".";
                        state.previousKeys = keyStateManager.keyStates;
                        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
                        continue;
                    }
                }
            }
        }

        const bool bossSequenceBlockingGameplay =
                state.pauseConfirmOpen || bossEncounter.dialogueActive || bossEncounter.victoryActive;

        if (!state.shopOpen && !bossSequenceBlockingGameplay) {
            std::string collisionMap = buildCollisionMap(terrainMap,
                                                         playerPosition,
                                                         currentMapDefinition,
                                                         groundEnemies,
                                                         flyingEnemies,
                                                         &bossEncounter);
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
        player.setCombatPosition(playerPosition);

        const int activeGroundEnemyIndex = findClosestGroundEnemyIndex(groundEnemies, playerPosition);
        const int activeFlyingEnemyIndex = findClosestFlyingEnemyIndex(flyingEnemies, playerPosition);
        const game::CharacterStats statsBeforeCombat = player.getStats();

        if (!bossSequenceBlockingGameplay && activeGroundEnemyIndex >= 0 && activeFlyingEnemyIndex >= 0) {
            player.updateCombat(gameplayMap,
                                groundEnemies[static_cast<size_t>(activeGroundEnemyIndex)],
                                flyingEnemies[static_cast<size_t>(activeFlyingEnemyIndex)]);
        } else if (!bossSequenceBlockingGameplay && activeGroundEnemyIndex >= 0) {
            player.updateCombat(gameplayMap,
                                groundEnemies[static_cast<size_t>(activeGroundEnemyIndex)],
                                dummyFlyingEnemy);
        } else if (!bossSequenceBlockingGameplay && activeFlyingEnemyIndex >= 0) {
            player.updateCombat(gameplayMap,
                                dummyGroundEnemy,
                                flyingEnemies[static_cast<size_t>(activeFlyingEnemyIndex)]);
        } else if (!bossSequenceBlockingGameplay) {
            player.updateCombat(gameplayMap, dummyGroundEnemy, dummyFlyingEnemy);
        }

        if (!bossSequenceBlockingGameplay) {
            tryResolvePlayerHitOnBoss(playerPosition,
                                      statsBeforeCombat,
                                      player,
                                      state,
                                      saveData,
                                      gameSession,
                                      saveSystem,
                                      combatSystem,
                                      keyStateManager,
                                      bossEncounter);
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
                game::AttackDefinition attack = groundEnemies[index].getPrimaryAttack();
                attack.damage.sourceId = "Ground enemy";
                combatSystem.resolveAttackInRange(player,
                                                  groundEnemies[index].getPosition(),
                                                  attack,
                                                  attack.horizontalReach,
                                                  attack.verticalTolerance);
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

        updateEnemyProjectiles(terrainMap, enemyProjectiles, player, combatSystem);
        removeDefeatedEnemies(groundEnemies, flyingEnemies);
        updateBossImpacts(bossEncounter);
        if (bossEncounter.active && bossEncounter.battleActive && !bossEncounter.victoryActive) {
            const game::Position previousBossPosition = bossEncounter.boss.getPosition();
            bossEncounter.boss.updateAI(playerPosition, static_cast<float>(kFrameMs) / 1000.0f);
            const game::Position nextBossPosition = bossEncounter.boss.getPosition();
            if (bossEncounter.boss.isAlive() &&
                (nextBossPosition.x != previousBossPosition.x || nextBossPosition.y != previousBossPosition.y) &&
                !canBossOccupy(terrainMap, playerPosition, nextBossPosition)) {
                bossEncounter.boss.setPosition(previousBossPosition);
            }

            game::BossAttackSignal signal;
            if (bossEncounter.boss.consumeAttackSignal(signal)) {
                resolveBossSignal(terrainMap, playerPosition, bossEncounter, signal, state, player, combatSystem);
            }
        }
        gameSession.update(static_cast<float>(kFrameMs) / 1000.0f);
        syncSaveDataWithRuntime(player, state, gameSession, saveData);
        gameSession.setActiveSave(saveData);

        std::string renderMap = buildRenderMap(terrainMap,
                                               playerPosition,
                                               currentMapDefinition,
                                               saveData,
                                               groundEnemies,
                                               flyingEnemies,
                                               enemyProjectiles,
                                               &bossEncounter);
        player.overlayRender(renderMap, gameplayMap);
        if (bossEncounter.dialogueActive) {
            std::vector<std::string> dialogueLines;
            dialogueLines.push_back(bossEncounter.bossName);
            dialogueLines.push_back(bossEncounter.dialogueLines[static_cast<size_t>(bossEncounter.dialogueIndex)]);
            dialogueLines.push_back("ENTER");
            overlayCenteredLines(renderMap, 4, dialogueLines);
        } else if (bossEncounter.victoryActive) {
            std::vector<std::string> victoryLines;
            victoryLines.push_back("VICTORY");
            victoryLines.push_back("Defeated " + bossEncounter.bossName);
            victoryLines.push_back("Reward +" + std::to_string(bossEncounter.rewardHkd) + " HKD");
            victoryLines.push_back("ENTER");
            overlayCenteredLines(renderMap, 4, victoryLines);
        } else if (state.pauseConfirmOpen) {
            overlayCenteredLines(renderMap, 4, buildPauseOverlayLines(state.pauseQuitSelected));
        }
        mapDrawer.currentmap = player.buildHud(currentMapDefinition.displayName) +
                               renderMap +
                               buildBottomFooter(state, saveData);
        mapDrawer.draw();

        if (player.consumeResetRequest()) {
            gameSession.markPlayerDead();
            saveData = saveSystem.restoreAfterDeath(gameSession.getActiveSave());
            gameSession.setActiveSave(saveData);
            player.restoreSavedStats(saveData.playerStats);
            applyPersistentSkillsToPlayer(player, saveData);
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
            initializeBossEncounter(currentMapDefinition, terrainMap, saveData, state, bossEncounter);
            state.lastInteraction = "Respawned at current save point.";
        }

        state.previousKeys = keyStateManager.keyStates;
        std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs));
    }

    return 0;
}
