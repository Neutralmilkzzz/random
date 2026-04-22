#include <algorithm>
#include <cmath>

#include "combat/CombatTuning.h"
#include "enemy/Enemy.h"

namespace game {

namespace {

const int kBossDefaultHealth = kBossHealth;
const int kRangedBossHealth = kBossHealth;

char mirrorBossGlyph(char glyph) {
    switch (glyph) {
    case '/':
        return '\\';
    case '\\':
        return '/';
    case '<':
        return '>';
    case '>':
        return '<';
    case '[':
        return ']';
    case ']':
        return '[';
    default:
        return glyph;
    }
}

float computeVisualProgress(float remaining, float total) {
    if (total <= 0.0f) {
        return 1.0f;
    }

    const float elapsed = total - remaining;
    return std::max(0.0f, std::min(1.0f, elapsed / total));
}

std::vector<BossVisualGlyph> buildGlyphPattern(const Position& anchor,
                                               FacingDirection facingDirection,
                                               const std::vector<std::string>& rows,
                                               int anchorColumn) {
    std::vector<BossVisualGlyph> glyphs;
    for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        std::string row = rows[rowIndex];
        int effectiveAnchor = anchorColumn;
        if (facingDirection == FacingDirection::Left) {
            std::reverse(row.begin(), row.end());
            for (size_t column = 0; column < row.size(); ++column) {
                row[column] = mirrorBossGlyph(row[column]);
            }
            effectiveAnchor = static_cast<int>(row.size()) - 1 - anchorColumn;
        }

        const int y = anchor.y - static_cast<int>(rows.size()) + 1 + static_cast<int>(rowIndex);
        for (size_t column = 0; column < row.size(); ++column) {
            if (row[column] == ' ') {
                continue;
            }
            glyphs.push_back(BossVisualGlyph(Position(anchor.x + static_cast<int>(column) - effectiveAnchor, y),
                                             row[column]));
        }
    }
    return glyphs;
}

std::vector<Position> buildBossFrontArc(const Position& origin,
                                        FacingDirection facingDirection,
                                        int startOffset,
                                        int reach) {
    std::vector<Position> cells;
    const int direction = facingDirection == FacingDirection::Right ? 1 : -1;
    for (int step = startOffset; step < startOffset + reach; ++step) {
        cells.push_back(Position(origin.x + direction * step, origin.y));
    }
    return cells;
}

std::vector<Position> buildBossJumpSlashCells(const Position& targetPosition) {
    std::vector<Position> cells;
    for (int dx = -2; dx <= 2; ++dx) {
        cells.push_back(Position(targetPosition.x + dx, targetPosition.y));
    }
    cells.push_back(Position(targetPosition.x, targetPosition.y - 1));
    cells.push_back(Position(targetPosition.x, targetPosition.y + 1));
    return cells;
}

std::vector<std::string> armoredIdlePattern(bool breathing) {
    if (breathing) {
        return std::vector<std::string>{
                "   ___     ",
                "  /###\\    ",
                " |#o_o|]   ",
                " |##_##|   ",
                "  /   \\    "};
    }

    return std::vector<std::string>{
            "   ___     ",
            "  /###\\    ",
            " |#o_o|]   ",
            " |#####|   ",
            "  /   \\    "};
}

std::vector<std::string> armoredWalkPattern() {
    return std::vector<std::string>{
            "   ___     ",
            "  /###\\    ",
            " |#o_o|]   ",
            " |#####|   ",
            " _/   \\_   "};
}

std::vector<std::string> armoredHammerStartupPattern(bool fullyDrawn) {
    if (fullyDrawn) {
        return std::vector<std::string>{
                "   ___ __  ",
                "  /###\\  ] ",
                " |#>_<#    ",
                " |#####|   ",
                "  /   \\    "};
    }

    return std::vector<std::string>{
            "   ___     ",
            "  /###\\__  ",
            " |#o_o#]   ",
            " |#####|   ",
            "  /   \\    "};
}

std::vector<std::string> armoredHammerRecoveryPattern(float progress) {
    if (progress < 0.22f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /###\\    ",
                " |#>_<|==] ",
                " |#####|   ",
                "    *      "};
    }

    if (progress < 0.55f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /###\\    ",
                " |#x_x|]   ",
                " |#####|   ",
                "   .====>  "};
    }

    return std::vector<std::string>{
            "   ___     ",
            "  /###\\    ",
            " |#-_-|]   ",
            " |#####|   ",
            "   . .     "};
}

std::vector<std::string> armoredDashStartupPattern(float progress) {
    if (progress < 0.55f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /###\\    ",
                " |#o_o|]   ",
                " |#####|   ",
                " ===.      "};
    }

    return std::vector<std::string>{
            "   ___     ",
            "  /###\\_   ",
            " |#>_<|]=  ",
            " |#####|   ",
            " =====     "};
}

std::vector<std::string> armoredDashRecoveryPattern(float progress) {
    if (progress < 0.25f) {
        return std::vector<std::string>{
                "  ___      ",
                " /###\\__   ",
                "|#>_<|]==  ",
                "|#####|    ",
                " ======    "};
    }

    if (progress < 0.60f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /###\\    ",
                " |#x_x|]=  ",
                " |#####|   ",
                "  .====    "};
    }

    return std::vector<std::string>{
            "   ___     ",
            "  /###\\    ",
            " |#-_-|]   ",
            " |#####|   ",
            "   . .     "};
}

std::vector<std::string> armoredJumpStartupPattern(float progress) {
    if (progress < 0.25f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /###\\    ",
                " |#o_o|]   ",
                " |#####|   ",
                " _/   \\_   "};
    }

    if (progress < 0.50f) {
        return std::vector<std::string>{
                "     ]     ",
                "   ___     ",
                "  /###\\    ",
                " |#o_o#|   ",
                "   / \\     "};
    }

    if (progress < 0.78f) {
        return std::vector<std::string>{
                "     ]     ",
                "    _!_    ",
                "   /###\\   ",
                "  |#>_<#|  ",
                "    / \\    "};
    }

    return std::vector<std::string>{
            "     ]     ",
            "   ___     ",
            "  /###\\    ",
            " |#x_x#|   ",
            "  _/ \\_    "};
}

std::vector<std::string> armoredJumpRecoveryPattern() {
    return std::vector<std::string>{
            "   ___     ",
            "  /###\\    ",
            " |#-_-|]   ",
            " |#####|   ",
            "  /   \\    "};
}

