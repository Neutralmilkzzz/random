#ifndef TESTCPP1_GAMESESSION_H
#define TESTCPP1_GAMESESSION_H

#include <string>

#include "save/SaveSystem.h"

namespace game {

enum class GameFlowState {
    MainMenu,
    DifficultySelect,
    SaveSelect,
    InGame,
    Dead,
    Victory,
    Paused
};

struct RunStatistics {
    int totalPlaySeconds = 0;
    int enemiesDefeated = 0;
    int bossesDefeated = 0;
};

class GameSession {
public:
    bool startNewRun(Difficulty difficulty);
    bool continueRun(const SaveData& saveData);
    void update(float deltaSeconds);
    void markPlayerDead();
    void markBossDefeated(const std::string& bossId);
    bool hasDefeatedBoss(const std::string& bossId) const;
    void resumeGameplay();
    GameFlowState getFlowState() const;
    const RunStatistics& getRunStatistics() const;
    const SaveData& getActiveSave() const;
    void setActiveSave(const SaveData& saveData);
    bool hasSaveData() const;
    bool isShortcutUnlocked(const std::string& shortcutId) const;
    bool unlockShortcut(const std::string& shortcutId);

private:
    GameFlowState flowState = GameFlowState::MainMenu;
    Difficulty selectedDifficulty = Difficulty::Normal;
    RunStatistics runStatistics;
    SaveData activeSave;
    bool hasActiveSave = false;
    float accumulatedSeconds = 0.0f;
};

} // namespace game

#endif // TESTCPP1_GAMESESSION_H
