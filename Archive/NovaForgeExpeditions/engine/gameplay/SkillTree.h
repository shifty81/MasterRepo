#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace atlas::gameplay {

using SkillID = uint32_t;

enum class ModifierType : uint8_t {
    Add,
    Multiply,
    Clamp
};

struct SkillEffect {
    std::string systemName;
    ModifierType modifierType = ModifierType::Add;
    float value = 0.0f;
};

struct SkillNode {
    SkillID id = 0;
    std::string name;
    std::vector<SkillID> prerequisites;
    std::vector<SkillEffect> effects;
    bool unlocked = false;
};

class SkillTree {
public:
    void Init();
    SkillID AddNode(const std::string& name,
                    const std::vector<SkillID>& prerequisites = {},
                    const std::vector<SkillEffect>& effects = {});
    bool Unlock(SkillID id);
    bool IsUnlocked(SkillID id) const;
    bool CanUnlock(SkillID id) const;
    std::vector<SkillEffect> GetEffects(SkillID id) const;
    size_t NodeCount() const;
    const SkillNode* GetNode(SkillID id) const;

private:
    std::unordered_map<SkillID, SkillNode> m_nodes;
    SkillID m_nextId = 1;
};

}