std::vector<std::string> armoredStaggerPattern(float progress) {
    if (progress < 0.20f) {
        return std::vector<std::string>{
                "   _x_     ",
                "  /#x#\\    ",
                " |#x_x|]   ",
                " |##x##|   ",
                "  /   \\    "};
    }

    if (progress < 0.40f) {
        return std::vector<std::string>{
                "   _x_     ",
                "  /# #\\    ",
                " | x_x |   ",
                " |##_##|   ",
                "  /   \\    "};
    }

    if (progress < 0.68f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /x x\\    ",
                " |  o  |   ",
                " |__ __|   ",
                "  /   \\    "};
    }

    if (progress < 0.88f) {
        return std::vector<std::string>{
                "   ___     ",
                "  /x x\\    ",
                " | \\o/ |   ",
                " |__ __|   ",
                "  /   \\    "};
    }

    return std::vector<std::string>{
            "   _x_     ",
            "  /###\\    ",
            " |#o_o|]   ",
            " |#####|   ",
            "  /   \\    "};
}

BossAttackVisual buildBossSweepVisual(const Position& origin, FacingDirection facingDirection) {
    BossAttackVisual visual;
    visual.activeFrames = 6;
    visual.fadeFrames = 4;
    visual.label = "Hammer shockwave";
    visual.damageCells = buildBossFrontArc(origin, facingDirection, 1, 6);

    const int direction = facingDirection == FacingDirection::Right ? 1 : -1;
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y - 1), '*'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 0, origin.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 1, origin.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y), '*'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 3, origin.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 4, origin.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 5, origin.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 6, origin.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 7, origin.y),
                                                  facingDirection == FacingDirection::Right ? '>' : '<'));

    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 1, origin.y), '.'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y), '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 3, origin.y), '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 4, origin.y), '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 5, origin.y), '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 6, origin.y),
                                                facingDirection == FacingDirection::Right ? '>' : '<'));
    return visual;
}

BossAttackVisual buildBossDashVisual(const Position& origin, FacingDirection facingDirection) {
    BossAttackVisual visual;
    visual.activeFrames = 6;
    visual.fadeFrames = 4;
    visual.label = "Armored charge";
    visual.damageCells = buildBossFrontArc(origin, facingDirection, 0, 7);

    const int direction = facingDirection == FacingDirection::Right ? 1 : -1;
    for (int step = 0; step < 6; ++step) {
        visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * step, origin.y), '='));
    }

    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 1, origin.y), '.'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y), '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 3, origin.y), '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 4, origin.y), '='));

    visual.hasLandingPosition = true;
    visual.landingPosition = Position(origin.x + direction * 4, origin.y);
    return visual;
}

BossAttackVisual buildBossJumpSlashVisual(const Position& targetPosition,
                                          FacingDirection facingDirection) {
    BossAttackVisual visual;
    visual.activeFrames = 8;
    visual.fadeFrames = 4;
    visual.label = "Jump smash";
    visual.damageCells = buildBossJumpSlashCells(targetPosition);

    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x - 1, targetPosition.y - 1), '\\'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x, targetPosition.y - 1), '|'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x + 1, targetPosition.y - 1), '/'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x - 2, targetPosition.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x - 1, targetPosition.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(targetPosition, '*'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x + 1, targetPosition.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x + 2, targetPosition.y), '='));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x - 1, targetPosition.y + 1), '/'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x, targetPosition.y + 1), '|'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x + 1, targetPosition.y + 1), '\\'));

    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x - 1, targetPosition.y - 1), '.'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x, targetPosition.y - 1), '|'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x + 1, targetPosition.y - 1), '.'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x - 1, targetPosition.y), '.'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(targetPosition, '='));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(targetPosition.x + 1, targetPosition.y), '.'));

    const int landingOffset = facingDirection == FacingDirection::Right ? -1 : 1;
    visual.hasLandingPosition = true;
    visual.landingPosition = Position(targetPosition.x + landingOffset, targetPosition.y);
    return visual;
}

BossAttackVisual buildBossStaffVisual(const Position& origin, FacingDirection facingDirection) {
    BossAttackVisual visual;
    visual.activeFrames = 5;
    visual.fadeFrames = 3;
    visual.label = "Boss staff knockback";
    visual.damageCells = buildBossFrontArc(origin, facingDirection, 1, 3);

    const int direction = facingDirection == FacingDirection::Right ? 1 : -1;
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction, origin.y), '-'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y), '-'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 3, origin.y), '-'));
    visual.activeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y + 1), '*'));

    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction, origin.y), '.'));
    visual.fadeGlyphs.push_back(BossVisualGlyph(Position(origin.x + direction * 2, origin.y), '-'));
    return visual;
}

} // namespace

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
      groundPlaneY(initialSpawnPosition.y),
      patrolDirection(1),
      dashAccumulator(0.0f) {
    stats.health.current = kStandardEnemyHealth;
    stats.health.maximum = kStandardEnemyHealth;
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
        groundPlaneY = spawnPosition.y;
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
                dashStepsRemaining = 0;
                enterAttackRecovery();
                return;
            }
        }

        if (dashStepsRemaining <= 0) {
            if (isInAttackRange(playerPosition)) {
                attackTriggerQueued = true;
            }
            enterAttackRecovery();
        }
        return;
    }

    if (state == GroundEnemyState::AttackRecovery) {
        snapToGroundPlane();
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
        groundPlaneY = spawnPosition.y;
        snapToGroundPlane();

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
        groundPlaneY = spawnPosition.y;
        snapToGroundPlane();
        if (position.x == spawnPosition.x && position.y == spawnPosition.y) {
            state = GroundEnemyState::Idle;
            moveAccumulator = 0.0f;
            idlePauseRemaining = idlePauseSeconds;
            return;
        }

        moveTowardX(spawnPosition.x, deltaSeconds);
        snapToGroundPlane();

        if (position.x == spawnPosition.x && position.y == spawnPosition.y) {
            state = GroundEnemyState::Idle;
            idlePauseRemaining = idlePauseSeconds;
        }
        return;
    }

    if (state == GroundEnemyState::Chase) {
        snapToGroundPlane();
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
    attack.horizontalReach = static_cast<int>(attackRange);
    attack.verticalTolerance = 0;
    attack.directional = false;
    return attack;
}

int GroundEnemy::getHkdReward() const {
    return 10;
}

