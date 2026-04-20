#ifndef TESTCPP1_COMBATSYSTEM_H
#define TESTCPP1_COMBATSYSTEM_H

#include <string>
#include <vector>

#include "shared/GameTypes.h"

namespace game {

class CombatActor {
public:
    virtual ~CombatActor() = default;

    virtual const std::string& getId() const = 0;
    virtual Position getPosition() const = 0;
    virtual FacingDirection getFacingDirection() const = 0;
    virtual const CharacterStats& getStats() const = 0;
    virtual CharacterStats& accessStats() = 0;
    virtual HitFeedbackState& accessHitFeedback() = 0;
    virtual void takeDamage(const DamageInfo& damageInfo) = 0;
    virtual bool isAlive() const = 0;
};

struct AttackDefinition {
    std::string id;
    DamageInfo damage;
    int soulGainOnHit = 0;
    float startupSeconds = 0.0f;
    float activeSeconds = 0.0f;
    float recoverySeconds = 0.0f;
    int knockbackX = 0;
    int knockbackY = 0;
};

class Projectile {
public:
    virtual ~Projectile() = default;

    virtual void update(float deltaSeconds) = 0;
    virtual bool isExpired() const = 0;
    virtual Position getPosition() const = 0;
    virtual DamageInfo getDamageInfo() const = 0;
};

class CombatSystem {
public:
    void queueAttack(const AttackDefinition& attackDefinition);
    void registerProjectile(Projectile* projectile);
    void update(float deltaSeconds);
    bool canResolveHeal(const CombatActor& actor, float secondsSinceLastHit) const;

private:
    std::vector<AttackDefinition> queuedAttacks;
    std::vector<Projectile*> activeProjectiles;
};

} // namespace game

#endif // TESTCPP1_COMBATSYSTEM_H
