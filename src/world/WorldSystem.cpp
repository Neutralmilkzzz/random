#include "world/WorldSystem.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* kMapRepositoryRoot = "data/maps/";
const char* kMapIndexFile = "data/maps/index.txt";

std::string trim(const std::string& input) {
    const std::string whitespace = " \t\r\n";
    const std::string::size_type start = input.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return std::string();
    }

    const std::string::size_type end = input.find_last_not_of(whitespace);
    return input.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::string current;

    for (std::string::size_type index = 0; index < input.length(); ++index) {
        if (input[index] == delimiter) {
            tokens.push_back(trim(current));
            current.clear();
        } else {
            current.push_back(input[index]);
        }
    }

    tokens.push_back(trim(current));
    return tokens;
}

game::MapType parseMapType(const std::string& value) {
    if (value == "SpawnVillage") {
        return game::MapType::SpawnVillage;
    }
    if (value == "CombatRoom") {
        return game::MapType::CombatRoom;
    }
    if (value == "BossRoom") {
        return game::MapType::BossRoom;
    }
    if (value == "ShopRoom") {
        return game::MapType::ShopRoom;
    }
    if (value == "EventRoom") {
        return game::MapType::EventRoom;
    }

    throw std::runtime_error("Unknown map type: " + value);
}

bool parseBool(const std::string& value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }

    throw std::runtime_error("Unknown boolean value: " + value);
}

int parseInt(const std::string& value) {
    return std::atoi(value.c_str());
}

bool containsString(const std::vector<std::string>& values, const std::string& target) {
    return std::find(values.begin(), values.end(), target) != values.end();
}

} // namespace

namespace game {

MapDefinition MapLoader::loadMap(const std::string& mapId) const {
    const std::string path = std::string(kMapRepositoryRoot) + mapId + ".map";
    std::ifstream input(path.c_str());
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open map file: " + path);
    }

    enum class Section {
        None,
        SpawnPoints,
        EnemySpawns,
        NpcPlacements,
        Transitions,
        Terrain
    };

    MapDefinition definition;
    Section currentSection = Section::None;
    std::string line;

    while (std::getline(input, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }

        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line[0] == '[' && line[line.length() - 1] == ']') {
            const std::string sectionName = trim(line.substr(1, line.length() - 2));
            if (sectionName == "spawn_points") {
                currentSection = Section::SpawnPoints;
            } else if (sectionName == "enemy_spawns") {
                currentSection = Section::EnemySpawns;
            } else if (sectionName == "npc_placements") {
                currentSection = Section::NpcPlacements;
            } else if (sectionName == "transitions") {
                currentSection = Section::Transitions;
            } else if (sectionName == "terrain") {
                currentSection = Section::Terrain;
            } else {
                throw std::runtime_error("Unknown map section: " + sectionName);
            }
            continue;
        }

        if (currentSection == Section::Terrain) {
            definition.terrainRows.push_back(line);
            continue;
        }

        if (currentSection == Section::SpawnPoints) {
            const std::vector<std::string> parts = split(line, ',');
            if (parts.size() != 3) {
                throw std::runtime_error("Invalid spawn point row in map: " + mapId);
            }
            definition.spawnPoints.push_back(SpawnPoint{parts[0], Position(parseInt(parts[1]), parseInt(parts[2]))});
            continue;
        }

        if (currentSection == Section::EnemySpawns) {
            const std::vector<std::string> parts = split(line, ',');
            if (parts.size() != 3) {
                throw std::runtime_error("Invalid enemy spawn row in map: " + mapId);
            }
            definition.enemySpawns.push_back(EnemySpawn{parts[0], Position(parseInt(parts[1]), parseInt(parts[2]))});
            continue;
        }

        if (currentSection == Section::NpcPlacements) {
            const std::vector<std::string> parts = split(line, ',');
            if (parts.size() != 3) {
                throw std::runtime_error("Invalid npc placement row in map: " + mapId);
            }
            definition.npcPlacements.push_back(NpcPlacement{parts[0], Position(parseInt(parts[1]), parseInt(parts[2]))});
            continue;
        }

        if (currentSection == Section::Transitions) {
            const std::vector<std::string> parts = split(line, ',');
            if (parts.size() != 3 && parts.size() != 5) {
                throw std::runtime_error("Invalid transition row in map: " + mapId);
            }
            MapTransition transition;
            transition.fromMapId = parts[0];
            transition.toMapId = parts[1];
            transition.spawnPointId = parts[2];
            transition.triggerPosition = parts.size() == 5
                    ? Position(parseInt(parts[3]), parseInt(parts[4]))
                    : Position(-1, -1);
            definition.transitions.push_back(transition);
            continue;
        }

        const std::string::size_type separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("Invalid map metadata row in map: " + mapId);
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        if (key == "id") {
            definition.id = value;
        } else if (key == "display_name") {
            definition.displayName = value;
        } else if (key == "type") {
            definition.type = parseMapType(value);
        } else if (key == "boss_room") {
            definition.isBossRoom = parseBool(value);
        } else {
            throw std::runtime_error("Unknown map metadata key: " + key);
        }
    }

    if (definition.id.empty()) {
        throw std::runtime_error("Map file is missing id: " + path);
    }

    return definition;
}

std::vector<MapDefinition> MapLoader::loadAllMaps() const {
    std::ifstream input(kMapIndexFile);
    if (!input.is_open()) {
        throw std::runtime_error(std::string("Failed to open map index: ") + kMapIndexFile);
    }

    std::vector<MapDefinition> maps;
    std::string line;
    while (std::getline(input, line)) {
        const std::string mapId = trim(line);
        if (mapId.empty() || mapId[0] == '#') {
            continue;
        }
        maps.push_back(loadMap(mapId));
    }

    return maps;
}

bool WorldSystem::enterMap(const std::string& mapId, const std::string& spawnPointId) {
    MapLoader loader;
    const MapDefinition mapDefinition = loader.loadMap(mapId);

    if (!spawnPointId.empty()) {
        bool foundSpawnPoint = false;
        for (std::vector<SpawnPoint>::const_iterator it = mapDefinition.spawnPoints.begin();
             it != mapDefinition.spawnPoints.end();
             ++it) {
            if (it->id == spawnPointId) {
                foundSpawnPoint = true;
                break;
            }
        }

        if (!foundSpawnPoint) {
            return false;
        }
    }

    runtimeState.currentMapId = mapDefinition.id;
    runtimeState.respawnMapId = spawnPointId.empty()
            ? (mapDefinition.spawnPoints.empty() ? std::string() : mapDefinition.spawnPoints.front().id)
            : spawnPointId;
    return true;
}

void WorldSystem::recordEnemyDefeat(const std::string& enemyId) {
    if (!containsString(runtimeState.defeatedEnemyIds, enemyId)) {
        runtimeState.defeatedEnemyIds.push_back(enemyId);
    }
}

void WorldSystem::recordBossDefeat(const std::string& bossId) {
    if (!containsString(runtimeState.defeatedBossIds, bossId)) {
        runtimeState.defeatedBossIds.push_back(bossId);
    }
}

const MapRuntimeState& WorldSystem::getRuntimeState() const {
    return runtimeState;
}

} // namespace game