void GroundEnemy::setPosition(const Position& newPosition) {
    position = newPosition;
    groundPlaneY = newPosition.y;
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

void GroundEnemy::snapToGroundPlane() {
    position.y = groundPlaneY;
}

void GroundEnemy::enterAttackRecovery() {
    state = GroundEnemyState::AttackRecovery;
    attackRecoveryRemaining = attackRecoverySeconds;
    dashAccumulator = 0.0f;
    snapToGroundPlane();
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
    groundPlaneY = position.y;
    moveAccumulator = 0.0f;
    dashStepsRemaining = 0;
    dashAccumulator = 0.0f;
}

void GroundEnemy::startDashAttack(const Position& playerPosition) {
    const int deltaX = playerPosition.x - position.x;
    facingDirection = deltaX < 0 ? FacingDirection::Left : FacingDirection::Right;
    if (playerPosition.y > position.y) {
        groundPlaneY = std::min(playerPosition.y, position.y + 2);
    } else {
        groundPlaneY = position.y;
    }
    state = GroundEnemyState::DashAttack;
    dashStepsRemaining = std::max(3, std::min(5, std::abs(deltaX)));
    dashAccumulator = 0.0f;
}

FlyingEnemy::FlyingEnemy(const std::string& enemyId, const Position& initialSpawnPosition)
    : id(enemyId),
      position(initialSpawnPosition),
      spawnPosition(initialSpawnPosition),
      facingDirection(FacingDirection::Left),
      state(FlyingEnemyState::Idle),
      alive(true),
      projectileTriggerQueued(false),
      moveAccumulator(0.0f),
      attackStartupRemaining(0.0f),
      attackRecoveryRemaining(0.0f),
      aggroRange(12.0f),
      loseAggroRange(18.0f),
      alertRange(8.0f),
      hoverRange(2.0f),
      moveStepSeconds(0.24f),
      attackStartupSeconds(0.30f),
      attackRecoverySeconds(0.80f),
      hitFlashSeconds(0.12f),
      deathFlashSeconds(0.07f),
      deathMarkerSeconds(0.07f),
      deathAnimationRemaining(0.0f),
      hoverDirection(1) {
    stats.health.current = kStandardEnemyHealth;
    stats.health.maximum = kStandardEnemyHealth;
}

const std::string& FlyingEnemy::getId() const {
    return id;
}

Position FlyingEnemy::getPosition() const {
    return position;
}

FacingDirection FlyingEnemy::getFacingDirection() const {
    return facingDirection;
}

const CharacterStats& FlyingEnemy::getStats() const {
    return stats;
}

CharacterStats& FlyingEnemy::accessStats() {
    return stats;
}

HitFeedbackState& FlyingEnemy::accessHitFeedback() {
    return hitFeedback;
}

void FlyingEnemy::takeDamage(const DamageInfo& damageInfo) {
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
        state = FlyingEnemyState::Dead;
        projectileTriggerQueued = false;
        attackStartupRemaining = 0.0f;
        attackRecoveryRemaining = 0.0f;
        hitFeedback.blinking = false;
        hitFeedback.blinkDurationSeconds = 0.0f;
        hitFeedback.invulnerabilitySeconds = 0.0f;
        deathAnimationRemaining = deathFlashSeconds + deathMarkerSeconds;
    }
}

bool FlyingEnemy::isAlive() const {
    return alive;
}

EnemyType FlyingEnemy::getEnemyType() const {
    return EnemyType::Flying;
}

