#ifndef TESTCPP1_PLAYER_H
#define TESTCPP1_PLAYER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "combat/CombatSystem.h"
#include "input/KeyStateManager.h"
#include "shared/GameTypes.h"

namespace game {
class Enemy;
class GroundEnemy;
class FlyingEnemy;
}

class Player : public game::CombatActor {
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
    bool isAlive() const override;
    bool consumeResetRequest();

    const std::string& getId() const override;
    game::Position getPosition() const override;
    const game::CharacterStats& getStats() const override;
    game::CharacterStats& accessStats() override;
    game::HitFeedbackState& accessHitFeedback() override;
    void takeDamage(const game::DamageInfo& damageInfo) override;
    void restoreSavedStats(const game::CharacterStats& savedStats);
    game::FacingDirection getFacingDirection() const override;
    void setCombatPosition(const game::Position& position);
    void setDoubleJumpUnlocked(bool unlocked);
    bool hasDoubleJumpUnlocked() const;

    std::string buildHud() const;
    std::string buildHud(const std::string& locationName) const;
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
    std::string id;
    game::Position combatPosition;
    bool isJumping;
    bool jumpHeldLastFrame;
    bool doubleJumpUnlocked;
    bool airJumpAvailable;
    bool dashActive;
    bool dashAvailable;
    float horizontalMoveAccumulator;
    float verticalMoveAccumulator;
    float dashMoveAccumulator;
    float upwardVelocity;
    float downwardVelocity;
    float jumpHoldRemaining;
    float minimumJumpRiseRemaining;
    float riseVelocityDropAccumulator;
    int horizontalInputDirection;
    int pendingBasicAttackRecoilDirection;
    int dashFramesRemaining;
    int dashCooldownFrames;
    int dashDirection;

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
    bool consumeBasicAttackRecoil(std::string& currentmap);
    void applyIncomingDamage(const game::DamageInfo& damageInfo,
                             const game::Position& playerPosition);
    void applyCombatReward(const game::RewardResolution& reward,
                           const game::CombatSystem& combatSystem);
    bool applyEnemyCombatResolution(const game::Enemy& enemy,
                                    const game::DamageResolution& resolution,
                                    const game::CombatSystem& combatSystem);

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

    game::Position findDownSlamImpactPosition(const std::string& gameplayMap,
                                              const game::Position& origin) const;

    std::vector<SpellVisualCell> buildHorizontalWaveVisualCells(const VisualProjectile& projectile) const;
    std::vector<SpellVisualCell> buildUpWaveVisualCells() const;
    std::vector<SpellVisualCell> buildDownSlamVisualCells() const;
    std::vector<SpellVisualCell> buildDashVisualCells() const;
    std::vector<SpellVisualCell> buildMeleeVisualCells() const;
    std::vector<game::Position> buildUpWaveDamageCells(const std::string& gameplayMap, bool includeCrown) const;
    std::vector<game::Position> buildDownSlamDamageCells(const std::string& gameplayMap) const;

    std::vector<std::string> buildSoulVesselLines() const;
    std::string buildHealthOrbLine() const;
    void applyDeathAnimationOverlay(std::string& renderMap) const;
};

#endif // TESTCPP1_PLAYER_H
