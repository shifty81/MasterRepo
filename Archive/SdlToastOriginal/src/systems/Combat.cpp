#include "Combat.h"
#include "../entities/Player.h"
#include "../entities/Enemy.h"

int Combat::CalculateDamage(int attackPower, int defense) {
    int damage = attackPower - defense;
    return damage > 0 ? damage : 0;
}

bool Combat::CheckCollision(float x1, float y1, float w1, float h1,
                            float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 &&
            y1 < y2 + h2 && y1 + h1 > y2);
}

bool Combat::PlayerAttack(Player* player, Enemy* enemy, int attackPower) {
    if (!player || !enemy || !enemy->IsActive()) return false;

    float px, py, pw, ph;
    player->GetPosition(px, py);
    player->GetSize(pw, ph);

    float ex, ey, ew, eh;
    enemy->GetPosition(ex, ey);
    enemy->GetSize(ew, eh);

    // Attack range is slightly larger than the player hitbox
    float ax = px - ATTACK_RANGE / 2;
    float ay = py - ATTACK_RANGE / 2;
    float aw = pw + ATTACK_RANGE;
    float ah = ph + ATTACK_RANGE;

    if (CheckCollision(ax, ay, aw, ah, ex, ey, ew, eh)) {
        int damage = CalculateDamage(attackPower, 0);
        int newHealth = enemy->GetHealth() - damage;
        enemy->SetHealth(newHealth);
        if (newHealth <= 0) {
            enemy->SetActive(false);
        }
        return true;
    }
    return false;
}

bool Combat::EnemyContact(Enemy* enemy, Player* player, int attackPower) {
    if (!enemy || !player || !enemy->IsActive()) return false;

    float px, py, pw, ph;
    player->GetPosition(px, py);
    player->GetSize(pw, ph);

    float ex, ey, ew, eh;
    enemy->GetPosition(ex, ey);
    enemy->GetSize(ew, eh);

    if (CheckCollision(px, py, pw, ph, ex, ey, ew, eh)) {
        int damage = CalculateDamage(attackPower, 0);
        int newHealth = player->GetHealth() - damage;
        player->SetHealth(newHealth > 0 ? newHealth : 0);
        return true;
    }
    return false;
}
