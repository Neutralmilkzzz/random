#include <iostream>

#include "combat/CombatSystem.h"

int main() {
    game::AttackDefinition basicAttack;
    basicAttack.id = "player_basic_attack";
    basicAttack.damage = game::DamageInfo(1, game::DamageType::BasicAttack, "player", true);
    basicAttack.soulGainOnHit = 11;
    basicAttack.startupSeconds = 0.10f;
    basicAttack.activeSeconds = 0.12f;
    basicAttack.recoverySeconds = 0.20f;
    basicAttack.knockbackX = 1;

    game::AttackDefinition fireball;
    fireball.id = "flying_enemy_fireball";
    fireball.damage = game::DamageInfo(1, game::DamageType::Fireball, "flying_enemy", false);
    fireball.startupSeconds = 0.30f;
    fireball.activeSeconds = 1.20f;

    std::cout << "[CombatSandbox] attack definition scaffold\n";
    std::cout << "Attack: " << basicAttack.id << " | damage: " << basicAttack.damage.amount
              << " | soul gain on hit: " << basicAttack.soulGainOnHit << "\n";
    std::cout << "Attack: " << fireball.id << " | damage: " << fireball.damage.amount
              << " | startup: " << fireball.startupSeconds << "s\n";
    std::cout << "\nNext step: plug concrete CombatActor / Enemy objects into this sandbox.\n";

    return 0;
}
