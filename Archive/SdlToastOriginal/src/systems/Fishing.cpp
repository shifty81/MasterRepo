#include "Fishing.h"
#include "../systems/Calendar.h"
#include <cstdlib>

FishingSystem::FishingSystem() {
    InitFish();
}

void FishingSystem::InitFish() {
    // Name, value, difficulty(1-10), spring, summer, fall, winter
    m_fish.push_back({"Sunfish",       30,  2, true,  true,  false, false});
    m_fish.push_back({"Catfish",       50,  4, true,  false, true,  false});
    m_fish.push_back({"Bass",          40,  3, true,  true,  true,  false});
    m_fish.push_back({"Trout",         45,  3, false, true,  true,  false});
    m_fish.push_back({"Salmon",        75,  5, false, false, true,  false});
    m_fish.push_back({"Carp",          20,  1, true,  true,  true,  true });
    m_fish.push_back({"Eel",           60,  6, false, false, true,  true });
    m_fish.push_back({"Pike",          70,  5, false, true,  false, true });
    m_fish.push_back({"Ice Perch",     55,  4, false, false, false, true });
    m_fish.push_back({"Golden Trout", 150,  9, false, true,  false, false});
}

std::vector<const FishType*> FishingSystem::GetAvailableFish(Season season) const {
    std::vector<const FishType*> result;
    for (const auto& fish : m_fish) {
        bool avail = false;
        switch (season) {
            case Season::SPRING: avail = fish.springAvail; break;
            case Season::SUMMER: avail = fish.summerAvail; break;
            case Season::FALL:   avail = fish.fallAvail;   break;
            case Season::WINTER: avail = fish.winterAvail; break;
        }
        if (avail) {
            result.push_back(&fish);
        }
    }
    return result;
}

int FishingSystem::AttemptCatch(Season season, int skillLevel) const {
    int roll = std::rand() % 100;
    return AttemptCatchWithRoll(season, skillLevel, roll);
}

int FishingSystem::AttemptCatchWithRoll(Season season, int skillLevel, int roll) const {
    auto available = GetAvailableFish(season);
    if (available.empty()) return -1;

    // Base success chance: 40% + 5% per skill level, capped at 90%
    int successChance = 40 + skillLevel * 5;
    if (successChance > 90) successChance = 90;

    if (roll >= successChance) return -1;  // Miss

    // Pick a fish based on difficulty vs skill.
    // Higher skill makes harder fish more likely to be caught.
    // Use roll to deterministically select from available fish.
    int fishRoll = roll % static_cast<int>(available.size());

    // Check if the fish difficulty is within range (skill + 3)
    const FishType* picked = available[fishRoll];
    if (picked->difficulty > skillLevel + 3) {
        // Too hard — fall back to easiest available
        const FishType* easiest = available[0];
        for (const auto* f : available) {
            if (f->difficulty < easiest->difficulty) easiest = f;
        }
        picked = easiest;
    }

    // Find original index in m_fish
    for (int i = 0; i < static_cast<int>(m_fish.size()); ++i) {
        if (&m_fish[i] == picked) return i;
    }

    return -1;
}

const FishType* FishingSystem::GetFish(int index) const {
    if (index < 0 || index >= static_cast<int>(m_fish.size())) return nullptr;
    return &m_fish[index];
}