void FlyingEnemy::updateAI(const Position& playerPosition, float deltaSeconds) {
    updateHitFeedback(deltaSeconds);

    if (!alive) {
        state = FlyingEnemyState::Dead;
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

    if (state != FlyingEnemyState::AttackStartup &&
        state != FlyingEnemyState::AttackRecovery &&
        absX > loseAggroRange) {
        state = FlyingEnemyState::ReturnToPost;
    }

    if (state == FlyingEnemyState::AttackStartup) {
        attackStartupRemaining -= deltaSeconds;
        if (attackStartupRemaining <= 0.0f) {
            attackStartupRemaining = 0.0f;
            projectileTriggerQueued = true;
            state = FlyingEnemyState::AttackRecovery;
            attackRecoveryRemaining = attackRecoverySeconds;
        }
        return;
    }

    if (state == FlyingEnemyState::AttackRecovery) {
        attackRecoveryRemaining -= deltaSeconds;
        if (attackRecoveryRemaining <= 0.0f) {
            attackRecoveryRemaining = 0.0f;
            if (absX <= aggroRange && absY <= 5.0f) {
                state = FlyingEnemyState::Chase;
            } else {
                state = FlyingEnemyState::ReturnToPost;
            }
        }
        return;
    }

    if (state == FlyingEnemyState::Idle && absX <= aggroRange && absY <= 5.0f) {
        state = FlyingEnemyState::Chase;
        return;
    }

    if (state == FlyingEnemyState::Idle) {
        if (position.x != spawnPosition.x) {
            moveToward(Position(spawnPosition.x, position.y), deltaSeconds);
            return;
        }

        if (position.y >= spawnPosition.y + static_cast<int>(hoverRange)) {
            hoverDirection = -1;
        } else if (position.y <= spawnPosition.y - static_cast<int>(hoverRange)) {
            hoverDirection = 1;
        }

        moveToward(Position(spawnPosition.x, position.y + hoverDirection), deltaSeconds);
        return;
    }

    if (state == FlyingEnemyState::ReturnToPost) {
        if (position.x == spawnPosition.x && position.y == spawnPosition.y) {
            state = FlyingEnemyState::Idle;
            moveAccumulator = 0.0f;
            return;
        }

        moveToward(spawnPosition, deltaSeconds);
        return;
    }

    if (state == FlyingEnemyState::Chase) {
        if (absX > loseAggroRange) {
            state = FlyingEnemyState::ReturnToPost;
            return;
        }

        if (absX <= alertRange && absY <= 3.0f) {
            startAttack();
            return;
        }

        Position target = position;
        if (deltaX != 0) {
            target.x += deltaX > 0 ? 1 : -1;
        }
        if (std::abs(deltaY) > 1) {
            target.y += deltaY > 0 ? 1 : -1;
        }
        moveToward(target, deltaSeconds);
    }
}

AttackDefinition FlyingEnemy::getPrimaryAttack() const {
    return getFireballAttack();
}

AttackDefinition FlyingEnemy::getFireballAttack() const {
    AttackDefinition attack;
    attack.id = id + "_fireball";
    attack.damage = DamageInfo(1, DamageType::Fireball, id, false);
    attack.startupSeconds = attackStartupSeconds;
    attack.activeSeconds = 0.12f;
    attack.recoverySeconds = attackRecoverySeconds;
    return attack;
}

int FlyingEnemy::getHkdReward() const {
    return 10;
}

void FlyingEnemy::setPosition(const Position& newPosition) {
    position = newPosition;
}

void FlyingEnemy::setSpawnPosition(const Position& newSpawnPosition) {
    spawnPosition = newSpawnPosition;
}

Position FlyingEnemy::getSpawnPosition() const {
    return spawnPosition;
}

FlyingEnemyState FlyingEnemy::getState() const {
    return state;
}

bool FlyingEnemy::consumeProjectileTrigger() {
    if (!projectileTriggerQueued) {
        return false;
    }

    projectileTriggerQueued = false;
    return true;
}

bool FlyingEnemy::isRenderable() const {
    return alive || deathAnimationRemaining > 0.0f;
}

bool FlyingEnemy::shouldDespawn() const {
    return !alive && deathAnimationRemaining <= 0.0f;
}

char FlyingEnemy::getRenderGlyph() const {
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

    return 'f';
}

void FlyingEnemy::updateHitFeedback(float deltaSeconds) {
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

void FlyingEnemy::moveToward(const Position& targetPosition, float deltaSeconds) {
    if (position.x == targetPosition.x && position.y == targetPosition.y) {
        moveAccumulator = 0.0f;
        return;
    }

    moveAccumulator += deltaSeconds;
    while (moveAccumulator >= moveStepSeconds) {
        moveAccumulator -= moveStepSeconds;

        if (position.x != targetPosition.x) {
            position.x += position.x < targetPosition.x ? 1 : -1;
        }

        if (position.y != targetPosition.y) {
            position.y += position.y < targetPosition.y ? 1 : -1;
        }
    }
}

void FlyingEnemy::startAttack() {
    state = FlyingEnemyState::AttackStartup;
    attackStartupRemaining = attackStartupSeconds;
    projectileTriggerQueued = false;
    moveAccumulator = 0.0f;
}

Boss::Boss(const std::string& enemyId,
           EnemyType type,
           const Position& initialSpawnPosition,
           char renderGlyph)
    : id(enemyId),
      enemyType(type),
      position(initialSpawnPosition),
      spawnPosition(initialSpawnPosition),
      facingDirection(FacingDirection::Left),
      hitFeedback(false, 0.0f, 0.0f),
      state(BossState::Dormant),
      alive(true),
      baseRenderGlyph(renderGlyph),
      queuedAttackSignal(),
      attackSignalQueued(false),
      moveAccumulator(0.0f),
      moveStepSeconds(0.18f),
      introRemaining(0.0f),
      attackStartupRemaining(0.0f),
      attackRecoveryRemaining(0.0f),
      staggerRemaining(0.0f),
      hitFlashSeconds(0.12f),
      deathFlashSeconds(0.10f),
      deathMarkerSeconds(0.12f),
      deathAnimationRemaining(0.0f),
      staggerThreshold(6),
      damageTakenSinceLastStagger(0),
      staggerWindow(false, 5.0f, 0.0f),
      pendingDefeatReward(),
      defeatRewardQueued(false),
      startupAttackType(BossAttackType::None),
      startupTargetPosition(-1, -1),
      startupDurationSeconds(0.0f),
      startupRecoverySeconds(0.0f),
      staggerDurationSeconds(0.0f),
      visualElapsedSeconds(0.0f) {
    stats.health.current = kBossDefaultHealth;
    stats.health.maximum = kBossDefaultHealth;
}

const std::string& Boss::getId() const {
    return id;
}

Position Boss::getPosition() const {
    return position;
}

FacingDirection Boss::getFacingDirection() const {
    return facingDirection;
}

const CharacterStats& Boss::getStats() const {
    return stats;
}

CharacterStats& Boss::accessStats() {
    return stats;
}

HitFeedbackState& Boss::accessHitFeedback() {
    return hitFeedback;
}

void Boss::takeDamage(const DamageInfo& damageInfo) {
    if (!alive || hitFeedback.invulnerabilitySeconds > 0.0f) {
        return;
    }

    const int effectiveDamage = std::max(0, damageInfo.amount);
    if (effectiveDamage <= 0) {
        return;
    }

    stats.health.current -= effectiveDamage;
    hitFeedback.blinking = true;
    hitFeedback.blinkDurationSeconds = hitFlashSeconds;
    hitFeedback.invulnerabilitySeconds = hitFlashSeconds;

    CombatSystem combatSystem;
    if (!staggerWindow.active) {
        combatSystem.openTimedWindow(staggerWindow, 5.0f);
    }
    damageTakenSinceLastStagger += effectiveDamage;

    if (stats.health.current <= 0) {
        stats.health.current = 0;
        alive = false;
        state = BossState::Dead;
        pendingDefeatReward = combatSystem.buildEnemyReward(*this);
        defeatRewardQueued = true;
        attackSignalQueued = false;
        queuedAttackSignal = BossAttackSignal();
        introRemaining = 0.0f;
        attackStartupRemaining = 0.0f;
        attackRecoveryRemaining = 0.0f;
        startupTargetPosition = Position(-1, -1);
        staggerRemaining = 0.0f;
        moveAccumulator = 0.0f;
        deathAnimationRemaining = deathFlashSeconds + deathMarkerSeconds;
        hitFeedback.blinking = false;
        hitFeedback.blinkDurationSeconds = 0.0f;
        hitFeedback.invulnerabilitySeconds = 0.0f;
        resetStaggerMeter();
    }
}

bool Boss::isAlive() const {
    return alive;
}

void Boss::setPosition(const Position& newPosition) {
    position = newPosition;
}

void Boss::setSpawnPosition(const Position& newSpawnPosition) {
    spawnPosition = newSpawnPosition;
}

Position Boss::getSpawnPosition() const {
    return spawnPosition;
}

BossState Boss::getState() const {
    return state;
}

bool Boss::isRenderable() const {
    return alive || deathAnimationRemaining > 0.0f;
}

bool Boss::shouldDespawn() const {
    return !alive && deathAnimationRemaining <= 0.0f;
}

char Boss::getRenderGlyph() const {
    if (!alive) {
        if (deathAnimationRemaining > deathMarkerSeconds) {
            return '*';
        }
        if (deathAnimationRemaining > 0.0f) {
            return 'x';
        }
        return ' ';
    }

    if (state == BossState::Staggered) {
        return '&';
    }

    if (hitFeedback.blinking) {
        return '*';
    }

    return baseRenderGlyph;
}

bool Boss::consumeAttackSignal(BossAttackSignal& signal) {
    if (!attackSignalQueued) {
        return false;
    }

    signal = queuedAttackSignal;
    queuedAttackSignal = BossAttackSignal();
    attackSignalQueued = false;
    return true;
}

bool Boss::consumeDefeatReward(RewardResolution& reward) {
    if (!defeatRewardQueued) {
        return false;
    }

    reward = pendingDefeatReward;
    pendingDefeatReward = RewardResolution();
    defeatRewardQueued = false;
    return true;
}

bool Boss::hasEncounterStarted() const {
    return state != BossState::Dormant;
}

bool Boss::isCombatActive() const {
    return alive &&
           state != BossState::Dormant &&
           state != BossState::Intro &&
           state != BossState::Dead;
}

bool Boss::isStaggered() const {
    return state == BossState::Staggered;
}

int Boss::getStaggerThreshold() const {
    return staggerThreshold;
}

int Boss::getStaggerDamage() const {
    return damageTakenSinceLastStagger;
}

bool Boss::isStaggerWindowActive() const {
    return staggerWindow.active;
}

float Boss::getStaggerWindowRemaining() const {
    if (!staggerWindow.active) {
        return 0.0f;
    }

    return std::max(0.0f, staggerWindow.durationSeconds - staggerWindow.elapsedSeconds);
}

BossAttackType Boss::getStartupAttackType() const {
    return startupAttackType;
}

Position Boss::getStartupTargetPosition() const {
    return startupTargetPosition;
}

std::vector<BossVisualGlyph> Boss::buildBodyVisual() const {
    if (enemyType == EnemyType::BossRanged) {
        std::vector<BossVisualGlyph> glyphs;
        const bool ragePhase = stats.health.current * 2 <= stats.health.maximum;
        const char coreGlyph = getRenderGlyph() == '&' ? 'o' : getRenderGlyph();

        if (state == BossState::Dead) {
            if (getRenderGlyph() == '*') {
                const bool burstFrame = std::fmod(visualElapsedSeconds, 0.16f) < 0.08f;
                if (burstFrame) {
                    glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x, position.y), 'x'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '-'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '*'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '-'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
                } else {
                    glyphs.push_back(BossVisualGlyph(Position(position.x, position.y - 1), '*'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '*'));
                    glyphs.push_back(BossVisualGlyph(position, 'o'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '*'));
                    glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '*'));
                }
                return glyphs;
            }

            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '.'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '*'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 3, position.y), '.'));
            glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '.'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '.'));
            return glyphs;
        }

        if (state == BossState::Staggered) {
            const float phase = std::fmod(visualElapsedSeconds, 0.36f);
            if (phase < 0.12f) {
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
                glyphs.push_back(BossVisualGlyph(position, 'x'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '|'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
            } else if (phase < 0.24f) {
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
                glyphs.push_back(BossVisualGlyph(position, 'o'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '*'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
            } else {
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
                glyphs.push_back(BossVisualGlyph(position, 'x'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '|'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 3), '.'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 3), '.'));
            }
            return glyphs;
        }

        if (ragePhase) {
            const bool rageFrame = std::fmod(visualElapsedSeconds, 0.22f) >= 0.11f;
            if (!rageFrame) {
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y - 1), '~'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y - 1), '!'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y - 1), '~'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y), '\\'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), 'o'));
                glyphs.push_back(BossVisualGlyph(position, '_'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), 'o'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 3, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '|'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '*'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '|'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 3, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 3), '~'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 3), '~'));
            } else {
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y - 1), '~'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y - 1), '~'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y - 1), '~'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y), '\\'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), 'o'));
                glyphs.push_back(BossVisualGlyph(position, '_'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), 'o'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 3, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '*'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '|'));
                glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '*'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '|'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '*'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 3, position.y + 1), '-'));
                glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
                glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
            }
            return glyphs;
        }

        const float hoverPhase = std::fmod(visualElapsedSeconds, 0.54f);
        if (hoverPhase < 0.18f) {
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
            glyphs.push_back(BossVisualGlyph(position, coreGlyph));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
            glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '-'));
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
            glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '|'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '-'));
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
            glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 3), '~'));
            return glyphs;
        }

        if (hoverPhase < 0.36f) {
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
            glyphs.push_back(BossVisualGlyph(position, coreGlyph));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
            glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '~'));
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
            glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '|'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '~'));
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
            return glyphs;
        }

        glyphs.push_back(BossVisualGlyph(Position(position.x - 3, position.y), '\\'));
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '\\'));
        glyphs.push_back(BossVisualGlyph(position, coreGlyph));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '/'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 3, position.y), '/'));
        glyphs.push_back(BossVisualGlyph(Position(position.x - 2, position.y + 1), '-'));
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '-'));
        glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '|'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '-'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 2, position.y + 1), '-'));
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 3), '~'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 3), '~'));
        return glyphs;
    }

    if (enemyType != EnemyType::BossMelee) {
        std::vector<BossVisualGlyph> glyphs;
        const char renderGlyph = getRenderGlyph();
        if (renderGlyph != ' ') {
            glyphs.push_back(BossVisualGlyph(position, renderGlyph));
        }
        return glyphs;
    }

    if (!alive) {
        std::vector<BossVisualGlyph> glyphs;
        const char renderGlyph = getRenderGlyph();
        if (renderGlyph != ' ') {
            glyphs.push_back(BossVisualGlyph(position, renderGlyph));
        }
        return glyphs;
    }

    if (state == BossState::Staggered) {
        return buildGlyphPattern(position,
                                 facingDirection,
                                 armoredStaggerPattern(computeVisualProgress(staggerRemaining, staggerDurationSeconds)),
                                 4);
    }

    if (state == BossState::AttackStartup) {
        const float progress = computeVisualProgress(attackStartupRemaining, startupDurationSeconds);
        if (startupAttackType == BossAttackType::SweepSlash) {
            return buildGlyphPattern(position, facingDirection, armoredHammerStartupPattern(progress >= 0.50f), 4);
        }
        if (startupAttackType == BossAttackType::DashSlash) {
            return buildGlyphPattern(position, facingDirection, armoredDashStartupPattern(progress), 4);
        }
        if (startupAttackType == BossAttackType::JumpSlash) {
            return buildGlyphPattern(position, facingDirection, armoredJumpStartupPattern(progress), 4);
        }
    }

    if (state == BossState::AttackRecovery) {
        const float progress = computeVisualProgress(attackRecoveryRemaining, startupRecoverySeconds);
        if (startupAttackType == BossAttackType::SweepSlash) {
            return buildGlyphPattern(position, facingDirection, armoredHammerRecoveryPattern(progress), 4);
        }
        if (startupAttackType == BossAttackType::DashSlash) {
            return buildGlyphPattern(position, facingDirection, armoredDashRecoveryPattern(progress), 4);
        }
        if (startupAttackType == BossAttackType::JumpSlash) {
            return buildGlyphPattern(position, facingDirection, armoredJumpRecoveryPattern(), 4);
        }
    }

    if (state == BossState::Positioning || state == BossState::Intro) {
        const bool walkFrame = std::fmod(visualElapsedSeconds, 0.36f) >= 0.18f;
        return buildGlyphPattern(position,
                                 facingDirection,
                                 walkFrame ? armoredWalkPattern() : armoredIdlePattern(false),
                                 4);
    }

    const bool breathing = std::fmod(visualElapsedSeconds, 0.50f) >= 0.25f;
    return buildGlyphPattern(position, facingDirection, armoredIdlePattern(breathing), 4);
}

