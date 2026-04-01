#include "Renderer/Lighting/LightSystem.h"
#include <algorithm>

namespace NF {

LightId LightSystem::AddLight(Light light) {
    LightId id = m_NextId++;
    m_Lights.push_back({id, std::move(light)});
    m_FlatCache.clear(); // invalidate
    return id;
}

void LightSystem::RemoveLight(LightId id) {
    m_Lights.erase(
        std::remove_if(m_Lights.begin(), m_Lights.end(),
                       [id](const LightEntry& e) { return e.Id == id; }),
        m_Lights.end());
    m_FlatCache.clear(); // invalidate
}

const std::vector<Light>& LightSystem::GetLights() const noexcept {
    if (m_FlatCache.size() != m_Lights.size()) {
        m_FlatCache.clear();
        m_FlatCache.reserve(m_Lights.size());
        for (const auto& e : m_Lights)
            m_FlatCache.push_back(e.Data);
    }
    return m_FlatCache;
}

void LightSystem::Update(float dt) {
    // Reserved for animated / dynamic light behaviour.
    (void)dt;
}

} // namespace NF
