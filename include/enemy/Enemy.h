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

enum class GroundEnemyState {
    Idle,
    Chase,
    AttackStartup,
    DashAttack,
    AttackRecovery,
    ReturnToPost,
    Dead
};

enum class FlyingEnemyState {
    Idle,
    Chase,
    AttackStartup,
    AttackRecovery,
    ReturnToPost,
    Dead
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
    GroundEnemy(const std::string& enemyId = "ground_enemy",
                const Position& spawnPosition = Position());

    const std::string& getId() const override;
    Position getPosition() const override;
    FacingDirection getFacingDirection() const override;
    const CharacterStats& getStats() const override;
    CharacterStats& accessStats() override;
    HitFeedbackState& accessHitFeedback() override;
    void takeDamage(const DamageInfo& damageInfo) override;
    bool isAlive() const override;

    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    int getHkdReward() const override;

    void setPosition(const Position& newPosition);
    void setSpawnPosition(const Position& newSpawnPosition);
    Position getSpawnPosition() const;
    GroundEnemyState getState() const;
    bool isAggroed() const;
    bool isTouchingPlayer(const Position& playerPosition) const;
    bool isInAttackRange(const Position& playerPosition) const;
    bool consumeAttackTrigger();
    bool isRenderable() const;
    bool shouldDespawn() const;
    char getRenderGlyph() const;

private:
    void updateHitFeedback(float deltaSeconds);
    void moveTowardX(int targetX, float deltaSeconds);
    void startAttack();
    void startDashAttack(const Position& playerPosition);

    std::string id;
    Position position;
    Position spawnPosition;
    FacingDirection facingDirection;
    CharacterStats stats;
    HitFeedbackState hitFeedback;
    GroundEnemyState state;
    bool alive;
    bool attackTriggerQueued;
    float moveAccumulator;
    float attackStartupRemaining;
    float attackRecoveryRemaining;
    float aggroRange;
    float loseAggroRange;
    float alertRange;
    float attackRange;
    float patrolRange;
    float moveStepSeconds;
    float attackStartupSeconds;
    float dashStepSeconds;
    float attackRecoverySeconds;
    float idlePauseRemaining;
    float idlePauseSeconds;
    float hitFlashSeconds;
    float deathFlashSeconds;
    float deathMarkerSeconds;
    float deathAnimationRemaining;
    int dashStepsRemaining;
    int patrolDirection;
    float dashAccumulator;
};

class FlyingEnemy : public Enemy {
public:
    FlyingEnemy(const std::string& enemyId = "flying_enemy",
                const Position& spawnPosition = Position());

    const std::string& getId() const override;
    Position getPosition() const override;
    FacingDirection getFacingDirection() const override;
    const CharacterStats& getStats() const override;
    CharacterStats& accessStats() override;
    HitFeedbackState& accessHitFeedback() override;
    void takeDamage(const DamageInfo& damageInfo) override;
    bool isAlive() const override;

    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    AttackDefinition getFireballAttack() const;
    int getHkdReward() const override;

    void setPosition(const Position& newPosition);
    void setSpawnPosition(const Position& newSpawnPosition);
    Position getSpawnPosition() const;
    FlyingEnemyState getState() const;
    bool consumeProjectileTrigger();
    bool isRenderable() const;
    bool shouldDespawn() const;
    char getRenderGlyph() const;

private:
    void updateHitFeedback(float deltaSeconds);
    void moveToward(const Position& targetPosition, float deltaSeconds);
    void startAttack();

    std::string id;
    Position position;
    Position spawnPosition;
    FacingDirection facingDirection;
    CharacterStats stats;
    HitFeedbackState hitFeedback;
    FlyingEnemyState state;
    bool alive;
    bool projectileTriggerQueued;
    float moveAccumulator;
    float attackStartupRemaining;
    float attackRecoveryRemaining;
    float aggroRange;
    float loseAggroRange;
    float alertRange;
    float hoverRange;
    float moveStepSeconds;
    float attackStartupSeconds;
    float attackRecoverySeconds;
    float hitFlashSeconds;
    float deathFlashSeconds;
    float deathMarkerSeconds;
    float deathAnimationRemaining;
    int hoverDirection;
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
