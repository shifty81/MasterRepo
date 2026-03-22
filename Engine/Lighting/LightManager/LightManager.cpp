#include "Engine/Lighting/LightManager/LightManager.h"
#include <unordered_map>
#include <cmath>

namespace Engine {

// ── Impl ──────────────────────────────────────────────────────────────────
struct LightManager::Impl {
    std::unordered_map<LightId, Light> lights;
    LightId nextId{1};

    std::array<float, 3> ambientColour{0.1f, 0.1f, 0.1f};
    float ambientIntensity{1.0f};

    std::vector<LightAddedCb>   addedCbs;
    std::vector<LightRemovedCb> removedCbs;

    mutable uint32_t lastVisibleCount{0};

    // Sphere-frustum overlap: returns true if sphere (center, r) is inside/overlaps all planes.
    static bool SphereFrustum(const Math::Vec3& center, float r,
                               const std::array<Math::Vec3, 6>& normals,
                               const std::array<float, 6>& ds) {
        for (int i = 0; i < 6; ++i) {
            float d = normals[i].x * center.x + normals[i].y * center.y + normals[i].z * center.z + ds[i];
            if (d < -r) return false;
        }
        return true;
    }

    static float Vec3Dist(const Math::Vec3& a, const Math::Vec3& b) {
        float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
};

// ── Lifecycle ─────────────────────────────────────────────────────────────
LightManager::LightManager() : m_impl(new Impl()) {}
LightManager::~LightManager() { delete m_impl; }

// ── Registry ──────────────────────────────────────────────────────────────
LightId LightManager::AddLight(const LightDesc& desc) {
    LightId id = m_impl->nextId++;
    Light& l = m_impl->lights[id];
    l.id     = id;
    l.desc   = desc;
    l.active = true;
    for (auto& cb : m_impl->addedCbs) cb(id);
    return id;
}

bool LightManager::RemoveLight(LightId id) {
    auto it = m_impl->lights.find(id);
    if (it == m_impl->lights.end()) return false;
    for (auto& cb : m_impl->removedCbs) cb(id);
    m_impl->lights.erase(it);
    return true;
}

bool LightManager::UpdateLight(LightId id, const LightDesc& desc) {
    auto it = m_impl->lights.find(id);
    if (it == m_impl->lights.end()) return false;
    it->second.desc = desc;
    return true;
}

void LightManager::SetActive(LightId id, bool active) {
    auto it = m_impl->lights.find(id);
    if (it != m_impl->lights.end()) it->second.active = active;
}

void LightManager::Clear() {
    for (auto& [id, _] : m_impl->lights)
        for (auto& cb : m_impl->removedCbs) cb(id);
    m_impl->lights.clear();
}

// ── Ambient ───────────────────────────────────────────────────────────────
void LightManager::SetAmbient(std::array<float, 3> colour, float intensity) {
    m_impl->ambientColour    = colour;
    m_impl->ambientIntensity = intensity;
}
std::array<float, 3> LightManager::AmbientColour()    const { return m_impl->ambientColour; }
float                LightManager::AmbientIntensity()  const { return m_impl->ambientIntensity; }

// ── Queries ───────────────────────────────────────────────────────────────
const Light* LightManager::GetLight(LightId id) const {
    auto it = m_impl->lights.find(id);
    return it != m_impl->lights.end() ? &it->second : nullptr;
}

std::vector<LightId> LightManager::AllIds() const {
    std::vector<LightId> out;
    out.reserve(m_impl->lights.size());
    for (const auto& [id, _] : m_impl->lights) out.push_back(id);
    return out;
}

std::vector<LightId> LightManager::GetDirectionalLights() const {
    std::vector<LightId> out;
    for (const auto& [id, l] : m_impl->lights)
        if (l.active && l.desc.type == LightType::Directional) out.push_back(id);
    return out;
}

std::vector<LightId> LightManager::GetShadowCasters() const {
    std::vector<LightId> out;
    for (const auto& [id, l] : m_impl->lights)
        if (l.active && l.desc.castsShadow) out.push_back(id);
    return out;
}

std::vector<LightId> LightManager::CullVisible(
    const std::array<Math::Vec3, 6>& normals,
    const std::array<float, 6>& ds) const
{
    std::vector<LightId> out;
    for (const auto& [id, l] : m_impl->lights) {
        if (!l.active) continue;
        if (l.desc.type == LightType::Directional) {
            out.push_back(id);
        } else {
            if (Impl::SphereFrustum(l.desc.position, l.desc.range, normals, ds))
                out.push_back(id);
        }
    }
    m_impl->lastVisibleCount = static_cast<uint32_t>(out.size());
    return out;
}

std::vector<LightId> LightManager::GetPointLightsInRadius(const Math::Vec3& center, float radius) const {
    std::vector<LightId> out;
    for (const auto& [id, l] : m_impl->lights) {
        if (!l.active) continue;
        if (l.desc.type != LightType::Point && l.desc.type != LightType::Spot) continue;
        float dist = Impl::Vec3Dist(center, l.desc.position);
        if (dist < radius + l.desc.range) out.push_back(id);
    }
    return out;
}

std::vector<LightId> LightManager::ByTag(const std::string& tag) const {
    std::vector<LightId> out;
    for (const auto& [id, l] : m_impl->lights)
        if (l.desc.tag == tag) out.push_back(id);
    return out;
}

// ── Callbacks ─────────────────────────────────────────────────────────────
void LightManager::OnLightAdded(LightAddedCb cb)     { m_impl->addedCbs.push_back(std::move(cb)); }
void LightManager::OnLightRemoved(LightRemovedCb cb) { m_impl->removedCbs.push_back(std::move(cb)); }

// ── Stats ─────────────────────────────────────────────────────────────────
LightStats LightManager::Stats() const {
    LightStats s;
    s.total = static_cast<uint32_t>(m_impl->lights.size());
    s.visible = m_impl->lastVisibleCount;
    for (const auto& [_, l] : m_impl->lights)
        if (l.active && l.desc.castsShadow) ++s.shadowCasting;
    return s;
}

} // namespace Engine
