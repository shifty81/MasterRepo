#ifndef NPC_H
#define NPC_H

#include "Entity.h"
#include "../systems/Dialogue.h"
#include <string>
#include <vector>

struct ScheduleEntry {
    int hour;         // Hour of day (0-23) this entry activates
    float destX;      // Destination X
    float destY;      // Destination Y
};

class NPC : public Entity {
public:
    NPC();
    ~NPC() override = default;

    void Update(float deltaTime) override;
    void Render(Renderer* renderer) override;

    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    // Friendship
    int GetFriendship() const { return m_friendshipLevel; }
    void AddFriendship(int amount);

    // Schedule
    void AddScheduleEntry(int hour, float x, float y);
    void SetCurrentHour(int hour);

    // Dialogue
    Dialogue& GetDialogue() { return m_dialogue; }
    const Dialogue& GetDialogue() const { return m_dialogue; }

    // Interaction
    bool IsPlayerNearby(float playerX, float playerY) const;

private:
    std::string m_name;
    int m_friendshipLevel;

    // Schedule
    std::vector<ScheduleEntry> m_schedule;
    float m_moveSpeed;
    float m_destX, m_destY;
    bool m_moving;

    // Dialogue
    Dialogue m_dialogue;

    static constexpr float INTERACT_RANGE = 48.0f;
    static constexpr float ARRIVAL_THRESHOLD = 2.0f;
    static constexpr int MAX_FRIENDSHIP = 10;
};

#endif // NPC_H
