#include "Engine/Decals/DecalSystem/DecalSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <unordered_map>

namespace Engine {

// ── Impl ─────────────────────────────────────────────────────────────────
struct DecalSystem::Impl {
    std::vector<Decal> pool;
    uint32_t           maxDecals{512};
    uint32_t           nextId{1};
    uint32_t           expiredThisFrame{0};

    Decal* find(DecalId id) {
        for (auto& d : pool) if (d.id == id && d.active) return &d;
        return nullptr;
    }
    const Decal* find(DecalId id) const {
        for (const auto& d : pool) if (d.id == id && d.active) return &d;
        return nullptr;
    }
    Decal* freeSlot() {
        for (auto& d : pool) if (!d.active) return &d;
        if (pool.size() < maxDecals) { pool.emplace_back(); return &pool.back(); }
        return nullptr;
    }
};

DecalSystem::DecalSystem() : m_impl(new Impl()) {}
DecalSystem::DecalSystem(uint32_t maxDecals) : m_impl(new Impl()) {
    m_impl->maxDecals = std::min(maxDecals, kHardLimit);
    m_impl->pool.reserve(m_impl->maxDecals);
}
DecalSystem::~DecalSystem() { delete m_impl; }

DecalId DecalSystem::Spawn(const DecalDesc& desc) {
    Decal* slot = m_impl->freeSlot();
    if (!slot) return kInvalidDecalId;
    slot->id             = m_impl->nextId++;
    slot->desc           = desc;
    slot->remainingLife  = desc.lifetime;
    slot->currentOpacity = desc.opacity;
    slot->active         = true;
    return slot->id;
}

bool DecalSystem::Remove(DecalId id) {
    Decal* d = m_impl->find(id);
    if (!d) return false;
    *d = Decal{};
    return true;
}

void DecalSystem::Clear() {
    for (auto& d : m_impl->pool) d = Decal{};
}

void DecalSystem::Update(float dt) {
    m_impl->expiredThisFrame = 0;
    for (auto& d : m_impl->pool) {
        if (!d.active) continue;
        if (d.remainingLife < 0.0f) continue; // infinite
        d.remainingLife -= dt;
        if (d.remainingLife <= 0.0f) {
            d.active = false;
            ++m_impl->expiredThisFrame;
        } else if (d.remainingLife <= d.desc.fadeOutTime && d.desc.fadeOutTime > 0.0f) {
            d.currentOpacity = d.desc.opacity * (d.remainingLife / d.desc.fadeOutTime);
        }
    }
}

std::vector<DecalId> DecalSystem::CullVisible(
        const std::array<Math::Vec3, 6>& normals,
        const std::array<float, 6>& ds) const {
    std::vector<DecalId> visible;
    for (const auto& d : m_impl->pool) {
        if (!d.active) continue;
        bool outside = false;
        for (int p = 0; p < 6 && !outside; ++p) {
            // Use decal position as proxy; a real impl would test oriented box
            float dist = normals[p].x * d.desc.position.x +
                         normals[p].y * d.desc.position.y +
                         normals[p].z * d.desc.position.z + ds[p];
            float radius = std::max({d.desc.halfExtents.x,
                                     d.desc.halfExtents.y,
                                     d.desc.halfExtents.z});
            if (dist < -radius) outside = true;
        }
        if (!outside) visible.push_back(d.id);
    }
    return visible;
}

void DecalSystem::SortByLayer(std::vector<DecalId>& ids) const {
    std::sort(ids.begin(), ids.end(), [this](DecalId a, DecalId b) {
        const Decal* da = m_impl->find(a);
        const Decal* db = m_impl->find(b);
        if (!da || !db) return false;
        return da->desc.layer < db->desc.layer;
    });
}

void DecalSystem::SortByDistance(std::vector<DecalId>& ids,
                                  const Math::Vec3& eye) const {
    auto dist2 = [&](const Math::Vec3& p) {
        float dx = p.x - eye.x, dy = p.y - eye.y, dz = p.z - eye.z;
        return dx*dx + dy*dy + dz*dz;
    };
    std::sort(ids.begin(), ids.end(), [&](DecalId a, DecalId b) {
        const Decal* da = m_impl->find(a);
        const Decal* db = m_impl->find(b);
        if (!da || !db) return false;
        return dist2(da->desc.position) < dist2(db->desc.position);
    });
}

const Decal* DecalSystem::GetDecal(DecalId id) const { return m_impl->find(id); }

std::vector<DecalId> DecalSystem::ActiveIds() const {
    std::vector<DecalId> ids;
    for (const auto& d : m_impl->pool) if (d.active) ids.push_back(d.id);
    return ids;
}

std::vector<DecalId> DecalSystem::ByTag(const std::string& tag) const {
    std::vector<DecalId> ids;
    for (const auto& d : m_impl->pool)
        if (d.active && d.desc.tag == tag) ids.push_back(d.id);
    return ids;
}

DecalStats DecalSystem::Stats() const {
    DecalStats s;
    s.poolCapacity     = m_impl->maxDecals;
    s.expiredThisFrame = m_impl->expiredThisFrame;
    for (const auto& d : m_impl->pool) if (d.active) ++s.active;
    return s;
}

void DecalSystem::SetMaxDecals(uint32_t n) {
    m_impl->maxDecals = std::min(n, kHardLimit);
}
uint32_t DecalSystem::MaxDecals() const { return m_impl->maxDecals; }

} // namespace Engine
