#include "Enemy.h"
#include "../engine/Renderer.h"
#include <cmath>
#include <random>

Enemy::Enemy()
    : Entity()
    , m_health(3)
    , m_speed(50.0f)
    , m_aiState(AIState::PATROL)
    , m_patrolOriginX(0.0f)
    , m_patrolOriginY(0.0f)
    , m_patrolTargetX(0.0f)
    , m_patrolTargetY(0.0f)
    , m_patrolTimer(0.0f)
    , m_targetX(0.0f)
    , m_targetY(0.0f)
{
}

void Enemy::Update(float deltaTime) {
    if (!m_active) return;

    switch (m_aiState) {
        case AIState::IDLE:
            break;

        case AIState::PATROL: {
            m_patrolTimer += deltaTime;
            if (m_patrolTimer >= PATROL_INTERVAL) {
                m_patrolTimer = 0.0f;
                // Pick a new random patrol target near origin
                static std::mt19937 rng(std::random_device{}());
                static constexpr float TWO_PI = 6.2831853f;
                std::uniform_real_distribution<float> dist(0.0f, TWO_PI);
                float angle = dist(rng);
                m_patrolTargetX = m_patrolOriginX + std::cos(angle) * PATROL_RADIUS;
                m_patrolTargetY = m_patrolOriginY + std::sin(angle) * PATROL_RADIUS;
            }

            // Move toward patrol target
            float dx = m_patrolTargetX - m_x;
            float dy = m_patrolTargetY - m_y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 2.0f) {
                m_x += (dx / dist) * m_speed * deltaTime;
                m_y += (dy / dist) * m_speed * deltaTime;
            }

            // Check if player is within chase range
            float pdx = m_targetX - m_x;
            float pdy = m_targetY - m_y;
            float playerDist = std::sqrt(pdx * pdx + pdy * pdy);
            if (playerDist < CHASE_RANGE) {
                m_aiState = AIState::CHASE;
            }
            break;
        }

        case AIState::CHASE: {
            float dx = m_targetX - m_x;
            float dy = m_targetY - m_y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > CHASE_RANGE * 1.5f) {
                // Lost the target, return to patrol
                m_aiState = AIState::PATROL;
                m_patrolTimer = 0.0f;
            } else if (dist > 2.0f) {
                m_x += (dx / dist) * m_speed * 1.3f * deltaTime;
                m_y += (dy / dist) * m_speed * 1.3f * deltaTime;
            }
            break;
        }
    }
}

void Enemy::Render(Renderer* renderer) {
    if (!m_active) return;

    // Color based on AI state
    unsigned char r = 200, g = 50, b = 50;
    if (m_aiState == AIState::CHASE) {
        r = 255; g = 80; b = 80;
    } else if (m_aiState == AIState::IDLE) {
        r = 150; g = 50; b = 50;
    }

    renderer->FillRect(
        static_cast<int>(m_x),
        static_cast<int>(m_y),
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        r, g, b
    );
}