std::vector<BossVisualGlyph> Boss::buildStartupVisual() const {
    std::vector<BossVisualGlyph> glyphs;
    if (state != BossState::AttackStartup) {
        return glyphs;
    }

    if (enemyType == EnemyType::BossMelee) {
        const Position warningRow(position.x, position.y - 5);
        glyphs.push_back(BossVisualGlyph(Position(warningRow.x - 1, warningRow.y), '!'));
        glyphs.push_back(BossVisualGlyph(warningRow, '!'));
        glyphs.push_back(BossVisualGlyph(Position(warningRow.x + 1, warningRow.y), '!'));

        if (startupAttackType == BossAttackType::JumpSlash && startupTargetPosition.x >= 0) {
            glyphs.push_back(BossVisualGlyph(Position(startupTargetPosition.x - 1, startupTargetPosition.y), '^'));
            glyphs.push_back(BossVisualGlyph(startupTargetPosition, '!'));
            glyphs.push_back(BossVisualGlyph(Position(startupTargetPosition.x + 1, startupTargetPosition.y), '^'));
        }
        return glyphs;
    }

    const Position warningRow(position.x, position.y - 1);
    glyphs.push_back(BossVisualGlyph(Position(warningRow.x - 1, warningRow.y), '!'));
    glyphs.push_back(BossVisualGlyph(warningRow, '!'));
    glyphs.push_back(BossVisualGlyph(Position(warningRow.x + 1, warningRow.y), '!'));

    if (enemyType == EnemyType::BossRanged) {
        const int direction = facingDirection == FacingDirection::Right ? 1 : -1;
        const int wandX = position.x + direction * 2;

        if (startupAttackType == BossAttackType::FireballBurst) {
            glyphs.push_back(BossVisualGlyph(Position(wandX, position.y + 1), '*'));
            glyphs.push_back(BossVisualGlyph(Position(wandX + direction, position.y + 1), '*'));
        } else if (startupAttackType == BossAttackType::MeteorDrop) {
            glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 2), '/'));
            glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 2), '|'));
            glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 2), '\\'));
        } else if (startupAttackType == BossAttackType::SweepSlash) {
            glyphs.push_back(BossVisualGlyph(Position(wandX, position.y + 1), '_'));
            glyphs.push_back(BossVisualGlyph(Position(wandX + direction, position.y + 1), '-'));
        }
        return glyphs;
    }

    if (facingDirection == FacingDirection::Right) {
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y), '>'));
    } else {
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y), '<'));
    }

    if (startupAttackType == BossAttackType::SweepSlash) {
        const int slashX = facingDirection == FacingDirection::Right ? position.x + 1 : position.x - 1;
        glyphs.push_back(BossVisualGlyph(Position(slashX, position.y + 1),
                                         facingDirection == FacingDirection::Right ? '/' : '\\'));
    } else if (startupAttackType == BossAttackType::DashSlash) {
        const int direction = facingDirection == FacingDirection::Right ? 1 : -1;
        for (int step = 1; step <= 4; ++step) {
            glyphs.push_back(BossVisualGlyph(Position(position.x + direction * step, position.y + 1), '='));
        }
        glyphs.push_back(BossVisualGlyph(Position(position.x + direction * 5, position.y + 1), '.'));
    } else if (startupAttackType == BossAttackType::JumpSlash) {
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '/'));
        glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '#'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '\\'));

        if (startupTargetPosition.x >= 0) {
            glyphs.push_back(BossVisualGlyph(Position(startupTargetPosition.x - 1, startupTargetPosition.y), '^'));
            glyphs.push_back(BossVisualGlyph(startupTargetPosition, '!'));
            glyphs.push_back(BossVisualGlyph(Position(startupTargetPosition.x + 1, startupTargetPosition.y), '^'));
        }
    } else if (startupAttackType == BossAttackType::FireballBurst) {
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '.'));
        glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '*'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '.'));
    } else if (startupAttackType == BossAttackType::MeteorDrop) {
        glyphs.push_back(BossVisualGlyph(Position(position.x - 1, position.y + 1), '/'));
        glyphs.push_back(BossVisualGlyph(Position(position.x, position.y + 1), '|'));
        glyphs.push_back(BossVisualGlyph(Position(position.x + 1, position.y + 1), '\\'));
    }

    return glyphs;
}

