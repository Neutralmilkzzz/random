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

enum class NpcInteractionType {
    Dialogue,
    Heal,
    Shop,
    Purchase
};

enum class ShopPurchaseStatus {
    None,
    Success,
    InsufficientHkd,
    SoldOut,
    NotAvailable,
    UnknownOffer
};

struct ShopOffer {
    std::string id;
    std::string name;
    std::string description;
    int hkdCost = 0;
    SkillId unlockedSkill = SkillId::BasicAttack;
    int bonusHealth = 0;
    int bonusAttack = 0;
    int bonusAttackSpeed = 0;
    int maxPurchaseCount = 1;
    bool unlocksSkill = false;
};

struct NpcInteractionResult {
    NpcInteractionType type;
    std::string speaker;
    std::string text;
    bool opensShop;
    bool healedToFull;
    int hkdSpent;
    int healthRestored;
    int bonusHealthApplied;
    int bonusAttackApplied;
    int bonusAttackSpeedApplied;
    bool unlockedSkillGranted;
    SkillId grantedSkill;
    ShopPurchaseStatus purchaseStatus;
    std::vector<ShopOffer> offers;

    NpcInteractionResult();
};

class Npc {
public:
    virtual ~Npc();

    const std::string& getId() const;
    const std::string& getDisplayName() const;
    NpcType getNpcType() const;

    virtual std::vector<DialogueLine> getDialogue() const = 0;
    virtual NpcInteractionResult interact(CharacterStats& stats) const;
    virtual bool opensShopOnInteract() const;
    virtual const std::vector<ShopOffer>& getOffers() const;
    virtual NpcInteractionResult buyOffer(const std::string& offerId, CharacterStats& stats) const;

protected:
    Npc(const std::string& idValue,
        const std::string& displayNameValue,
        NpcType npcTypeValue);

private:
    std::string id;
    std::string displayName;
    NpcType npcType;
};

class Merchant : public Npc {
public:
    Merchant();

    std::vector<DialogueLine> getDialogue() const override;
    NpcInteractionResult interact(CharacterStats& stats) const override;
    bool opensShopOnInteract() const override;
    const std::vector<ShopOffer>& getOffers() const override;
    NpcInteractionResult buyOffer(const std::string& offerId, CharacterStats& stats) const override;
    bool canBuyHealthUpgrade(const CharacterStats& stats) const;

private:
    std::vector<ShopOffer> offers;
};

class DoctorNpc : public Npc {
public:
    DoctorNpc();

    std::vector<DialogueLine> getDialogue() const override;
    NpcInteractionResult interact(CharacterStats& stats) const override;
    void healToFull(CharacterStats& stats) const;
};

class LoreNpc : public Npc {
public:
    LoreNpc(const std::string& idValue,
            const std::string& displayNameValue,
            const std::vector<DialogueLine>& dialogueValue);

    std::vector<DialogueLine> getDialogue() const override;

protected:
    std::vector<DialogueLine> dialogue;
};

class VillageChiefNpc : public LoreNpc {
public:
    VillageChiefNpc();

    std::vector<DialogueLine> getDialogue() const override;
    NpcInteractionResult interact(CharacterStats& stats) const override;
};

class AmbientNpc : public Npc {
public:
    AmbientNpc(const std::string& idValue,
               const std::string& displayNameValue,
               const std::string& textValue);

    std::vector<DialogueLine> getDialogue() const override;

private:
    std::string text;
};

const Npc& getNpcDefinition(const std::string& npcId);
NpcInteractionResult interactWithNpc(const std::string& npcId, CharacterStats& stats);
NpcInteractionResult purchaseNpcOffer(const std::string& npcId,
                                      const std::string& offerId,
                                      CharacterStats& stats);

} // namespace game

#endif // TESTCPP1_NPCSYSTEM_H
