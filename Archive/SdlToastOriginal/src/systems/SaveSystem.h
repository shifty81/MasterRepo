#ifndef SAVESYSTEM_H
#define SAVESYSTEM_H

#include <string>

class Player;
class Inventory;
class Calendar;
class Energy;
class Skills;
class QuestSystem;

class SaveSystem {
public:
    // Save game state to a file. Returns true on success.
    static bool Save(const std::string& filepath,
                     const Player* player,
                     const Inventory* inventory,
                     const Calendar* calendar,
                     int gold);

    // Extended save with energy, skills, and quests.
    static bool Save(const std::string& filepath,
                     const Player* player,
                     const Inventory* inventory,
                     const Calendar* calendar,
                     int gold,
                     const Energy* energy,
                     const Skills* skills,
                     const QuestSystem* quests);

    // Load game state from a file. Returns true on success.
    static bool Load(const std::string& filepath,
                     Player* player,
                     Inventory* inventory,
                     Calendar* calendar,
                     int& gold);

    // Extended load with energy, skills, and quests.
    static bool Load(const std::string& filepath,
                     Player* player,
                     Inventory* inventory,
                     Calendar* calendar,
                     int& gold,
                     Energy* energy,
                     Skills* skills,
                     QuestSystem* quests);

    static constexpr const char* DEFAULT_SAVE_PATH = "saves/save.dat";
};

#endif // SAVESYSTEM_H
