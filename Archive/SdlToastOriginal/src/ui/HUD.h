#ifndef HUD_H
#define HUD_H

#include <string>
#include <vector>

class Renderer;

class HUD {
public:
    HUD() = default;
    ~HUD() = default;

    void Render(Renderer* renderer);
    
    void SetHealth(int current, int max) { m_currentHealth = current; m_maxHealth = max; }
    void SetGold(int gold) { m_gold = gold; }
    void SetEnergy(int current, int max) { m_currentEnergy = current; m_maxEnergy = max; }
    void SetDayInfo(int day, const std::string& season, int year);
    void SetActionText(const std::string& text) { m_actionText = text; }
    void SetShowInventory(bool show) { m_showInventory = show; }

    // Inventory display helper
    void AddInventoryLine(const std::string& line);
    void ClearInventoryLines();

private:
    int m_currentHealth = 3;
    int m_maxHealth = 3;
    int m_gold = 0;
    int m_currentEnergy = 100;
    int m_maxEnergy = 100;
    int m_day = 1;
    std::string m_season = "Spring";
    int m_year = 1;
    std::string m_actionText;
    bool m_showInventory = false;
    std::vector<std::string> m_inventoryLines;
};

#endif // HUD_H
