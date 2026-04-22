#ifndef TESTCPP1_COMBATTUNING_H
#define TESTCPP1_COMBATTUNING_H

#include <algorithm>

#include "shared/GameTypes.h"

namespace game {

const int kStandardEnemyHealth = 15;
const int kBaseNailDamage = 5;
const int kUpgradedNailDamage = 6;
const int kSpellDamage = 15;
const int kBossHealth = 200;
const int kMinimumAttackPowerLevel = 1;
const int kMaximumAttackPowerLevel = 2;

inline int clampPlayerAttackPowerLevel(int attackPower) {
    return std::max(kMinimumAttackPowerLevel,
                    std::min(attackPower, kMaximumAttackPowerLevel));
}

inline void clampPlayerAttackPower(CharacterStats& stats) {
    stats.attackPower = clampPlayerAttackPowerLevel(stats.attackPower);
}

inline int getPlayerNailDamage(const CharacterStats& stats) {
    return clampPlayerAttackPowerLevel(stats.attackPower) >= kMaximumAttackPowerLevel
            ? kUpgradedNailDamage
            : kBaseNailDamage;
}

inline int getPlayerAttackDamage(const CharacterStats& stats, DamageType damageType) {
    switch (damageType) {
    case DamageType::BasicAttack:
    case DamageType::UpSlash:
    case DamageType::DownSlash:
        return getPlayerNailDamage(stats);
    case DamageType::SoulWaveHorizontal:
    case DamageType::SoulWaveUp:
    case DamageType::SoulSlam:
        return kSpellDamage;
    default:
        return 0;
    }
}

} // namespace game

#endif // TESTCPP1_COMBATTUNING_H
