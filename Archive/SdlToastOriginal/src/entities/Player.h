#ifndef PLAYER_H
#define PLAYER_H

#include "Entity.h"

class Input;

class Player : public Entity {
public:
    Player();
    ~Player() override = default;

    void Update(float deltaTime) override;
    void Update(float deltaTime, Input* input);
    void Render(Renderer* renderer) override;

    void SetHealth(int health) { m_health = health; }
    int GetHealth() const { return m_health; }
    
    void SetMaxHealth(int maxHealth) { m_maxHealth = maxHealth; }
    int GetMaxHealth() const { return m_maxHealth; }

private:
    static constexpr float MOVE_SPEED = 150.0f;
    
    int m_health;
    int m_maxHealth;
    float m_velocityX, m_velocityY;
    
    enum class Direction { DOWN, UP, LEFT, RIGHT };
    Direction m_facing;
    
    // Helper methods
    void RenderFallback(Renderer* renderer);
    int GetCharacterSpriteId() const;
};

#endif // PLAYER_H
