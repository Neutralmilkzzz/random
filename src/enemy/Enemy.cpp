#include <cmath>

#include "enemy/Enemy.h"

namespace game {

GroundEnemy::GroundEnemy(const std::string& enemyId, const Position& initialSpawnPosition)
    : id(enemyId),
      position(initialSpawnPosition),
      spawnPosition(initialSpawnPosition),
      facingDirection(FacingDirection::Left),
      state(GroundEnemyState::Idle),
      alive(true),
      attackTriggerQueued(false),
      moveAccumulator(0.0f),
      attackStartupRemaining(0.0f),
      attackRecoveryRemaining(0.0f),
      aggroRange(10.0f),
      loseAggroRange(16.0f),
      alertRange(7.0f),
      attackRange(2.0f),
      patrolRange(3.0f),
      moveStepSeconds(0.12f),
      attackStartupSeconds(0.35f),
      dashStepSeconds(0.035f),
      attackRecoverySeconds(1.0f),
      idlePauseRemaining(0.0f),
      idlePauseSeconds(0.20f),
      hitFlashSeconds(0.12f),
      deathFlashSeconds(0.07f),
      deathMarkerSeconds(0.07f),
      deathAnimationRemaining(0.0f),
      dashStepsRemaining(0),
      patrolDirection(1),
      dashAccumulator(0.0f) {
    stats.health.current = 3;
    stats.health.maximum = 3;
}

const std::string& GroundEnemy::getId() const {
    return id;
}

Position GroundEnemy::getPosition() const {
    return position;
}

FacingDirection GroundEnemy::getFacingDirection() const {
    return facingDirection;
}

const CharacterStats& GroundEnemy::getStats() const {
    return stats;
}

CharacterStats& GroundEnemy::accessStats() {
    return stats;
}

HitFeedbackState& GroundEnemy::accessHitFeedback() {
    return hitFeedback;
}

void GroundEnemy::takeDamage(const DamageInfo& damageInfo) {
    if (!alive || hitFeedback.invulnerabilitySeconds > 0.0f) {
        return;
    }

    stats.health.current -= damageInfo.amount;
    hitFeedback.blinking = true;
    hitFeedback.blinkDurationSeconds = hitFlashSeconds;
    hitFeedback.invulnerabilitySeconds = hitFlashSeconds;

    if (stats.health.current <= 0) {
        stats.health.current = 0;
        alive = false;
        state = GroundEnemyState::Dead;
        hitFeedback.blinking = false;
        hitFeedback.blinkDurationSeconds = 0.0f;
        hitFeedback.invulnerabilitySeconds = 0.0f;
        attackTriggerQueued = false;
        attackStartupRemaining = 0.0f;
        attackRecoveryRemaining = 0.0f;
        idlePauseRemaining = 0.0f;
        deathAnimationRemaining = deathFlashSeconds + deathMarkerSeconds;
        dashStepsRemaining = 0;
        patrolDirection = 1;
        dashAccumulator = 0.0f;
    }
}

bool GroundEnemy::isAlive() const {
    return alive;
}

EnemyType GroundEnemy::getEnemyType() const {
    return EnemyType::Ground;
}

void GroundEnemy::updateAI(const Position& playerPosition, float deltaSeconds) {
    updateHitFeedback(deltaSeconds);

    if (!alive) {
        state = GroundEnemyState::Dead;
        return;
    }

    const int deltaX = playerPosition.x - position.x;
    const int deltaY = playerPosition.y - position.y;
    const float absX = static_cast<float>(std::abs(deltaX));
    const float absY = static_cast<float>(std::abs(deltaY));

    if (deltaX < 0) {
        facingDirection = FacingDirection::Left;
    } else if (deltaX > 0) {
        facingDirection = FacingDirection::Right;
    }

    if (state != GroundEnemyState::AttackStartup &&
        state != GroundEnemyState::DashAttack &&
        state != GroundEnemyState::AttackRecovery &&
        absX > loseAggroRange) {
        state = GroundEnemyState::ReturnToPost;
    }

    if (state == GroundEnemyState::AttackStartup) {
        attackStartupRemaining -= deltaSeconds;
        if (attackStartupRemaining <= 0.0f) {
            attackStartupRemaining = 0.0f;
            startDashAttack(playerPosition);
        }
        return;
    }

    if (state == GroundEnemyState::DashAttack) {
        dashAccumulator += deltaSeconds;

        while (dashStepsRemaining > 0 && dashAccumulator >= dashStepSeconds) {
            dashAccumulator -= dashStepSeconds;
            position.x += facingDirection == FacingDirection::Right ? 1 : -1;
            dashStepsRemaining--;

            if (isTouchingPlayer(playerPosition)) {
                attackTriggerQueued = true;
                state = GroundEnemyState::AttackRecovery;
                attackRecoveryRemaining = attackRecoverySeconds;
                dashStepsRemaining = 0;
                dashAccumulator = 0.0f;
                return;
            }
        }

        if (dashStepsRemaining <= 0) {
            if (isInAttackRange(playerPosition)) {
                attackTriggerQueued = true;
            }
            state = GroundEnemyState::AttackRecovery;
            attackRecoveryRemaining = attackRecoverySeconds;
            dashAccumulator = 0.0f;
        }
        return;
    }

    if (state == GroundEnemyState::AttackRecovery) {
        attackRecoveryRemaining -= deltaSeconds;
        if (attackRecoveryRemaining <= 0.0f) {
            attackRecoveryRemaining = 0.0f;
            if (absX <= aggroRange && absY <= 1.0f) {
                state = GroundEnemyState::Chase;
            } else {
                state = GroundEnemyState::ReturnToPost;
            }
        }
        return;
    }

    if (state == GroundEnemyState::Idle && absX <= aggroRange && absY <= 1.0f) {
        state = GroundEnemyState::Chase;
        idlePauseRemaining = 0.0f;
        return;
    }

    if (state == GroundEnemyState::Idle) {
        position.y = spawnPosition.y;

        if (idlePauseRemaining > 0.0f) {
            idlePauseRemaining -= deltaSeconds;
            if (idlePauseRemaining < 0.0f) {
                idlePauseRemaining = 0.0f;
            }
            return;
        }

        const int patrolTargetX = spawnPosition.x + static_cast<int>(patrolRange) * patrolDirection;
        moveTowardX(patrolTargetX, deltaSeconds);

        if ((patrolDirection > 0 && position.x >= patrolTargetX) ||
            (patrolDirection < 0 && position.x <= patrolTargetX)) {
            patrolDirection *= -1;
            idlePauseRemaining = idlePauseSeconds;
        }
        return;
    }

    if (state == GroundEnemyState::ReturnToPost) {
        if (position.x == spawnPosition.x && position.y == spawnPosition.y) {
            state = GroundEnemyState::Idle;
            moveAccumulator = 0.0f;
            idlePauseRemaining = idlePauseSeconds;
            return;
        }

        moveTowardX(spawnPosition.x, deltaSeconds);
        position.y = spawnPosition.y;

        if (position.x == spawnPosition.x && position.y == spawnPosition.y) {
            state = GroundEnemyState::Idle;
            idlePauseRemaining = idlePauseSeconds;
        }
        return;
    }

    if (state == GroundEnemyState::Chase) {
        if (absX > loseAggroRange) {
            state = GroundEnemyState::ReturnToPost;
            return;
        }

        if (absX <= alertRange && absY <= 0.0f) {
            startAttack();
            return;
        }

        moveTowardX(playerPosition.x, deltaSeconds);
    }
}

AttackDefinition GroundEnemy::getPrimaryAttack() const {
    AttackDefinition attack;
    attack.id = id + "_melee";
    attack.damage = DamageInfo(1, DamageType::Contact, id, true);
    attack.startupSeconds = attackStartupSeconds;
    attack.activeSeconds = 0.12f;
    attack.recoverySeconds = attackRecoverySeconds;
    attack.knockbackX = facingDirection == FacingDirection::Right ? 1 : -1;
    return attack;
}

int GroundEnemy::getHkdReward() const {
    return 10;
}

void GroundEnemy::setPosition(const Position& newPosition) {
    position = newPosition;
}

void GroundEnemy::setSpawnPosition(const Position& newSpawnPosition) {
    spawnPosition = newSpawnPosition;
}

Position GroundEnemy::getSpawnPosition() const {
    return spawnPosition;
}

GroundEnemyState GroundEnemy::getState() const {
    return state;
}

bool GroundEnemy::isAggroed() const {
    return state == GroundEnemyState::Chase ||
           state == GroundEnemyState::AttackStartup ||
           state == GroundEnemyState::DashAttack ||
           state == GroundEnemyState::AttackRecovery;
}

bool GroundEnemy::isTouchingPlayer(const Position& playerPosition) const {
    return std::abs(playerPosition.x - position.x) <= 1 &&
           playerPosition.y == position.y;
}

bool GroundEnemy::isInAttackRange(const Position& playerPosition) const {
    return std::abs(playerPosition.x - position.x) <= static_cast<int>(attackRange) &&
           playerPosition.y == position.y;
}

bool GroundEnemy::consumeAttackTrigger() {
    if (!attackTriggerQueued) {
        return false;
    }

    attackTriggerQueued = false;
    return true;
}

bool GroundEnemy::isRenderable() const {
    return alive || deathAnimationRemaining > 0.0f;
}

bool GroundEnemy::shouldDespawn() const {
    return !alive && deathAnimationRemaining <= 0.0f;
}

char GroundEnemy::getRenderGlyph() const {
    if (!alive) {
        if (deathAnimationRemaining > deathMarkerSeconds) {
            return '*';
        }
        if (deathAnimationRemaining > 0.0f) {
            return 'x';
        }
        return ' ';
    }

    if (hitFeedback.blinking) {
        return '*';
    }

    return 'g';
}

void GroundEnemy::updateHitFeedback(float deltaSeconds) {
    if (hitFeedback.blinkDurationSeconds > 0.0f) {
        hitFeedback.blinkDurationSeconds -= deltaSeconds;
        if (hitFeedback.blinkDurationSeconds <= 0.0f) {
            hitFeedback.blinkDurationSeconds = 0.0f;
            hitFeedback.blinking = false;
        }
    }

    if (hitFeedback.invulnerabilitySeconds > 0.0f) {
        hitFeedback.invulnerabilitySeconds -= deltaSeconds;
        if (hitFeedback.invulnerabilitySeconds < 0.0f) {
            hitFeedback.invulnerabilitySeconds = 0.0f;
        }
    }

    if (deathAnimationRemaining > 0.0f) {
        deathAnimationRemaining -= deltaSeconds;
        if (deathAnimationRemaining < 0.0f) {
            deathAnimationRemaining = 0.0f;
        }
    }
}

void GroundEnemy::moveTowardX(int targetX, float deltaSeconds) {
    if (position.x == targetX) {
        moveAccumulator = 0.0f;
        return;
    }

    moveAccumulator += deltaSeconds;
    while (moveAccumulator >= moveStepSeconds) {
        moveAccumulator -= moveStepSeconds;
        if (position.x < targetX) {
            position.x++;
        } else if (position.x > targetX) {
            position.x--;
        }
    }
}

void GroundEnemy::startAttack() {
    state = GroundEnemyState::AttackStartup;
    attackStartupRemaining = attackStartupSeconds;
    attackTriggerQueued = false;
    moveAccumulator = 0.0f;
    dashStepsRemaining = 0;
    dashAccumulator = 0.0f;
}

void GroundEnemy::startDashAttack(const Position& playerPosition) {
    const int deltaX = playerPosition.x - position.x;
    facingDirection = deltaX < 0 ? FacingDirection::Left : FacingDirection::Right;
    state = GroundEnemyState::DashAttack;
    dashStepsRemaining = std::max(3, std::min(5, std::abs(deltaX)));
    dashAccumulator = 0.0f;
}

} // namespace game
