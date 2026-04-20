#ifndef TESTCPP1_NPCSYSTEM_H
#define TESTCPP1_NPCSYSTEM_H

#include <string>
#include <vector>

#include "shared/GameTypes.h"

namespace game {

enum class NpcType {
    Merchant,
    Doctor,
    Lore,
    VillageChief,
    Ambient
};

struct ShopOffer {
    std::string id;
    std::string name;
    int hkdCost = 0;
    SkillId unlockedSkill = SkillId::BasicAttack;
    int bonusHealth = 0;
    int bonusAttack = 0;
    int bonusAttackSpeed = 0;
};

class Npc {
public:
    virtual ~Npc() = default;

    virtual const std::string& getId() const = 0;
    virtual const std::string& getDisplayName() const = 0;
    virtual NpcType getNpcType() const = 0;
    virtual std::vector<DialogueLine> getDialogue() const = 0;
};

class Merchant : public Npc {
public:
    NpcType getNpcType() const override;
    std::vector<DialogueLine> getDialogue() const override;
    const std::vector<ShopOffer>& getOffers() const;
    bool canBuyHealthUpgrade(const CharacterStats& stats) const;

private:
    std::vector<ShopOffer> offers;
};

class DoctorNpc : public Npc {
public:
    NpcType getNpcType() const override;
    std::vector<DialogueLine> getDialogue() const override;
    void healToFull(CharacterStats& stats) const;
};

class LoreNpc : public Npc {
public:
    NpcType getNpcType() const override;
    std::vector<DialogueLine> getDialogue() const override;
};

class VillageChiefNpc : public LoreNpc {
public:
    std::vector<DialogueLine> getDialogue() const override;
};

class AmbientNpc : public Npc {
public:
    NpcType getNpcType() const override;
    std::vector<DialogueLine> getDialogue() const override;
};

} // namespace game

#endif // TESTCPP1_NPCSYSTEM_H
