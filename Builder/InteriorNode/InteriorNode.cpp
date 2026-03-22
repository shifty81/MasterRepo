#include "Builder/InteriorNode/InteriorNode.h"
#include <algorithm>

namespace Builder {

size_t InteriorNode::AddSlot(const ModuleSlot& slot) {
    m_slots.push_back(slot);
    m_modules.emplace_back(); // empty placeholder
    return m_slots.size() - 1;
}

bool InteriorNode::PlaceModule(size_t idx, const InteriorModule& mod) {
    if (idx >= m_slots.size()) return false;
    auto& slot = m_slots[idx];
    if (slot.occupied) return false;
    // Check type is allowed
    if (!slot.allowedTypes.empty()) {
        bool ok = false;
        for (auto t : slot.allowedTypes) if (t == mod.type) { ok = true; break; }
        if (!ok) return false;
    }
    slot.occupied    = true;
    slot.occupantId  = mod.id;
    m_modules[idx]   = mod;
    return true;
}

void InteriorNode::RemoveModule(size_t idx) {
    if (idx >= m_slots.size()) return;
    m_slots[idx].occupied   = false;
    m_slots[idx].occupantId = 0;
    m_modules[idx]          = {};
}

const InteriorModule* InteriorNode::GetModule(size_t idx) const {
    if (idx >= m_slots.size() || !m_slots[idx].occupied) return nullptr;
    return &m_modules[idx];
}

const ModuleSlot* InteriorNode::GetSlot(size_t idx) const {
    if (idx >= m_slots.size()) return nullptr;
    return &m_slots[idx];
}

size_t InteriorNode::OccupiedSlotCount() const {
    size_t n = 0;
    for (auto& s : m_slots) if (s.occupied) ++n;
    return n;
}

size_t InteriorNode::TotalModuleCount() const { return OccupiedSlotCount(); }

std::vector<size_t> InteriorNode::FindSlotsByType(ModuleType type) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < m_slots.size(); ++i) {
        for (auto t : m_slots[i].allowedTypes) {
            if (t == type) { out.push_back(i); break; }
        }
    }
    return out;
}

std::vector<size_t> InteriorNode::FindSlotsBySize(ModuleSize size) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < m_slots.size(); ++i)
        if (m_slots[i].size == size) out.push_back(i);
    return out;
}

std::vector<InteriorModule> InteriorNode::GetModulesByType(ModuleType type) const {
    std::vector<InteriorModule> out;
    for (size_t i = 0; i < m_slots.size(); ++i)
        if (m_slots[i].occupied && m_modules[i].type == type) out.push_back(m_modules[i]);
    return out;
}

bool InteriorNode::HasPower() const {
    for (size_t i = 0; i < m_slots.size(); ++i)
        if (m_slots[i].occupied && m_modules[i].type == ModuleType::PowerPlant
            && m_modules[i].IsOperational()) return true;
    return false;
}

void InteriorNode::Clear() {
    for (size_t i = 0; i < m_slots.size(); ++i) {
        m_slots[i].occupied = false; m_slots[i].occupantId = 0;
        m_modules[i] = {};
    }
}

} // namespace Builder
