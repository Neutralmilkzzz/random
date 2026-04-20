#include <iostream>

#include "save/SaveSystem.h"
#include "world/WorldSystem.h"

int main() {
    game::MapDefinition spawnVillage;
    spawnVillage.id = "spawn_village";
    spawnVillage.displayName = "Spawn Village";
    spawnVillage.type = game::MapType::SpawnVillage;
    spawnVillage.spawnPoints.push_back(game::SpawnPoint{"village_entry", game::Position(2, 10)});
    spawnVillage.npcPlacements.push_back(game::NpcPlacement{"village_chief", game::Position(5, 10)});
    spawnVillage.transitions.push_back(game::MapTransition{"spawn_village", "boss_room_melee", "boss_gate"});

    game::SaveData saveData;
    saveData.currentMapId = spawnVillage.id;
    saveData.respawnMapId = "village_entry";
    saveData.hasActiveRun = true;

    std::cout << "[WorldSandbox] map/save scaffold\n";
    std::cout << "Map: " << spawnVillage.displayName << "\n";
    std::cout << "Spawn points: " << spawnVillage.spawnPoints.size() << "\n";
    std::cout << "NPC count: " << spawnVillage.npcPlacements.size() << "\n";
    std::cout << "Transition count: " << spawnVillage.transitions.size() << "\n";
    std::cout << "Save current map: " << saveData.currentMapId << "\n";
    std::cout << "\nNext step: replace hardcoded data with real map loading.\n";

    return 0;
}
