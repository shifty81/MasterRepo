#include "Game/Gameplay/Inventory/InventorySystem.h"
#include <algorithm>

namespace NF::Game::Gameplay {

// ---- private helpers --------------------------------------------------------

InventorySystem::Entry* InventorySystem::FindEntry(ContainerId id) noexcept {
    for (auto& e : m_Containers) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

const InventorySystem::Entry* InventorySystem::FindEntry(ContainerId id) const noexcept {
    for (const auto& e : m_Containers) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

// ---- container management ---------------------------------------------------

ContainerId InventorySystem::AddContainer(const std::string& name) {
    const ContainerId id = m_NextId++;
    m_Containers.push_back({ id, name, NF::Game::Inventory{} });
    return id;
}

bool InventorySystem::RemoveContainer(ContainerId id) {
    auto it = std::find_if(m_Containers.begin(), m_Containers.end(),
                           [id](const Entry& e){ return e.id == id; });
    if (it == m_Containers.end()) return false;
    m_Containers.erase(it);
    return true;
}

NF::Game::Inventory* InventorySystem::GetContainer(ContainerId id) noexcept {
    Entry* e = FindEntry(id);
    return e ? &e->inventory : nullptr;
}

const NF::Game::Inventory* InventorySystem::GetContainer(ContainerId id) const noexcept {
    const Entry* e = FindEntry(id);
    return e ? &e->inventory : nullptr;
}

ContainerId InventorySystem::FindContainer(const std::string& name) const noexcept {
    for (const auto& e : m_Containers) {
        if (e.name == name) return e.id;
    }
    return kInvalidContainer;
}

// ---- transfer ---------------------------------------------------------------

bool InventorySystem::Transfer(ContainerId from, ContainerId to,
                                NF::Game::ResourceType type, uint32_t count)
{
    NF::Game::Inventory* src = GetContainer(from);
    NF::Game::Inventory* dst = GetContainer(to);
    if (!src || !dst) return false;
    if (!src->RemoveItem(type, count)) return false;
    if (!dst->AddItem(type, count)) {
        // Rollback: put items back into source.
        src->AddItem(type, count);
        return false;
    }
    return true;
}

// ---- lifecycle --------------------------------------------------------------

void InventorySystem::Clear() noexcept {
    m_Containers.clear();
    m_NextId = 1;
}

} // namespace NF::Game::Gameplay
