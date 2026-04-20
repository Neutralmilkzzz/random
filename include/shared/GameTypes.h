#ifndef TESTCPP1_GAMETYPES_H
#define TESTCPP1_GAMETYPES_H

#include <string>
#include <vector>

namespace game {

enum class Difficulty {
    Easy,
    Normal,
    Hard
};

enum class FacingDirection {
    Left,
    Right
};

enum class DamageType {
    Contact,
    BasicAttack,
    UpSlash,
    DownSlash,
    SoulWaveHorizontal,
    SoulWaveUp,
    SoulSlam,
    Fireball,
    Meteor,
    Dash,
    StaffHit
};

enum class SkillId {
    BasicAttack,
    UpSlash,
    DownSlash,
    SoulWaveHorizontal,
    SoulWaveUp,
    SoulSlam,
    Heal,
    DoubleJump,
    ShadowDash
};

enum class MapType {
    SpawnVillage,
    CombatRoom,
    BossRoom,
    ShopRoom,
    EventRoom
};

enum class EventModifierType {
    Buff,
    Debuff,
    Neutral
};

struct Position {
    int x;
    int y;

    Position(int xValue = 0, int yValue = 0)
        : x(xValue), y(yValue) {
    }
};

struct ResourcePool {
    int current;
    int maximum;

    ResourcePool(int currentValue = 0, int maximumValue = 0)
        : current(currentValue), maximum(maximumValue) {
    }
};

struct CharacterStats {
    ResourcePool health;
    ResourcePool soul;
    int hkd;
    int attackPower;
    int attackSpeedLevel;
    int purchasedHealthSlots;

    CharacterStats()
        : health(5, 5),
          soul(0, 100),
          hkd(0),
          attackPower(1),
          attackSpeedLevel(1),
          purchasedHealthSlots(0) {
    }
};

struct HitFeedbackState {
    bool blinking;
    float blinkDurationSeconds;
    float invulnerabilitySeconds;

    HitFeedbackState(bool blinkingValue = false,
                     float blinkDurationValue = 2.0f,
                     float invulnerabilityValue = 2.0f)
        : blinking(blinkingValue),
          blinkDurationSeconds(blinkDurationValue),
          invulnerabilitySeconds(invulnerabilityValue) {
    }
};

struct DamageInfo {
    int amount;
    DamageType type;
    std::string sourceId;
    bool causesKnockback;

    DamageInfo(int amountValue = 0,
               DamageType typeValue = DamageType::Contact,
               const std::string& sourceIdValue = std::string(),
               bool causesKnockbackValue = false)
        : amount(amountValue),
          type(typeValue),
          sourceId(sourceIdValue),
          causesKnockback(causesKnockbackValue) {
    }
};

struct TimedWindow {
    bool active;
    float durationSeconds;
    float elapsedSeconds;

    TimedWindow(bool activeValue = false,
                float durationValue = 0.0f,
                float elapsedValue = 0.0f)
        : active(activeValue),
          durationSeconds(durationValue),
          elapsedSeconds(elapsedValue) {
    }
};

struct DialogueLine {
    std::string speaker;
    std::string text;
};

struct MapTransition {
    std::string fromMapId;
    std::string toMapId;
    std::string spawnPointId;
};

} // namespace game

#endif // TESTCPP1_GAMETYPES_H
