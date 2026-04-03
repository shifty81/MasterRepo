#include "Game/Gameplay/Storage/StorageSystem.h"
#include <algorithm>
#include <cmath>

namespace NF::Game::Gameplay {

// ---- private helpers --------------------------------------------------------

StorageSystem::BoxEntry* StorageSystem::FindEntry(StorageBoxId id) noexcept {
    for (auto& b : m_Boxes) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

const StorageSystem::BoxEntry* StorageSystem::FindEntry(StorageBoxId id) const noexcept {
    for (const auto& b : m_Boxes) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

// ---- box management ---------------------------------------------------------

StorageBoxId StorageSystem::AddBox(const std::string& label, StoragePos pos) {
    const StorageBoxId id = m_NextId++;
    m_Boxes.push_back({ id, label, pos, NF::Game::Inventory{} });
    return id;
}

bool StorageSystem::RemoveBox(StorageBoxId id) {
    auto it = std::find_if(m_Boxes.begin(), m_Boxes.end(),
                           [id](const BoxEntry& b){ return b.id == id; });
    if (it == m_Boxes.end()) return false;
    m_Boxes.erase(it);
    return true;
}

NF::Game::Inventory* StorageSystem::GetBox(StorageBoxId id) noexcept {
    BoxEntry* e = FindEntry(id);
    return e ? &e->inventory : nullptr;
}

const NF::Game::Inventory* StorageSystem::GetBox(StorageBoxId id) const noexcept {
    const BoxEntry* e = FindEntry(id);
    return e ? &e->inventory : nullptr;
}

// ---- spatial lookup ---------------------------------------------------------

StorageBoxId StorageSystem::FindNearest(StoragePos pos, float radius) const noexcept {
    const float radiusSq = radius * radius;
    StorageBoxId best = kInvalidBoxId;
    float bestDist = radiusSq + 1.f;

    for (const auto& b : m_Boxes) {
        const float d = b.pos.DistanceSqTo(pos);
        if (d <= radiusSq && d < bestDist) {
            bestDist = d;
            best     = b.id;
        }
    }
    return best;
}

// ---- transfer helpers -------------------------------------------------------

bool StorageSystem::Deposit(StorageBoxId id, NF::Game::Inventory& src,
                             NF::Game::ResourceType type, uint32_t count)
{
    NF::Game::Inventory* box = GetBox(id);
    if (!box) return false;
    if (!src.RemoveItem(type, count)) return false;
    if (!box->AddItem(type, count)) {
        src.AddItem(type, count); // rollback
        return false;
    }
    return true;
}

bool StorageSystem::Withdraw(StorageBoxId id, NF::Game::Inventory& dst,
                              NF::Game::ResourceType type, uint32_t count)
{
    NF::Game::Inventory* box = GetBox(id);
    if (!box) return false;
    if (!box->RemoveItem(type, count)) return false;
    if (!dst.AddItem(type, count)) {
        box->AddItem(type, count); // rollback
        return false;
    }
    return true;
}

// ---- lifecycle --------------------------------------------------------------

void StorageSystem::Clear() noexcept {
    m_Boxes.clear();
    m_NextId = 1;
}

} // namespace NF::Game::Gameplay
