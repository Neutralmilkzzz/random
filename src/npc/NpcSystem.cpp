#include "npc/NpcSystem.h"

#include <algorithm>

namespace {

const std::vector<game::ShopOffer>& emptyOffers() {
    static const std::vector<game::ShopOffer> offers;
    return offers;
}

std::string firstDialogueText(const game::Npc& npc) {
    const std::vector<game::DialogueLine> dialogue = npc.getDialogue();
    if (dialogue.empty()) {
        return "NPC placeholder.";
    }
    return dialogue.front().text;
}

std::vector<game::DialogueLine> buildVillageChiefDialogue() {
    std::vector<game::DialogueLine> dialogue;
    dialogue.push_back(game::DialogueLine{
            "Chief",
            "Head right and clear the next rooms. The upper shortcut opens later."});
    return dialogue;
}

const game::ShopOffer* findOfferById(const std::vector<game::ShopOffer>& offers,
                                     const std::string& offerId) {
    for (size_t index = 0; index < offers.size(); ++index) {
        if (offers[index].id == offerId) {
            return &offers[index];
        }
    }
    return 0;
}

} // namespace

namespace game {

NpcInteractionResult::NpcInteractionResult()
    : type(NpcInteractionType::Dialogue),
      opensShop(false),
      healedToFull(false),
      hkdSpent(0),
      healthRestored(0),
      bonusHealthApplied(0),
      bonusAttackApplied(0),
      bonusAttackSpeedApplied(0),
      unlockedSkillGranted(false),
      grantedSkill(SkillId::BasicAttack),
      purchaseStatus(ShopPurchaseStatus::None) {
}

Npc::Npc(const std::string& idValue,
         const std::string& displayNameValue,
         NpcType npcTypeValue)
    : id(idValue),
      displayName(displayNameValue),
      npcType(npcTypeValue) {
}

Npc::~Npc() {
}

const std::string& Npc::getId() const {
    return id;
}

const std::string& Npc::getDisplayName() const {
    return displayName;
}

NpcType Npc::getNpcType() const {
    return npcType;
}

NpcInteractionResult Npc::interact(CharacterStats& stats) const {
    (void)stats;
    NpcInteractionResult result;
    result.type = NpcInteractionType::Dialogue;
    result.speaker = getDisplayName();
    result.text = firstDialogueText(*this);
    return result;
}

bool Npc::opensShopOnInteract() const {
    return false;
}

const std::vector<ShopOffer>& Npc::getOffers() const {
    return emptyOffers();
}

NpcInteractionResult Npc::buyOffer(const std::string& offerId, CharacterStats& stats) const {
    (void)offerId;
    (void)stats;
    NpcInteractionResult result;
    result.type = NpcInteractionType::Purchase;
    result.speaker = getDisplayName();
    result.text = "This NPC does not run a shop.";
    result.purchaseStatus = ShopPurchaseStatus::NotAvailable;
    return result;
}

Merchant::Merchant()
    : Npc("merchant", "Merchant", NpcType::Merchant) {
    ShopOffer healthOffer;
    healthOffer.id = "vital_shell";
    healthOffer.name = "Vital Shell";
    healthOffer.description = "Health cap +1 and refill the new mask immediately.";
    healthOffer.hkdCost = 40;
    healthOffer.bonusHealth = 1;
    healthOffer.maxPurchaseCount = 2;
    offers.push_back(healthOffer);

    ShopOffer attackOffer;
    attackOffer.id = "nail_edge";
    attackOffer.name = "Nail Edge";
    attackOffer.description = "Attack level +1 as the smallest combat upgrade.";
    attackOffer.hkdCost = 55;
    attackOffer.bonusAttack = 1;
    attackOffer.maxPurchaseCount = 2;
    offers.push_back(attackOffer);

    ShopOffer dashOffer;
    dashOffer.id = "shadow_dash_token";
    dashOffer.name = "Shadow Dash Token";
    dashOffer.description = "Reserved mobility unlock. Returns an unlock marker for later wiring.";
    dashOffer.hkdCost = 90;
    dashOffer.unlocksSkill = true;
    dashOffer.unlockedSkill = SkillId::ShadowDash;
    dashOffer.maxPurchaseCount = 1;
    offers.push_back(dashOffer);
}

std::vector<DialogueLine> Merchant::getDialogue() const {
    std::vector<DialogueLine> dialogue;
    dialogue.push_back(DialogueLine{
            "Merchant",
            "Take a look. HKD can now be spent in the minimal shop loop."});
    return dialogue;
}

NpcInteractionResult Merchant::interact(CharacterStats& stats) const {
    (void)stats;
    NpcInteractionResult result;
    result.type = NpcInteractionType::Shop;
    result.speaker = getDisplayName();
    result.text = firstDialogueText(*this);
    result.opensShop = true;
    result.offers = offers;
    return result;
}

bool Merchant::opensShopOnInteract() const {
    return true;
}

const std::vector<ShopOffer>& Merchant::getOffers() const {
    return offers;
}

bool Merchant::canBuyHealthUpgrade(const CharacterStats& stats) const {
    return stats.purchasedHealthSlots < 2;
}

NpcInteractionResult Merchant::buyOffer(const std::string& offerId, CharacterStats& stats) const {
    NpcInteractionResult result;
    result.type = NpcInteractionType::Purchase;
    result.speaker = getDisplayName();
    result.offers = offers;

    const ShopOffer* offer = findOfferById(offers, offerId);
    if (offer == 0) {
        result.text = "That offer does not exist.";
        result.purchaseStatus = ShopPurchaseStatus::UnknownOffer;
        return result;
    }

    if (stats.hkd < offer->hkdCost) {
        result.text = "Not enough HKD.";
        result.purchaseStatus = ShopPurchaseStatus::InsufficientHkd;
        return result;
    }

    if (offer->bonusHealth > 0 && stats.purchasedHealthSlots >= offer->maxPurchaseCount) {
        result.text = "Vital Shell is sold out.";
        result.purchaseStatus = ShopPurchaseStatus::SoldOut;
        return result;
    }

    if (offer->bonusAttack > 0 && stats.attackPower >= 1 + offer->maxPurchaseCount) {
        result.text = "Nail Edge is sold out.";
        result.purchaseStatus = ShopPurchaseStatus::SoldOut;
        return result;
    }

    if (offer->bonusAttackSpeed > 0 && stats.attackSpeedLevel >= 1 + offer->maxPurchaseCount) {
        result.text = "Attack speed upgrade is sold out.";
        result.purchaseStatus = ShopPurchaseStatus::SoldOut;
        return result;
    }

    stats.hkd -= offer->hkdCost;
    result.hkdSpent = offer->hkdCost;
    result.purchaseStatus = ShopPurchaseStatus::Success;

    if (offer->bonusHealth > 0) {
        stats.purchasedHealthSlots += offer->bonusHealth;
        stats.health.maximum += offer->bonusHealth;
        stats.health.current = std::min(stats.health.maximum, stats.health.current + offer->bonusHealth);
        result.bonusHealthApplied = offer->bonusHealth;
    }

    if (offer->bonusAttack > 0) {
        stats.attackPower += offer->bonusAttack;
        result.bonusAttackApplied = offer->bonusAttack;
    }

    if (offer->bonusAttackSpeed > 0) {
        stats.attackSpeedLevel += offer->bonusAttackSpeed;
        result.bonusAttackSpeedApplied = offer->bonusAttackSpeed;
    }

    if (offer->unlocksSkill) {
        result.unlockedSkillGranted = true;
        result.grantedSkill = offer->unlockedSkill;
        result.text = "Bought " + offer->name + ". Unlock marker returned; main flow and save hook still pending.";
        return result;
    }

    result.text = "Bought " + offer->name + ".";
    return result;
}

DoctorNpc::DoctorNpc()
    : Npc("doctor", "Doctor", NpcType::Doctor) {
}

std::vector<DialogueLine> DoctorNpc::getDialogue() const {
    std::vector<DialogueLine> dialogue;
    dialogue.push_back(DialogueLine{
            "Doctor",
            "Hold still. I will patch you up."});
    return dialogue;
}

NpcInteractionResult DoctorNpc::interact(CharacterStats& stats) const {
    NpcInteractionResult result;
    result.type = NpcInteractionType::Heal;
    result.speaker = getDisplayName();

    const int missingHealth = std::max(0, stats.health.maximum - stats.health.current);
    healToFull(stats);
    result.healedToFull = true;
    result.healthRestored = missingHealth;

    if (missingHealth > 0) {
        result.text = "Fully healed.";
    } else {
        result.text = "You are already at full health.";
    }
    return result;
}

void DoctorNpc::healToFull(CharacterStats& stats) const {
    stats.health.current = stats.health.maximum;
}

LoreNpc::LoreNpc(const std::string& idValue,
                 const std::string& displayNameValue,
                 const std::vector<DialogueLine>& dialogueValue,
                 NpcType npcTypeValue)
    : Npc(idValue, displayNameValue, npcTypeValue),
      dialogue(dialogueValue) {
}

std::vector<DialogueLine> LoreNpc::getDialogue() const {
    return dialogue;
}

VillageChiefNpc::VillageChiefNpc()
    : LoreNpc("village_chief", "Chief", buildVillageChiefDialogue(), NpcType::VillageChief) {
}

std::vector<DialogueLine> VillageChiefNpc::getDialogue() const {
    return dialogue;
}

NpcInteractionResult VillageChiefNpc::interact(CharacterStats& stats) const {
    return Npc::interact(stats);
}

AmbientNpc::AmbientNpc(const std::string& idValue,
                       const std::string& displayNameValue,
                       const std::string& textValue)
    : Npc(idValue, displayNameValue, NpcType::Ambient),
      text(textValue) {
}

std::vector<DialogueLine> AmbientNpc::getDialogue() const {
    std::vector<DialogueLine> dialogue;
    dialogue.push_back(DialogueLine{getDisplayName(), text});
    return dialogue;
}

const Npc& getNpcDefinition(const std::string& npcId) {
    static const Merchant merchant;
    static const DoctorNpc doctor;
    static const VillageChiefNpc chief;
    static const AmbientNpc eventMarker("event_marker",
                                        "Event Marker",
                                        "Event area placeholder.");
    static const AmbientNpc generic("ambient_npc",
                                    "NPC",
                                    "NPC placeholder.");

    if (npcId == "merchant" || npcId == "merchant_shop") {
        return merchant;
    }
    if (npcId == "doctor") {
        return doctor;
    }
    if (npcId == "village_chief" || npcId == "village_elder") {
        return chief;
    }
    if (npcId == "event_marker") {
        return eventMarker;
    }
    return generic;
}

NpcInteractionResult interactWithNpc(const std::string& npcId, CharacterStats& stats) {
    return getNpcDefinition(npcId).interact(stats);
}

NpcInteractionResult purchaseNpcOffer(const std::string& npcId,
                                      const std::string& offerId,
                                      CharacterStats& stats) {
    return getNpcDefinition(npcId).buyOffer(offerId, stats);
}

} // namespace game