BossAttackVisual Boss::buildResolvedAttackVisual(const BossAttackSignal& signal) const {
    if (signal.type == BossAttackType::SweepSlash) {
        if (enemyType == EnemyType::BossRanged) {
            return buildBossStaffVisual(signal.origin, signal.facingDirection);
        }
        return buildBossSweepVisual(signal.origin, signal.facingDirection);
    }

    if (signal.type == BossAttackType::DashSlash) {
        return buildBossDashVisual(signal.origin, signal.facingDirection);
    }

    if (signal.type == BossAttackType::JumpSlash) {
        const Position target = signal.targetPosition.x >= 0 ? signal.targetPosition : signal.origin;
        return buildBossJumpSlashVisual(target, signal.facingDirection);
    }

    return BossAttackVisual();
}

bool Boss::shouldEnterStagger() const {
    return alive &&
           state != BossState::Dead &&
           state != BossState::Staggered &&
           damageTakenSinceLastStagger >= staggerThreshold;
}

void Boss::resetStaggerMeter() {
    damageTakenSinceLastStagger = 0;
    staggerWindow.active = false;
    staggerWindow.elapsedSeconds = 0.0f;
}

bool Boss::updateCommonState(float deltaSeconds) {
    updateHitFeedback(deltaSeconds);
    visualElapsedSeconds += deltaSeconds;

    CombatSystem combatSystem;
    combatSystem.advanceTimedWindow(staggerWindow, deltaSeconds);
    if (!staggerWindow.active && damageTakenSinceLastStagger > 0 && state != BossState::Staggered) {
        resetStaggerMeter();
    }

    if (!alive) {
        state = BossState::Dead;
        return true;
    }

    if (state == BossState::Intro) {
        introRemaining -= deltaSeconds;
        if (introRemaining <= 0.0f) {
            introRemaining = 0.0f;
            state = BossState::Positioning;
        }
        return true;
    }

    if (state == BossState::AttackStartup) {
        attackStartupRemaining -= deltaSeconds;
        if (attackStartupRemaining <= 0.0f) {
            attackStartupRemaining = 0.0f;
            queuedAttackSignal = BossAttackSignal(startupAttackType,
                                                  position,
                                                  startupTargetPosition,
                                                  facingDirection);
            attackSignalQueued = true;
            state = BossState::AttackRecovery;
            attackRecoveryRemaining = startupRecoverySeconds;
        }
        return true;
    }

    if (state == BossState::AttackRecovery) {
        attackRecoveryRemaining -= deltaSeconds;
        if (attackRecoveryRemaining <= 0.0f) {
            finishRecovery();
        }
        return true;
    }

    if (state == BossState::Staggered) {
        staggerRemaining -= deltaSeconds;
        if (staggerRemaining <= 0.0f) {
            staggerRemaining = 0.0f;
            state = BossState::Positioning;
        }
        return true;
    }

    return false;
}

