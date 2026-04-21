#include <algorithm>

#include "combat/CombatSystem.h"
#include "enemy/Enemy.h"

namespace game {

void CombatSystem::queueAttack(const AttackDefinition& attackDefinition) {
    queuedAttacks.push_back(attackDefinition);
}

void CombatSystem::registerProjectile(Projectile* projectile) {
    if (projectile != 0) {
        activeProjectiles.push_back(projectile);
    }
}

void CombatSystem::update(float deltaSeconds) {
    for (size_t index = 0; index < activeProjectiles.size();) {
        Projectile* projectile = activeProjectiles[index];
        if (projectile == 0) {
            activeProjectiles.erase(activeProjectiles.begin() + static_cast<long long>(index));
            continue;
        }

        projectile->update(deltaSeconds);
        if (projectile->isExpired()) {
            activeProjectiles.erase(activeProjectiles.begin() + static_cast<long long>(index));
            continue;
        }

        ++index;
    }

    queuedAttacks.clear();
}

bool CombatSystem::canResolveHeal(const CombatActor& actor, float secondsSinceLastHit) const {
    const CharacterStats& stats = actor.getStats();
    return actor.isAlive() &&
           stats.health.current < stats.health.maximum &&
           secondsSinceLastHit >= 1.5f;
}

DamageResolution CombatSystem::resolveAttack(CombatActor& target,
                                             const AttackDefinition& attackDefinition) const {
    return resolveDamage(target, attackDefinition.damage, attackDefinition.soulGainOnHit);
}

DamageResolution CombatSystem::resolveDamage(CombatActor& target,
                                             const DamageInfo& damageInfo,
                                             int soulGainOnHit) const {
    DamageResolution resolution;
    if (!target.isAlive()) {
        return resolution;
    }

    const int previousHealth = target.getStats().health.current;
    target.takeDamage(damageInfo);
    const int currentHealth = target.getStats().health.current;

    if (currentHealth < previousHealth) {
        resolution.hitApplied = true;
        resolution.damageApplied = previousHealth - currentHealth;
        resolution.soulGranted = std::max(0, soulGainOnHit);
    }

    resolution.targetDefeated = !target.isAlive();
    return resolution;
}

RewardResolution CombatSystem::buildEnemyReward(const Enemy& enemy) const {
    RewardResolution reward;
    reward.hkdGranted = std::max(0, enemy.getHkdReward());
    return reward;
}

void CombatSystem::applyReward(CharacterStats& recipient, const RewardResolution& reward) const {
    recipient.hkd += std::max(0, reward.hkdGranted);
    if (reward.soulGranted > 0) {
        recipient.soul.current = std::min(recipient.soul.maximum,
                                          recipient.soul.current + reward.soulGranted);
    }
}

void CombatSystem::openTimedWindow(TimedWindow& window, float durationSeconds) const {
    window.active = true;
    window.durationSeconds = std::max(0.0f, durationSeconds);
    window.elapsedSeconds = 0.0f;
}

void CombatSystem::advanceTimedWindow(TimedWindow& window, float deltaSeconds) const {
    if (!window.active) {
        return;
    }

    window.elapsedSeconds += std::max(0.0f, deltaSeconds);
    if (window.elapsedSeconds >= window.durationSeconds) {
        window.active = false;
        window.elapsedSeconds = 0.0f;
    }
}

} // namespace game
