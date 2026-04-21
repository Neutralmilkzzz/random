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

enum class BossState {
    Dormant,
    Intro,
    Positioning,
    AttackStartup,
    AttackRecovery,
    Staggered,
    Dead
};

enum class BossAttackType {
    None,
    SweepSlash,
    DashSlash,
    JumpSlash,
    FireballBurst,
    MeteorDrop
};

struct BossAttackSignal {
    BossAttackType type;
    Position origin;
    Position targetPosition;
    FacingDirection facingDirection;

    BossAttackSignal(BossAttackType attackType = BossAttackType::None,
                     const Position& attackOrigin = Position(),
                     const Position& attackTargetPosition = Position(-1, -1),
                     FacingDirection facing = FacingDirection::Right)
        : type(attackType),
          origin(attackOrigin),
          targetPosition(attackTargetPosition),
          facingDirection(facing) {
    }
};

struct BossVisualGlyph {
    Position position;
    char glyph;

    BossVisualGlyph(const Position& glyphPosition = Position(), char glyphValue = ' ')
        : position(glyphPosition),
          glyph(glyphValue) {
    }
};

struct BossAttackVisual {
    std::vector<BossVisualGlyph> activeGlyphs;
    std::vector<BossVisualGlyph> fadeGlyphs;
    std::vector<Position> damageCells;
    Position landingPosition;
    int activeFrames;
    int fadeFrames;
    bool hasLandingPosition;
    std::string label;

    BossAttackVisual()
        : landingPosition(-1, -1),
          activeFrames(0),
          fadeFrames(0),
          hasLandingPosition(false) {
    }
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
    const std::string& getId() const override;
    Position getPosition() const override;
    FacingDirection getFacingDirection() const override;
    const CharacterStats& getStats() const override;
    CharacterStats& accessStats() override;
    HitFeedbackState& accessHitFeedback() override;
    void takeDamage(const DamageInfo& damageInfo) override;
    bool isAlive() const override;

    void setPosition(const Position& newPosition);
    void setSpawnPosition(const Position& newSpawnPosition);
    Position getSpawnPosition() const;
    BossState getState() const;
    bool isRenderable() const;
    bool shouldDespawn() const;
    char getRenderGlyph() const;
    bool consumeAttackSignal(BossAttackSignal& signal);
    bool consumeDefeatReward(RewardResolution& reward);
    bool hasEncounterStarted() const;
    bool isCombatActive() const;
    bool isStaggered() const;
    int getStaggerThreshold() const;
    int getStaggerDamage() const;
    bool isStaggerWindowActive() const;
    float getStaggerWindowRemaining() const;
    BossAttackType getStartupAttackType() const;
    Position getStartupTargetPosition() const;
    std::vector<BossVisualGlyph> buildBodyVisual() const;
    std::vector<BossVisualGlyph> buildStartupVisual() const;
    BossAttackVisual buildResolvedAttackVisual(const BossAttackSignal& signal) const;
    bool shouldEnterStagger() const;
    void resetStaggerMeter();
    virtual AttackDefinition getAttackForType(BossAttackType attackType) const = 0;

protected:
    Boss(const std::string& enemyId,
         EnemyType type,
         const Position& initialSpawnPosition,
         char baseRenderGlyph);

    bool updateCommonState(float deltaSeconds);
    void updateHitFeedback(float deltaSeconds);
    void faceToward(const Position& targetPosition);
    void moveToward(const Position& targetPosition, float deltaSeconds);
    bool isWithinHorizontalRange(const Position& targetPosition, float range) const;
    bool isWithinVerticalRange(const Position& targetPosition, float range) const;
    void wakeUp(float introSeconds);
    void startAttack(BossAttackType attackType,
                     float startupSeconds,
                     float recoverySeconds,
                     const Position& targetPosition = Position(-1, -1));
    void enterStagger(float staggerSeconds);
    void finishRecovery();

    std::string id;
    EnemyType enemyType;
    Position position;
    Position spawnPosition;
    FacingDirection facingDirection;
    CharacterStats stats;
    HitFeedbackState hitFeedback;
    BossState state;
    bool alive;
    char baseRenderGlyph;
    BossAttackSignal queuedAttackSignal;
    bool attackSignalQueued;
    float moveAccumulator;
    float moveStepSeconds;
    float introRemaining;
    float attackStartupRemaining;
    float attackRecoveryRemaining;
    float staggerRemaining;
    float hitFlashSeconds;
    float deathFlashSeconds;
    float deathMarkerSeconds;
    float deathAnimationRemaining;
    int staggerThreshold;
    int damageTakenSinceLastStagger;
    TimedWindow staggerWindow;
    RewardResolution pendingDefeatReward;
    bool defeatRewardQueued;
    BossAttackType startupAttackType;
    Position startupTargetPosition;
    float startupDurationSeconds;
    float startupRecoverySeconds;
    float staggerDurationSeconds;
    float visualElapsedSeconds;
};

class MeleeBoss : public Boss {
public:
    MeleeBoss(const std::string& enemyId = "melee_boss",
              const Position& spawnPosition = Position());

    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    AttackDefinition getAttackForType(BossAttackType attackType) const override;
    AttackDefinition getBackDashSlash() const;
    AttackDefinition getFrontJumpSlash() const;
    AttackDefinition getFrontDash() const;
    int getHkdReward() const override;

private:
    float aggroRange;
    float loseAggroRange;
    float slashRange;
    float dashRange;
    float jumpSlashRange;
    float introSeconds;
    float attackStartupSeconds;
    float attackRecoverySeconds;
    float staggerSeconds;
};

class RangedBoss : public Boss {
public:
    RangedBoss(const std::string& enemyId = "ranged_boss",
               const Position& spawnPosition = Position());

    EnemyType getEnemyType() const override;
    void updateAI(const Position& playerPosition, float deltaSeconds) override;
    AttackDefinition getPrimaryAttack() const override;
    AttackDefinition getAttackForType(BossAttackType attackType) const override;
    AttackDefinition getStaffKnockback() const;
    AttackDefinition getFireballShot() const;
    AttackDefinition getMeteorRain() const;
    int getHkdReward() const override;

private:
    float aggroRange;
    float loseAggroRange;
    float castRange;
    float retreatRange;
    float introSeconds;
    float attackStartupSeconds;
    float attackRecoverySeconds;
    float staggerSeconds;
    bool nextAttackMeteor;
};

} // namespace game

#endif // TESTCPP1_ENEMY_H