void Boss::updateHitFeedback(float deltaSeconds) {
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

void Boss::faceToward(const Position& targetPosition) {
    if (targetPosition.x < position.x) {
        facingDirection = FacingDirection::Left;
    } else if (targetPosition.x > position.x) {
        facingDirection = FacingDirection::Right;
    }
}

void Boss::moveToward(const Position& targetPosition, float deltaSeconds) {
    if (position.x == targetPosition.x && position.y == targetPosition.y) {
        moveAccumulator = 0.0f;
        return;
    }

    moveAccumulator += deltaSeconds;
    while (moveAccumulator >= moveStepSeconds) {
        moveAccumulator -= moveStepSeconds;

        if (position.x != targetPosition.x) {
            position.x += position.x < targetPosition.x ? 1 : -1;
        }

        if (position.y != targetPosition.y) {
            position.y += position.y < targetPosition.y ? 1 : -1;
        }
    }
}

bool Boss::isWithinHorizontalRange(const Position& targetPosition, float range) const {
    return std::abs(targetPosition.x - position.x) <= static_cast<int>(range);
}

bool Boss::isWithinVerticalRange(const Position& targetPosition, float range) const {
    return std::abs(targetPosition.y - position.y) <= static_cast<int>(range);
}

void Boss::wakeUp(float introSeconds) {
    if (state != BossState::Dormant) {
        return;
    }

    state = BossState::Intro;
    introRemaining = introSeconds;
    moveAccumulator = 0.0f;
}

void Boss::startAttack(BossAttackType attackType,
                       float startupSeconds,
                       float recoverySeconds,
                       const Position& targetPosition) {
    startupAttackType = attackType;
    startupTargetPosition = targetPosition;
    startupDurationSeconds = startupSeconds;
    startupRecoverySeconds = recoverySeconds;
    attackStartupRemaining = startupSeconds;
    attackRecoveryRemaining = 0.0f;
    state = BossState::AttackStartup;
    attackSignalQueued = false;
    queuedAttackSignal = BossAttackSignal();
    moveAccumulator = 0.0f;
}

void Boss::enterStagger(float staggerSeconds) {
    state = BossState::Staggered;
    staggerDurationSeconds = staggerSeconds;
    staggerRemaining = staggerSeconds;
    introRemaining = 0.0f;
    attackStartupRemaining = 0.0f;
    attackRecoveryRemaining = 0.0f;
    attackSignalQueued = false;
    queuedAttackSignal = BossAttackSignal();
    startupTargetPosition = Position(-1, -1);
    moveAccumulator = 0.0f;
    resetStaggerMeter();
}

void Boss::finishRecovery() {
    attackRecoveryRemaining = 0.0f;
    state = BossState::Positioning;
}

MeleeBoss::MeleeBoss(const std::string& enemyId, const Position& initialSpawnPosition)
    : Boss(enemyId, EnemyType::BossMelee, initialSpawnPosition, 'B'),
      aggroRange(18.0f),
      loseAggroRange(24.0f),
      slashRange(2.0f),
      dashRange(7.0f),
      jumpSlashRange(6.0f),
      introSeconds(0.55f),
      attackStartupSeconds(0.45f),
      attackRecoverySeconds(0.70f),
      staggerSeconds(1.10f) {
    staggerThreshold = 7;
    moveStepSeconds = 0.14f;
}

EnemyType MeleeBoss::getEnemyType() const {
    return EnemyType::BossMelee;
}

void MeleeBoss::updateAI(const Position& playerPosition, float deltaSeconds) {
    if (updateCommonState(deltaSeconds)) {
        return;
    }

    if (shouldEnterStagger()) {
        enterStagger(staggerSeconds);
        return;
    }

    const float absX = static_cast<float>(std::abs(playerPosition.x - position.x));
    const float absY = static_cast<float>(std::abs(playerPosition.y - position.y));
    faceToward(playerPosition);

    if (state == BossState::Dormant) {
        if (absX <= aggroRange && absY <= 4.0f) {
            wakeUp(introSeconds);
        }
        return;
    }

    if (absX > loseAggroRange) {
        moveToward(spawnPosition, deltaSeconds);
        return;
    }

    if (absY >= 2.0f && absX <= jumpSlashRange) {
        startAttack(BossAttackType::JumpSlash,
                    attackStartupSeconds + 0.10f,
                    attackRecoverySeconds + 0.18f,
                    playerPosition);
        return;
    }

    if (absY <= 1.0f && absX <= slashRange) {
        startAttack(BossAttackType::SweepSlash, attackStartupSeconds, attackRecoverySeconds);
        return;
    }

    if (absY <= 1.0f && absX <= dashRange) {
        startAttack(BossAttackType::DashSlash, attackStartupSeconds + 0.12f, attackRecoverySeconds + 0.10f);
        return;
    }

    Position target = position;
    if (playerPosition.x != position.x) {
        target.x += playerPosition.x > position.x ? 1 : -1;
    }
    if (std::abs(playerPosition.y - position.y) > 1) {
        target.y += playerPosition.y > position.y ? 1 : -1;
    }
    moveToward(target, deltaSeconds);
}

AttackDefinition MeleeBoss::getPrimaryAttack() const {
    return getFrontJumpSlash();
}

AttackDefinition MeleeBoss::getAttackForType(BossAttackType attackType) const {
    switch (attackType) {
    case BossAttackType::SweepSlash:
        return getFrontJumpSlash();
    case BossAttackType::DashSlash:
        return getFrontDash();
    case BossAttackType::JumpSlash:
        return getFrontJumpSlash();
    default:
        return getPrimaryAttack();
    }
}

AttackDefinition MeleeBoss::getBackDashSlash() const {
    AttackDefinition attack;
    attack.id = id + "_back_dash_slash";
    attack.damage = DamageInfo(1, DamageType::Dash, id, true);
    attack.startupSeconds = attackStartupSeconds + 0.10f;
    attack.activeSeconds = 0.16f;
    attack.recoverySeconds = attackRecoverySeconds;
    attack.knockbackX = facingDirection == FacingDirection::Right ? -2 : 2;
    return attack;
}

AttackDefinition MeleeBoss::getFrontJumpSlash() const {
    AttackDefinition attack;
    attack.id = id + "_front_jump_slash";
    attack.damage = DamageInfo(2, DamageType::BasicAttack, id, true);
    attack.startupSeconds = attackStartupSeconds;
    attack.activeSeconds = 0.18f;
    attack.recoverySeconds = attackRecoverySeconds;
    attack.knockbackX = facingDirection == FacingDirection::Right ? 1 : -1;
    attack.knockbackY = -1;
    return attack;
}

AttackDefinition MeleeBoss::getFrontDash() const {
    AttackDefinition attack;
    attack.id = id + "_front_dash";
    attack.damage = DamageInfo(2, DamageType::Dash, id, true);
    attack.startupSeconds = attackStartupSeconds + 0.12f;
    attack.activeSeconds = 0.14f;
    attack.recoverySeconds = attackRecoverySeconds + 0.10f;
    attack.knockbackX = facingDirection == FacingDirection::Right ? 2 : -2;
    return attack;
}

int MeleeBoss::getHkdReward() const {
    return 60;
}

RangedBoss::RangedBoss(const std::string& enemyId, const Position& initialSpawnPosition)
    : Boss(enemyId, EnemyType::BossRanged, initialSpawnPosition, 'o'),
      aggroRange(20.0f),
      loseAggroRange(26.0f),
      castRange(10.0f),
      retreatRange(4.0f),
      introSeconds(0.65f),
      attackStartupSeconds(0.40f),
      attackRecoverySeconds(0.75f),
      staggerSeconds(1.20f),
      nextAttackMeteor(false) {
    stats.health.current = kRangedBossHealth;
    stats.health.maximum = kRangedBossHealth;
    staggerThreshold = 6;
    moveStepSeconds = 0.18f;
}

EnemyType RangedBoss::getEnemyType() const {
    return EnemyType::BossRanged;
}

void RangedBoss::updateAI(const Position& playerPosition, float deltaSeconds) {
    if (updateCommonState(deltaSeconds)) {
        return;
    }

    if (shouldEnterStagger()) {
        enterStagger(staggerSeconds);
        nextAttackMeteor = false;
        return;
    }

    const float absX = static_cast<float>(std::abs(playerPosition.x - position.x));
    const float absY = static_cast<float>(std::abs(playerPosition.y - position.y));
    const bool ragePhase = stats.health.current * 2 <= stats.health.maximum;
    const float currentCastRange = ragePhase ? castRange + 2.0f : castRange;
    const float currentRetreatRange = ragePhase ? retreatRange + 1.0f : retreatRange;
    faceToward(playerPosition);

    if (state == BossState::Dormant) {
        if (absX <= aggroRange && absY <= 6.0f) {
            wakeUp(introSeconds);
        }
        return;
    }

    if (absX > loseAggroRange) {
        moveToward(spawnPosition, deltaSeconds);
        return;
    }

    if (absX <= currentRetreatRange && absY <= 2.0f) {
        startAttack(BossAttackType::SweepSlash,
                    ragePhase ? 0.20f : 0.25f,
                    ragePhase ? 0.24f : 0.35f);
        return;
    }

    if (absX <= currentCastRange && absY <= 5.0f) {
        const BossAttackType nextAttack = nextAttackMeteor ? BossAttackType::MeteorDrop : BossAttackType::FireballBurst;
        nextAttackMeteor = !nextAttackMeteor;
        startAttack(nextAttack,
                    nextAttack == BossAttackType::MeteorDrop
                            ? attackStartupSeconds + (ragePhase ? 0.10f : 0.15f)
                            : (ragePhase ? attackStartupSeconds - 0.06f : attackStartupSeconds),
                    nextAttack == BossAttackType::MeteorDrop
                            ? attackRecoverySeconds + (ragePhase ? 0.05f : 0.15f)
                            : (ragePhase ? attackRecoverySeconds - 0.18f : attackRecoverySeconds));
        return;
    }

    Position target = position;
    if (absX < currentRetreatRange) {
        target.x += playerPosition.x < position.x ? 1 : -1;
    } else if (absX > currentCastRange - 2.0f && playerPosition.x != position.x) {
        target.x += playerPosition.x > position.x ? 1 : -1;
    }

    if (std::abs(playerPosition.y - position.y) > 2) {
        target.y += playerPosition.y > position.y ? 1 : -1;
    } else if (ragePhase && position.y > spawnPosition.y - 1) {
        target.y -= 1;
    }
    moveToward(target, deltaSeconds);
}

AttackDefinition RangedBoss::getPrimaryAttack() const {
    return getFireballShot();
}

AttackDefinition RangedBoss::getAttackForType(BossAttackType attackType) const {
    switch (attackType) {
    case BossAttackType::FireballBurst:
        return getFireballShot();
    case BossAttackType::MeteorDrop:
        return getMeteorRain();
    case BossAttackType::JumpSlash:
    case BossAttackType::SweepSlash:
        return getStaffKnockback();
    default:
        return getPrimaryAttack();
    }
}

AttackDefinition RangedBoss::getStaffKnockback() const {
    AttackDefinition attack;
    attack.id = id + "_staff_knockback";
    attack.damage = DamageInfo(1, DamageType::StaffHit, id, true);
    attack.startupSeconds = 0.25f;
    attack.activeSeconds = 0.12f;
    attack.recoverySeconds = 0.35f;
    attack.knockbackX = facingDirection == FacingDirection::Right ? 1 : -1;
    return attack;
}

AttackDefinition RangedBoss::getFireballShot() const {
    AttackDefinition attack;
    attack.id = id + "_fireball_burst";
    attack.damage = DamageInfo(1, DamageType::Fireball, id, false);
    attack.startupSeconds = attackStartupSeconds;
    attack.activeSeconds = 0.15f;
    attack.recoverySeconds = attackRecoverySeconds;
    return attack;
}

AttackDefinition RangedBoss::getMeteorRain() const {
    AttackDefinition attack;
    attack.id = id + "_meteor_drop";
    attack.damage = DamageInfo(1, DamageType::Meteor, id, false);
    attack.startupSeconds = attackStartupSeconds + 0.15f;
    attack.activeSeconds = 0.20f;
    attack.recoverySeconds = attackRecoverySeconds + 0.15f;
    return attack;
}

int RangedBoss::getHkdReward() const {
    return 70;
}

} // namespace game
