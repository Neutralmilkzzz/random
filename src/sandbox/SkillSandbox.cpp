#include <iostream>
#include <vector>

#include "combat/SkillSystem.h"
#include "shared/GameTypes.h"

namespace {

const char* skillName(game::SkillId skillId) {
    switch (skillId) {
        case game::SkillId::BasicAttack:
            return "BasicAttack";
        case game::SkillId::UpSlash:
            return "UpSlash";
        case game::SkillId::DownSlash:
            return "DownSlash";
        case game::SkillId::SoulWaveHorizontal:
            return "SoulWaveHorizontal";
        case game::SkillId::SoulWaveUp:
            return "SoulWaveUp";
        case game::SkillId::SoulSlam:
            return "SoulSlam";
        case game::SkillId::Heal:
            return "Heal";
        case game::SkillId::DoubleJump:
            return "DoubleJump";
        case game::SkillId::ShadowDash:
            return "ShadowDash";
    }

    return "Unknown";
}

int soulCost(game::SkillId skillId) {
    switch (skillId) {
        case game::SkillId::SoulWaveHorizontal:
            return 10;
        case game::SkillId::SoulWaveUp:
        case game::SkillId::SoulSlam:
            return 20;
        case game::SkillId::Heal:
            return 30;
        default:
            return 0;
    }
}

} // namespace

int main() {
    game::CharacterStats stats;
    stats.soul.current = 100;

    const std::vector<game::SkillId> skills = {
        game::SkillId::BasicAttack,
        game::SkillId::UpSlash,
        game::SkillId::DownSlash,
        game::SkillId::SoulWaveHorizontal,
        game::SkillId::SoulWaveUp,
        game::SkillId::SoulSlam,
        game::SkillId::Heal,
        game::SkillId::DoubleJump,
        game::SkillId::ShadowDash
    };

    std::cout << "[SkillSandbox] skill list scaffold\n";
    std::cout << "Current soul: " << stats.soul.current << "/" << stats.soul.maximum << "\n\n";

    for (std::vector<game::SkillId>::const_iterator it = skills.begin(); it != skills.end(); ++it) {
        std::cout << "- " << skillName(*it) << " | soul cost: " << soulCost(*it) << "\n";
    }

    std::cout << "\nNext step: bind concrete Skill implementations here.\n";
    return 0;
}
