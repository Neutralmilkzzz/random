#include "core/GameSession.h"

#include <algorithm>

namespace game {

bool GameSession::startNewRun(Difficulty difficulty) {
    SaveSystem saveSystem;
    selectedDifficulty = difficulty;
    runStatistics = RunStatistics();
    accumulatedSeconds = 0.0f;
    activeSave = saveSystem.createNewSave(difficulty);
    hasActiveSave = true;
    flowState = GameFlowState::InGame;
    return true;
}

bool GameSession::continueRun(const SaveData& saveData) {
    if (!saveData.hasActiveRun) {
        return false;
    }

    selectedDifficulty = saveData.difficulty;
    runStatistics = RunStatistics();
    runStatistics.totalPlaySeconds = saveData.completionTimeSeconds;
    accumulatedSeconds = 0.0f;
    activeSave = saveData;
    hasActiveSave = true;
    flowState = GameFlowState::InGame;
    return true;
}

void GameSession::update(float deltaSeconds) {
    if (flowState != GameFlowState::InGame || deltaSeconds <= 0.0f) {
        return;
    }

    accumulatedSeconds += deltaSeconds;
    while (accumulatedSeconds >= 1.0f) {
        accumulatedSeconds -= 1.0f;
        runStatistics.totalPlaySeconds++;
        activeSave.completionTimeSeconds = runStatistics.totalPlaySeconds;
    }
}

void GameSession::markPlayerDead() {
    if (hasActiveSave) {
        flowState = GameFlowState::Dead;
    }
}

void GameSession::markBossDefeated(const std::string& bossId) {
    if (!bossId.empty()) {
        if (std::find(activeSave.defeatedBossIds.begin(), activeSave.defeatedBossIds.end(), bossId) ==
            activeSave.defeatedBossIds.end()) {
            runStatistics.bossesDefeated++;
            activeSave.defeatedBossIds.push_back(bossId);
        }
    }
    flowState = GameFlowState::Victory;
}

bool GameSession::hasDefeatedBoss(const std::string& bossId) const {
    if (!hasActiveSave || bossId.empty()) {
        return false;
    }

    return std::find(activeSave.defeatedBossIds.begin(),
                     activeSave.defeatedBossIds.end(),
                     bossId) != activeSave.defeatedBossIds.end();
}

void GameSession::resumeGameplay() {
    if (hasActiveSave) {
        flowState = GameFlowState::InGame;
    }
}

GameFlowState GameSession::getFlowState() const {
    return flowState;
}

const RunStatistics& GameSession::getRunStatistics() const {
    return runStatistics;
}

const SaveData& GameSession::getActiveSave() const {
    return activeSave;
}

void GameSession::setActiveSave(const SaveData& saveData) {
    activeSave = saveData;
    hasActiveSave = saveData.hasActiveRun;
    if (hasActiveSave && flowState != GameFlowState::Paused && flowState != GameFlowState::Victory) {
        flowState = GameFlowState::InGame;
    }
}

bool GameSession::hasSaveData() const {
    return hasActiveSave;
}

bool GameSession::isShortcutUnlocked(const std::string& shortcutId) const {
    if (!hasActiveSave || shortcutId.empty()) {
        return false;
    }

    return std::find(activeSave.unlockedShortcutIds.begin(),
                     activeSave.unlockedShortcutIds.end(),
                     shortcutId) != activeSave.unlockedShortcutIds.end();
}

bool GameSession::unlockShortcut(const std::string& shortcutId) {
    if (!hasActiveSave || shortcutId.empty()) {
        return false;
    }

    if (!isShortcutUnlocked(shortcutId)) {
        activeSave.unlockedShortcutIds.push_back(shortcutId);
    }
    return true;
}

} // namespace game
