#ifndef TESTCPP1_SKILLSYSTEM_H
#define TESTCPP1_SKILLSYSTEM_H

#include <vector>

#include "shared/GameTypes.h"

namespace game {

class CombatActor;
class CombatSystem;

enum class SkillCategory {
    CoreAttack,
    SoulSpell,
    PassiveUpgrade
};

struct SkillUnlock {
    SkillId skillId = SkillId::BasicAttack;
    bool unlocked = false;
    int hkdCost = 0;
};

class Skill {
public:
    virtual ~Skill() = default;

    virtual SkillId getId() const = 0;
    virtual SkillCategory getCategory() const = 0;
    virtual int getSoulCost() const = 0;
    virtual bool canActivate(const CharacterStats& casterStats) const = 0;
    virtual void activate(CombatActor& caster, CombatSystem& combatSystem) = 0;
};

class SkillLoadout {
public:
    bool isUnlocked(SkillId skillId) const;
    void unlock(SkillId skillId);
    void equipPassive(SkillId skillId);

private:
    std::vector<SkillId> unlockedSkills;
    std::vector<SkillId> equippedPassives;
};

class SkillSystem {
public:
    void registerSkill(Skill* skill);
    bool canCast(SkillId skillId, const CharacterStats& casterStats) const;
    void cast(SkillId skillId, CombatActor& caster, CombatSystem& combatSystem);
    void update(float deltaSeconds);

private:
    std::vector<Skill*> registeredSkills;
};

} // namespace game

#endif // TESTCPP1_SKILLSYSTEM_H
