#include "Builder/Modules/ModuleDef.h"

namespace Builder {

size_t ModuleNode::AddSlot(const ModuleSlot& slot) {
    m_slots.push_back(slot);
    m_modules.push_back({});
    return m_slots.size() - 1;
}

bool ModuleNode::PlaceModule(size_t slotIndex, const ModuleInstance& mod) {
    if (slotIndex >= m_slots.size()) return false;
    ModuleSlot& slot = m_slots[slotIndex];
    if (slot.occupied) return false;
    if (!slot.allowedTypes.empty()) {
        bool ok = false;
        for (auto t : slot.allowedTypes) if (t == mod.type) { ok = true; break; }
        if (!ok) return false;
    }
    if (slot.size != mod.size) return false;
    slot.occupied   = true;
    slot.occupantId = mod.id;
    m_modules[slotIndex] = mod;
    return true;
}

void ModuleNode::RemoveModule(size_t slotIndex) {
    if (slotIndex >= m_slots.size()) return;
    m_slots[slotIndex].occupied   = false;
    m_slots[slotIndex].occupantId = 0;
    m_modules[slotIndex] = {};
}

const ModuleInstance* ModuleNode::GetModule(size_t slotIndex) const {
    if (slotIndex >= m_slots.size() || !m_slots[slotIndex].occupied) return nullptr;
    return &m_modules[slotIndex];
}

const ModuleSlot* ModuleNode::GetSlot(size_t index) const {
    if (index >= m_slots.size()) return nullptr;
    return &m_slots[index];
}

size_t ModuleNode::OccupiedSlotCount() const {
    size_t n = 0;
    for (const auto& s : m_slots) if (s.occupied) ++n;
    return n;
}

std::vector<size_t> ModuleNode::FindSlotsByType(ModuleType type) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < m_slots.size(); ++i)
        for (auto t : m_slots[i].allowedTypes)
            if (t == type) { out.push_back(i); break; }
    return out;
}

std::vector<size_t> ModuleNode::FindSlotsBySize(ModuleSize size) const {
    std::vector<size_t> out;
    for (size_t i = 0; i < m_slots.size(); ++i)
        if (m_slots[i].size == size) out.push_back(i);
    return out;
}

size_t ModuleNode::TotalModuleCount() const { return OccupiedSlotCount(); }

std::vector<ModuleInstance> ModuleNode::GetModulesByType(ModuleType type) const {
    std::vector<ModuleInstance> out;
    for (size_t i = 0; i < m_slots.size(); ++i)
        if (m_slots[i].occupied && m_modules[i].type == type)
            out.push_back(m_modules[i]);
    return out;
}

void ModuleNode::Clear() {
    for (auto& s : m_slots) { s.occupied = false; s.occupantId = 0; }
    for (auto& m : m_modules) m = {};
}

bool ModuleNode::HasPower() const {
    for (size_t i = 0; i < m_slots.size(); ++i)
        if (m_slots[i].occupied && m_modules[i].type == ModuleType::PowerPlant
            && m_modules[i].IsOperational())
            return true;
    return false;
}

} // namespace Builder
