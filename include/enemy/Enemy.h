#ifndef TESTCPP1_ENEMY_H
#define TESTCPP1_ENEMY_H

#include <string>

#include "combat/CombatSystem.h"

namespace game {

enum class EnemyType {
    Ground,
    Flying,
    BossMelee,
    BossRanged
};

class Enemy : public CombatActor {
public:
    virtual ~Enemy() = default;

    virtual EnemyType getEnemyType() const = 0;
    virtual void updateAI(const Position& playerPosition, float deltaSeconds) = 0;
    virtual AttackDefinition getPrimaryAttack() const = 0;
    virtual int getHkdReward() const = 0;
};

class GroundEnemy : public Enemy {
public:
    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    int getHkdReward() const override;
};

class FlyingEnemy : public Enemy {
public:
    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    AttackDefinition getFireballAttack() const;
    int getHkdReward() const override;
};

class Boss : public Enemy {
public:
    bool shouldEnterStagger() const;
    void resetStaggerMeter();

protected:
    int staggerThreshold = 40;
    int damageTakenSinceLastStagger = 0;
    TimedWindow staggerWindow = {false, 5.0f, 0.0f};
};

class MeleeBoss : public Boss {
public:
    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    AttackDefinition getBackDashSlash() const;
    AttackDefinition getFrontJumpSlash() const;
    AttackDefinition getFrontDash() const;
    int getHkdReward() const override;
};

class RangedBoss : public Boss {
public:
    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    AttackDefinition getStaffKnockback() const;
    AttackDefinition getFireballShot() const;
    AttackDefinition getMeteorRain() const;
    int getHkdReward() const override;
};

} // namespace game

#endif // TESTCPP1_ENEMY_H
