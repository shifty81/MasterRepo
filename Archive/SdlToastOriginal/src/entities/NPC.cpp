#include "NPC.h"
#include "../engine/Renderer.h"
#include <cmath>

NPC::NPC()
    : Entity()
    , m_name("NPC")
    , m_friendshipLevel(0)
    , m_moveSpeed(60.0f)
    , m_destX(0.0f)
    , m_destY(0.0f)
    , m_moving(false)
{
    m_width = 32.0f;
    m_height = 32.0f;
}

void NPC::AddFriendship(int amount) {
    m_friendshipLevel += amount;
    if (m_friendshipLevel > MAX_FRIENDSHIP) m_friendshipLevel = MAX_FRIENDSHIP;
    if (m_friendshipLevel < 0) m_friendshipLevel = 0;
}

void NPC::AddScheduleEntry(int hour, float x, float y) {
    m_schedule.push_back({hour, x, y});
}

void NPC::SetCurrentHour(int hour) {
    // Find the latest schedule entry whose hour <= current hour
    const ScheduleEntry* best = nullptr;
    for (const auto& entry : m_schedule) {
        if (entry.hour <= hour) {
            if (!best || entry.hour > best->hour) {
                best = &entry;
            }
        }
    }
    if (best) {
        m_destX = best->destX;
        m_destY = best->destY;
        // Only set moving if we're not already there
        float dx = m_destX - m_x;
        float dy = m_destY - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);
        m_moving = (dist > ARRIVAL_THRESHOLD);
    }
}

void NPC::Update(float deltaTime) {
    if (!m_active) return;

    if (m_moving) {
        float dx = m_destX - m_x;
        float dy = m_destY - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= ARRIVAL_THRESHOLD) {
            m_moving = false;
            m_x = m_destX;
            m_y = m_destY;
        } else {
            // Normalize and move
            float nx = dx / dist;
            float ny = dy / dist;
            m_x += nx * m_moveSpeed * deltaTime;
            m_y += ny * m_moveSpeed * deltaTime;
        }
    }
}

void NPC::Render(Renderer* renderer) {
    if (!m_active) return;
    // Color based on friendship level (greener = friendlier)
    int g = 100 + (m_friendshipLevel * 15);
    if (g > 255) g = 255;
    renderer->FillRect(
        static_cast<int>(m_x),
        static_cast<int>(m_y),
        static_cast<int>(m_width),
        static_cast<int>(m_height),
        100, g, 100
    );
}

bool NPC::IsPlayerNearby(float playerX, float playerY) const {
    float dx = playerX - m_x;
    float dy = playerY - m_y;
    float dist = std::sqrt(dx * dx + dy * dy);
    return dist <= INTERACT_RANGE;
}
