#include "save/SaveSystem.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "combat/CombatTuning.h"

namespace {

const char* kSaveFilePath = "data/save_slot_01.sav";

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
    std::vector<std::string> parts;
    std::string current;
    for (std::string::size_type index = 0; index < input.size(); ++index) {
        if (input[index] == delimiter) {
            parts.push_back(trim(current));
            current.clear();
        } else {
            current.push_back(input[index]);
        }
    }
    parts.push_back(trim(current));
    return parts;
}

int toInt(const std::string& value) {
    std::istringstream stream(value);
    int parsed = 0;
    stream >> parsed;
    return parsed;
}

std::string join(const std::vector<std::string>& values) {
    std::ostringstream stream;
    for (size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            stream << ",";
        }
        stream << values[index];
    }
    return stream.str();
}

int difficultyToInt(game::Difficulty difficulty) {
    switch (difficulty) {
    case game::Difficulty::Easy:
        return 0;
    case game::Difficulty::Normal:
        return 1;
    case game::Difficulty::Hard:
        return 2;
    }

    return 1;
}

game::Difficulty intToDifficulty(int value) {
    if (value <= 0) {
        return game::Difficulty::Easy;
    }
    if (value >= 2) {
        return game::Difficulty::Hard;
    }
    return game::Difficulty::Normal;
}

void writeSaveFile(const game::SaveData& saveData) {
    std::ofstream output(kSaveFilePath, std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error(std::string("Failed to open save file for writing: ") + kSaveFilePath);
    }

    output << "difficulty=" << difficultyToInt(saveData.difficulty) << "\n";
    output << "current_map=" << saveData.currentMapId << "\n";
    output << "respawn_point=" << saveData.respawnMapId << "\n";
    output << "health_current=" << saveData.playerStats.health.current << "\n";
    output << "health_max=" << saveData.playerStats.health.maximum << "\n";
    output << "soul_current=" << saveData.playerStats.soul.current << "\n";
    output << "soul_max=" << saveData.playerStats.soul.maximum << "\n";
    output << "hkd=" << saveData.playerStats.hkd << "\n";
    output << "attack_power=" << game::clampPlayerAttackPowerLevel(saveData.playerStats.attackPower) << "\n";
    output << "attack_speed_level=" << saveData.playerStats.attackSpeedLevel << "\n";
    output << "purchased_health_slots=" << saveData.playerStats.purchasedHealthSlots << "\n";
    output << "completion_time=" << saveData.completionTimeSeconds << "\n";
    output << "has_active_run=" << (saveData.hasActiveRun ? 1 : 0) << "\n";
    output << "defeated_bosses=" << join(saveData.defeatedBossIds) << "\n";
    output << "unlocked_shortcuts=" << join(saveData.unlockedShortcutIds) << "\n";
    output << "unlocked_skills=" << join(saveData.unlockedSkillIds) << "\n";
}

} // namespace

namespace game {

SaveData SaveSystem::createNewSave(Difficulty difficulty) const {
    SaveData saveData;
    saveData.difficulty = difficulty;
    saveData.playerStats = CharacterStats();
    saveData.currentMapId = "spawn_village";
    saveData.respawnMapId = "player_start";
    saveData.defeatedBossIds.clear();
    saveData.unlockedShortcutIds.clear();
    saveData.unlockedSkillIds.clear();
    saveData.completionTimeSeconds = 0;
    saveData.hasActiveRun = true;
    return saveData;
}

bool SaveSystem::hasSave() const {
    std::ifstream input(kSaveFilePath);
    return input.is_open();
}

SaveData SaveSystem::load() const {
    std::ifstream input(kSaveFilePath);
    if (!input.is_open()) {
        throw std::runtime_error(std::string("Failed to open save file: ") + kSaveFilePath);
    }

    SaveData saveData = createNewSave(Difficulty::Normal);
    saveData.hasActiveRun = false;

    std::string line;
    while (std::getline(input, line)) {
        const std::string::size_type separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        if (key == "difficulty") {
            saveData.difficulty = intToDifficulty(toInt(value));
        } else if (key == "current_map") {
            saveData.currentMapId = value;
        } else if (key == "respawn_point") {
            saveData.respawnMapId = value;
        } else if (key == "health_current") {
            saveData.playerStats.health.current = toInt(value);
        } else if (key == "health_max") {
            saveData.playerStats.health.maximum = toInt(value);
        } else if (key == "soul_current") {
            saveData.playerStats.soul.current = toInt(value);
        } else if (key == "soul_max") {
            saveData.playerStats.soul.maximum = toInt(value);
        } else if (key == "hkd") {
            saveData.playerStats.hkd = toInt(value);
        } else if (key == "attack_power") {
            saveData.playerStats.attackPower = clampPlayerAttackPowerLevel(toInt(value));
        } else if (key == "attack_speed_level") {
            saveData.playerStats.attackSpeedLevel = toInt(value);
        } else if (key == "purchased_health_slots") {
            saveData.playerStats.purchasedHealthSlots = toInt(value);
        } else if (key == "completion_time") {
            saveData.completionTimeSeconds = toInt(value);
        } else if (key == "has_active_run") {
            saveData.hasActiveRun = toInt(value) != 0;
        } else if (key == "defeated_bosses") {
            saveData.defeatedBossIds.clear();
            if (!value.empty()) {
                saveData.defeatedBossIds = split(value, ',');
            }
        } else if (key == "unlocked_shortcuts") {
            saveData.unlockedShortcutIds.clear();
            if (!value.empty()) {
                saveData.unlockedShortcutIds = split(value, ',');
            }
        } else if (key == "unlocked_skills") {
            saveData.unlockedSkillIds.clear();
            if (!value.empty()) {
                saveData.unlockedSkillIds = split(value, ',');
            }
        }
    }

    clampPlayerAttackPower(saveData.playerStats);

    return saveData;
}

void SaveSystem::saveOnMapEntry(const SaveData& saveData) {
    writeSaveFile(saveData);
}

SaveData SaveSystem::restoreAfterDeath(const SaveData& saveData) const {
    if (hasSave()) {
        return load();
    }
    return saveData;
}

void SaveSystem::clear() const {
    std::remove(kSaveFilePath);
}

} // namespace game
