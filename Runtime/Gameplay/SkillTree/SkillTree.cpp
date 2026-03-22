#include "Runtime/Gameplay/SkillTree/SkillTree.h"

namespace Runtime {

void SkillTree::Init() { m_nodes.clear(); m_nextId = 1; }

SkillID SkillTree::AddNode(const std::string& name,
                            const std::vector<SkillID>& prereqs,
                            const std::vector<SkillEffect>& effects) {
    SkillID id = m_nextId++;
    SkillNode& node = m_nodes[id];
    node.id            = id;
    node.name          = name;
    node.prerequisites = prereqs;
    node.effects       = effects;
    node.unlocked      = false;
    return id;
}

bool SkillTree::Unlock(SkillID id) {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return false;
    if (it->second.unlocked) return true; // already unlocked
    if (!CanUnlock(id)) return false;
    it->second.unlocked = true;
    return true;
}

bool SkillTree::IsUnlocked(SkillID id) const {
    auto it = m_nodes.find(id);
    return it != m_nodes.end() && it->second.unlocked;
}

bool SkillTree::CanUnlock(SkillID id) const {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) return false;
    if (it->second.unlocked) return false; // already done
    for (SkillID pre : it->second.prerequisites) {
        if (!IsUnlocked(pre)) return false;
    }
    return true;
}

std::vector<SkillEffect> SkillTree::GetEffects(SkillID id) const {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end() || !it->second.unlocked) return {};
    return it->second.effects;
}

const SkillNode* SkillTree::GetNode(SkillID id) const {
    auto it = m_nodes.find(id);
    return it != m_nodes.end() ? &it->second : nullptr;
}

size_t SkillTree::NodeCount() const { return m_nodes.size(); }

std::vector<SkillID> SkillTree::UnlockedSkills() const {
    std::vector<SkillID> out;
    for (auto& [id, node] : m_nodes) if (node.unlocked) out.push_back(id);
    return out;
}

std::vector<SkillID> SkillTree::AvailableToUnlock() const {
    std::vector<SkillID> out;
    for (auto& [id, _] : m_nodes) if (CanUnlock(id)) out.push_back(id);
    return out;
}

void SkillTree::Reset() {
    for (auto& [_, node] : m_nodes) node.unlocked = false;
}

} // namespace Runtime
