#ifndef TESTCPP1_SAVESYSTEM_H
#define TESTCPP1_SAVESYSTEM_H

#include <string>
#include <vector>

#include "shared/GameTypes.h"

namespace game {

struct SaveData {
    Difficulty difficulty = Difficulty::Normal;
    CharacterStats playerStats;
    std::string currentMapId;
    std::string respawnMapId;
    std::vector<std::string> defeatedBossIds;
    std::vector<std::string> unlockedShortcutIds;
    std::vector<std::string> unlockedSkillIds;
    int completionTimeSeconds = 0;
    bool hasActiveRun = false;
};

class SavePoint {
public:
    virtual ~SavePoint() = default;

    virtual const std::string& getId() const = 0;
    virtual Position getPosition() const = 0;
    virtual void capture(const SaveData& saveData) = 0;
};

class SaveSystem {
public:
    SaveData createNewSave(Difficulty difficulty) const;
    bool hasSave() const;
    SaveData load() const;
    void saveOnMapEntry(const SaveData& saveData);
    SaveData restoreAfterDeath(const SaveData& saveData) const;
    void clear() const;
};

} // namespace game

#endif // TESTCPP1_SAVESYSTEM_H
