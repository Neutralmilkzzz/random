#ifndef TESTCPP1_PLAYER_H
#define TESTCPP1_PLAYER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "input/KeyStateManager.h"
#include "shared/GameTypes.h"

namespace game {
class Enemy;
class GroundEnemy;
class FlyingEnemy;
}

class Player {
public:
    explicit Player(KeyStateManager& keyStateManager);

    std::vector<int> getPalce();

    void resetRuntimeState();
    void move(std::string& currentmap);
    void updateCombat(const std::string& gameplayMap,
                      game::GroundEnemy& groundEnemy,
                      game::FlyingEnemy& flyingEnemy);
    void receiveDamage(const std::string& sourceLabel,
                       const game::Position& playerPosition = game::Position(-1, -1));

    bool isMovementLocked() const;
    bool isVisible() const;
    bool isAlive() const;
    bool consumeResetRequest();

    const game::CharacterStats& getStats() const;
    game::FacingDirection getFacingDirection() const;

    std::string buildHud() const;
    void overlayRender(std::string& renderMap, const std::string& gameplayMap) const;

    bool isGrounded(const std::string& currentmap, size_t pos);
    bool applyGravity(std::string& currentmap, size_t pos);
    bool jumpUp(std::string& currentmap, size_t pos);

private:
    struct VisualProjectile {
        game::Position position;
        int dx;
        int dy;
        int remainingFrames;
        int totalFrames;
        std::string label;

        VisualProjectile()
            : dx(0),
              dy(0),
              remainingFrames(0),
              totalFrames(0) {
        }
    };

    struct SpellVisualCell {
        int dx;
        int dy;
        char glyph;

        SpellVisualCell(int dxValue = 0, int dyValue = 0, char glyphValue = ' ')
            : dx(dxValue), dy(dyValue), glyph(glyphValue) {
        }
    };

    struct RewardPopupState {
        bool active;
        int elapsedFrames;
        int totalFrames;
        int amount;

        RewardPopupState()
            : active(false),
              elapsedFrames(0),
              totalFrames(24),
              amount(0) {
        }
    };

    enum class MeleeVisualType {
        Horizontal,
        Up,
        Down
    };

    struct HealCastState {
        bool active;
        int elapsedFrames;
        int totalFrames;

        HealCastState()
            : active(false),
              elapsedFrames(0),
              totalFrames(42) {
        }
    };

    struct UpWaveCastState {
        bool active;
        bool damageResolved;
        int elapsedFrames;
        int totalFrames;
        game::Position origin;

        UpWaveCastState()
            : active(false),
              damageResolved(false),
              elapsedFrames(0),
              totalFrames(18),
              origin(-1, -1) {
        }
    };

    struct DownSlamCastState {
        bool active;
        bool damageResolved;
        int elapsedFrames;
        int totalFrames;
        game::Position origin;
        game::Position impact;

        DownSlamCastState()
            : active(false),
              damageResolved(false),
              elapsedFrames(0),
              totalFrames(16),
              origin(-1, -1),
              impact(-1, -1) {
        }
    };

    struct MeleeVisualState {
        bool active;
        int elapsedFrames;
        int totalFrames;
        MeleeVisualType type;
        game::Position origin;
        game::FacingDirection facing;

        MeleeVisualState()
            : active(false),
              elapsedFrames(0),
              totalFrames(6),
              type(MeleeVisualType::Horizontal),
              origin(-1, -1),
              facing(game::FacingDirection::Right) {
        }
    };

    struct DeathAnimationState {
        bool active;
        int elapsedFrames;
        game::Position center;

        DeathAnimationState()
            : active(false),
              elapsedFrames(0),
              center(-1, -1) {
        }
    };

    KeyStateManager& ksm;
    bool isJumping;
    bool jumpHeldLastFrame;
    float horizontalMoveAccumulator;
    float verticalMoveAccumulator;
    float upwardVelocity;
    float downwardVelocity;
    float jumpHoldRemaining;
    float minimumJumpRiseRemaining;
    float riseVelocityDropAccumulator;

    game::CharacterStats stats;
    game::HitFeedbackState hitFeedback;
    game::FacingDirection facing;
    int framesSinceLastDamage;
    int blinkFramesRemaining;
    HealCastState healCast;
    UpWaveCastState upWaveCast;
    DownSlamCastState downSlamCast;
    MeleeVisualState meleeVisual;
    DeathAnimationState deathAnimation;
    RewardPopupState rewardPopup;
    std::vector<VisualProjectile> projectiles;
    std::unordered_map<int, bool> previousKeys;
    std::string lastAction;
    std::string lastResult;
    bool resetQueued;

    bool isKeyDown(int keyCode) const;
    bool wasKeyDown(int keyCode) const;
    bool isJustPressed(int keyCode) const;
    bool canSpendSoul(int soulCost);

    void updateFacingFromInput();
    void updateBlink();
    void updateDeathAnimation();
    void updateHealCast();
    void updateRewardPopup();
    void updateUpWaveCast(const std::string& gameplayMap,
                         game::GroundEnemy& groundEnemy,
                         game::FlyingEnemy& flyingEnemy);
    void updateDownSlamCast(const std::string& gameplayMap,
                           game::GroundEnemy& groundEnemy,
                           game::FlyingEnemy& flyingEnemy);
    void updateMeleeVisual();
    void updateProjectiles(const std::string& gameplayMap,
                          game::GroundEnemy& groundEnemy,
                          game::FlyingEnemy& flyingEnemy);

    void castHorizontalWave(const game::Position& playerPosition);
    void castUpWave(const game::Position& playerPosition);
    void castDownSlam(const std::string& gameplayMap, const game::Position& playerPosition);
    void startHealCast();
    void startMeleeVisual(MeleeVisualType type, const game::Position& origin);
    void triggerDeathAnimation(const game::Position& center, const std::string& sourceLabel);

    int upWaveStage() const;
    int downSlamStage() const;
    int meleeVisualStage() const;
    int deathExpansionRadius() const;

    bool isEnemyInMeleeRange(const game::Position& playerPosition,
                             const game::Position& enemyPosition) const;
    bool applyDamageToEnemyAtPosition(game::Enemy& enemy,
                                      const game::Position& targetPosition,
                                      const game::DamageInfo& damageInfo);
    bool applyDamageToEnemyInMeleeRange(game::Enemy& enemy,
                                        const game::Position& playerPosition,
                                        const game::DamageInfo& damageInfo);
    void grantKillReward(int amount);
    game::Position findDownSlamImpactPosition(const std::string& gameplayMap,
                                              const game::Position& origin) const;

    std::vector<SpellVisualCell> buildHorizontalWaveVisualCells(const VisualProjectile& projectile) const;
    std::vector<SpellVisualCell> buildUpWaveVisualCells() const;
    std::vector<SpellVisualCell> buildDownSlamVisualCells() const;
    std::vector<SpellVisualCell> buildMeleeVisualCells() const;
    std::vector<game::Position> buildUpWaveDamageCells(const std::string& gameplayMap, bool includeCrown) const;
    std::vector<game::Position> buildDownSlamDamageCells() const;

    std::vector<std::string> buildSoulVesselLines() const;
    std::string buildHealthOrbLine() const;
    void applyDeathAnimationOverlay(std::string& renderMap) const;
};

#endif // TESTCPP1_PLAYER_H
