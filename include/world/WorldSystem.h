#ifndef TESTCPP1_WORLDSYSTEM_H
#define TESTCPP1_WORLDSYSTEM_H

#include <string>
#include <vector>

#include "shared/GameTypes.h"

namespace game {

struct SpawnPoint {
    std::string id;
    Position position;
};

struct EnemySpawn {
    std::string enemyTemplateId;
    Position position;
};

struct NpcPlacement {
    std::string npcId;
    Position position;
};

struct MapDefinition {
    std::string id;
    std::string displayName;
    MapType type = MapType::CombatRoom;
    std::vector<SpawnPoint> spawnPoints;
    std::vector<EnemySpawn> enemySpawns;
    std::vector<NpcPlacement> npcPlacements;
    std::vector<MapTransition> transitions;
    std::vector<std::string> terrainRows;
    bool isBossRoom = false;
};

struct MapRuntimeState {
    std::string currentMapId;
    std::string respawnMapId;
    std::vector<std::string> defeatedEnemyIds;
    std::vector<std::string> defeatedBossIds;
};

class MapLoader {
public:
    MapDefinition loadMap(const std::string& mapId) const;
    std::vector<MapDefinition> loadAllMaps() const;
};

class WorldSystem {
public:
    bool enterMap(const std::string& mapId, const std::string& spawnPointId);
    void recordEnemyDefeat(const std::string& enemyId);
    void recordBossDefeat(const std::string& bossId);
    const MapRuntimeState& getRuntimeState() const;

private:
    MapRuntimeState runtimeState;
};

} // namespace game

#endif // TESTCPP1_WORLDSYSTEM_H
