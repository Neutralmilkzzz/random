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

bool CombatSystem::isTargetAtPosition(const Position& targetPosition,
                                      const Position& attackPosition) const {
    return targetPosition.x == attackPosition.x &&
           targetPosition.y == attackPosition.y;
}

bool CombatSystem::isTargetInCells(const Position& targetPosition,
                                   const std::vector<Position>& attackCells) const {
    for (size_t index = 0; index < attackCells.size(); ++index) {
        if (isTargetAtPosition(targetPosition, attackCells[index])) {
            return true;
        }
    }

    return false;
}

bool CombatSystem::isTargetInRange(const Position& originPosition,
                                   const Position& targetPosition,
                                   int horizontalReach,
                                   int verticalTolerance) const {
    return std::abs(targetPosition.x - originPosition.x) <= std::max(0, horizontalReach) &&
           std::abs(targetPosition.y - originPosition.y) <= std::max(0, verticalTolerance);
}

bool CombatSystem::isTargetInFrontRange(const CombatActor& attacker,
                                        const Position& targetPosition,
                                        int horizontalReach,
                                        int verticalTolerance) const {
    const Position attackerPosition = attacker.getPosition();
    const int deltaX = targetPosition.x - attackerPosition.x;
    const int deltaY = std::abs(targetPosition.y - attackerPosition.y);

    if (deltaY > std::max(0, verticalTolerance)) {
        return false;
    }

    if (attacker.getFacingDirection() == FacingDirection::Right) {
        return deltaX >= 1 && deltaX <= std::max(0, horizontalReach);
    }

    return deltaX <= -1 && deltaX >= -std::max(0, horizontalReach);
}

bool CombatSystem::isTargetInVerticalRange(const CombatActor& attacker,
                                           const Position& targetPosition,
                                           bool aimingUp,
                                           int horizontalTolerance,
                                           int minVerticalDistance,
                                           int maxVerticalDistance) const {
    const Position attackerPosition = attacker.getPosition();
    const int deltaX = std::abs(targetPosition.x - attackerPosition.x);
    const int deltaY = targetPosition.y - attackerPosition.y;

    if (deltaX > std::max(0, horizontalTolerance)) {
        return false;
    }

    if (aimingUp) {
        return deltaY <= -std::max(0, minVerticalDistance) &&
               deltaY >= -std::max(std::max(0, minVerticalDistance), maxVerticalDistance);
    }

    return deltaY >= std::max(0, minVerticalDistance) &&
           deltaY <= std::max(std::max(0, minVerticalDistance), maxVerticalDistance);
}

DamageResolution CombatSystem::resolveAttack(CombatActor& target,
                                             const AttackDefinition& attackDefinition) const {
    return resolveDamage(target, attackDefinition.damage, attackDefinition.soulGainOnHit);
}

DamageResolution CombatSystem::resolveAttackAtPosition(CombatActor& target,
                                                       const Position& attackPosition,
                                                       const AttackDefinition& attackDefinition) const {
    if (!isTargetAtPosition(target.getPosition(), attackPosition)) {
        return DamageResolution();
    }

    return resolveAttack(target, attackDefinition);
}

DamageResolution CombatSystem::resolveAttackInCells(CombatActor& target,
                                                    const std::vector<Position>& attackCells,
                                                    const AttackDefinition& attackDefinition) const {
    if (!isTargetInCells(target.getPosition(), attackCells)) {
        return DamageResolution();
    }

    return resolveAttack(target, attackDefinition);
}

DamageResolution CombatSystem::resolveAttackInRange(CombatActor& target,
                                                    const Position& originPosition,
                                                    const AttackDefinition& attackDefinition,
                                                    int horizontalReach,
                                                    int verticalTolerance) const {
    if (!isTargetInRange(originPosition, target.getPosition(), horizontalReach, verticalTolerance)) {
        return DamageResolution();
    }

    return resolveAttack(target, attackDefinition);
}

DamageResolution CombatSystem::resolveAttackInFrontRange(const CombatActor& attacker,
                                                         CombatActor& target,
                                                         const AttackDefinition& attackDefinition,
                                                         int horizontalReach,
                                                         int verticalTolerance) const {
    if (!isTargetInFrontRange(attacker, target.getPosition(), horizontalReach, verticalTolerance)) {
        return DamageResolution();
    }

    return resolveAttack(target, attackDefinition);
}

DamageResolution CombatSystem::resolveAttackInVerticalRange(const CombatActor& attacker,
                                                            CombatActor& target,
                                                            const AttackDefinition& attackDefinition,
                                                            bool aimingUp,
                                                            int horizontalTolerance,
                                                            int minVerticalDistance,
                                                            int maxVerticalDistance) const {
    if (!isTargetInVerticalRange(attacker,
                                 target.getPosition(),
                                 aimingUp,
                                 horizontalTolerance,
                                 minVerticalDistance,
                                 maxVerticalDistance)) {
        return DamageResolution();
    }

    return resolveAttack(target, attackDefinition);
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

RewardResolution CombatSystem::buildEnemyRewardOnDefeat(const Enemy& enemy,
                                                        const DamageResolution& damageResolution) const {
    if (!damageResolution.targetDefeated) {
        return RewardResolution();
    }

    return buildEnemyReward(enemy);
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
