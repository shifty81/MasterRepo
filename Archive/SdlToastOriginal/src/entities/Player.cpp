#include "Player.h"
#include "../engine/Input.h"
#include "../engine/Renderer.h"
#include "../engine/SpriteSheet.h"
#include <raylib.h>

Player::Player()
    : Entity()
    , m_health(3)
    , m_maxHealth(3)
    , m_velocityX(0.0f)
    , m_velocityY(0.0f)
    , m_facing(Direction::DOWN)
{
    m_width = 32.0f;
    m_height = 32.0f;
}

void Player::Update(float /*deltaTime*/) {
    // Base update (called from Player::Update with input)
}

void Player::Update(float deltaTime, Input* input) {
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;

    // Movement input
    if (input->IsKeyDown(KEY_W) || input->IsKeyDown(KEY_UP)) {
        m_velocityY = -MOVE_SPEED;
        m_facing = Direction::UP;
    }
    else if (input->IsKeyDown(KEY_S) || input->IsKeyDown(KEY_DOWN)) {
        m_velocityY = MOVE_SPEED;
        m_facing = Direction::DOWN;
    }

    if (input->IsKeyDown(KEY_A) || input->IsKeyDown(KEY_LEFT)) {
        m_velocityX = -MOVE_SPEED;
        m_facing = Direction::LEFT;
    }
    else if (input->IsKeyDown(KEY_D) || input->IsKeyDown(KEY_RIGHT)) {
        m_velocityX = MOVE_SPEED;
        m_facing = Direction::RIGHT;
    }

    // Normalize diagonal movement
    if (m_velocityX != 0.0f && m_velocityY != 0.0f) {
        m_velocityX *= 0.707f; // 1/sqrt(2)
        m_velocityY *= 0.707f;
    }

    // Update position
    m_x += m_velocityX * deltaTime;
    m_y += m_velocityY * deltaTime;

    // Keep player on screen (temporary bounds)
    if (m_x < 0) m_x = 0;
    if (m_y < 0) m_y = 0;
    if (m_x > 800 - m_width) m_x = 800 - m_width;
    if (m_y > 600 - m_height) m_y = 600 - m_height;
}

void Player::Render(Renderer* renderer) {
    if (!m_active) return;

    // Try to use character sprite sheet if available
    SpriteSheet* charSheet = SpriteSheetManager::Instance().GetSpriteSheet("characters");
    
    if (charSheet && charSheet->IsLoaded()) {
        // Use sprite sheet - get tile based on direction
        int spriteId = GetCharacterSpriteId();
        charSheet->RenderTile(renderer, spriteId, 
                             static_cast<int>(m_x), 
                             static_cast<int>(m_y),
                             static_cast<int>(m_width),
                             static_cast<int>(m_height));
    } else {
        // Fallback: colored rectangle (current method)
        RenderFallback(renderer);
    }
}

void Player::RenderFallback(Renderer* renderer) {
    // Color changes based on facing direction for debugging
    unsigned char r = 50, g = 50, b = 200;
    
    switch (m_facing) {
        case Direction::UP:    r = 50;  g = 200; b = 50;  break;
        case Direction::DOWN:  r = 50;  g = 50;  b = 200; break;
        case Direction::LEFT:  r = 200; g = 50;  b = 50;  break;
        case Direction::RIGHT: r = 200; g = 200; b = 50;  break;
    }

    renderer->FillRect(
        static_cast<int>(m_x),
        static_cast<int>(m_y),
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        r, g, b
    );
}

int Player::GetCharacterSpriteId() const {
    // Map facing direction to sprite sheet tile ID
    // Assumes sprite sheet layout:
    // Row 0: Down-facing animation
    // Row 1: Up-facing animation  
    // Row 2: Left-facing animation
    // Row 3: Right-facing animation
    
    int baseId = 0;
    switch (m_facing) {
        case Direction::DOWN:  baseId = 0; break;
        case Direction::UP:    baseId = 4; break;
        case Direction::LEFT:  baseId = 8; break;
        case Direction::RIGHT: baseId = 12; break;
    }
    
    // For now, use first frame (idle)
    // TODO: Add animation based on movement
    return baseId;
}
