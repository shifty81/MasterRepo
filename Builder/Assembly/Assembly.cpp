#include "Builder/Assembly/Assembly.h"
#include <algorithm>

namespace Builder {

uint32_t Assembly::AddPart(uint32_t defId, const float transform[16]) {
    PlacedPart p;
    p.instanceId = m_nextId++;
    p.defId      = defId;
    for (int i = 0; i < 16; ++i) p.transform[i] = transform[i];
    m_parts.push_back(p);
    return p.instanceId;
}

void Assembly::RemovePart(uint32_t instanceId) {
    auto it = std::find_if(m_parts.begin(), m_parts.end(),
        [instanceId](const PlacedPart& p){ return p.instanceId == instanceId; });
    if (it == m_parts.end()) return;
    m_linkCount -= it->links.size();
    // Remove back-links from other parts
    for (auto& other : m_parts) {
        auto& lv = other.links;
        auto prev = lv.size();
        lv.erase(std::remove_if(lv.begin(), lv.end(),
            [instanceId](const AssemblyLink& l){
                return l.fromPartId == instanceId || l.toPartId == instanceId;
            }), lv.end());
        m_linkCount -= (prev - lv.size());
    }
    m_parts.erase(it);
}

bool Assembly::Link(uint32_t fromInstanceId, uint32_t fromSnapId,
                    uint32_t toInstanceId,   uint32_t toSnapId, bool weld) {
    PlacedPart* from = GetPart(fromInstanceId);
    PlacedPart* to   = GetPart(toInstanceId);
    if (!from || !to) return false;
    // Check snap points not already used
    for (const auto& l : from->links)
        if (l.fromPartId == fromInstanceId && l.fromSnapId == fromSnapId) return false;
    for (const auto& l : to->links)
        if (l.toPartId == toInstanceId && l.toSnapId == toSnapId) return false;

    AssemblyLink link{fromInstanceId, fromSnapId, toInstanceId, toSnapId, weld};
    from->links.push_back(link);
    ++m_linkCount;
    return true;
}

void Assembly::Unlink(uint32_t instanceId, uint32_t snapId) {
    for (auto& p : m_parts) {
        auto& lv = p.links;
        auto prev = lv.size();
        lv.erase(std::remove_if(lv.begin(), lv.end(),
            [instanceId, snapId](const AssemblyLink& l){
                return (l.fromPartId == instanceId && l.fromSnapId == snapId) ||
                       (l.toPartId   == instanceId && l.toSnapId   == snapId);
            }), lv.end());
        m_linkCount -= (prev - lv.size());
    }
}

PlacedPart* Assembly::GetPart(uint32_t instanceId) {
    for (auto& p : m_parts) if (p.instanceId == instanceId) return &p;
    return nullptr;
}
const PlacedPart* Assembly::GetPart(uint32_t instanceId) const {
    for (const auto& p : m_parts) if (p.instanceId == instanceId) return &p;
    return nullptr;
}

std::vector<AssemblyLink> Assembly::GetLinks(uint32_t instanceId) const {
    const PlacedPart* p = GetPart(instanceId);
    return p ? p->links : std::vector<AssemblyLink>{};
}

float Assembly::CalculateTotalMass(const std::vector<std::pair<uint32_t,float>>& defMasses) const {
    float total = 0.0f;
    for (const auto& p : m_parts)
        for (const auto& [defId, mass] : defMasses)
            if (defId == p.defId) { total += mass; break; }
    return total;
}

float Assembly::CalculateTotalHP(const std::vector<std::pair<uint32_t,float>>& defHP) const {
    float total = 0.0f;
    for (const auto& p : m_parts)
        for (const auto& [defId, hp] : defHP)
            if (defId == p.defId) { total += hp; break; }
    return total;
}

bool Assembly::Validate() const {
    // Each (instanceId, snapId) pair should appear at most once across all links
    std::vector<std::pair<uint32_t,uint32_t>> used;
    for (const auto& p : m_parts)
        for (const auto& l : p.links) {
            auto fa = std::make_pair(l.fromPartId, l.fromSnapId);
            auto ta = std::make_pair(l.toPartId,   l.toSnapId);
            if (std::find(used.begin(), used.end(), fa) != used.end()) return false;
            if (std::find(used.begin(), used.end(), ta) != used.end()) return false;
            used.push_back(fa);
            used.push_back(ta);
        }
    return true;
}

void Assembly::Clear() {
    m_parts.clear();
    m_linkCount = 0;
    m_nextId    = 1;
}

} // namespace Builder
