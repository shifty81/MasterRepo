#include "Source/Renderer/Lighting/LightSystem.h"
#include <algorithm>

namespace NF {

LightId LightSystem::AddLight(Light light) {
    LightId id = m_NextId++;
    m_Lights.push_back({id, std::move(light)});
    return id;
}

void LightSystem::RemoveLight(LightId id) {
    m_Lights.erase(
        std::remove_if(m_Lights.begin(), m_Lights.end(),
                       [id](const LightEntry& e) { return e.Id == id; }),
        m_Lights.end());
}

const std::vector<Light>& LightSystem::GetLights() const noexcept {
    // Return a view over the Light data stored in each entry.
    // We use a cached flat vector to satisfy the return type contract.
    static thread_local std::vector<Light> s_Cache;
    s_Cache.clear();
    s_Cache.reserve(m_Lights.size());
    for (const auto& e : m_Lights)
        s_Cache.push_back(e.Data);
    return s_Cache;
}

void LightSystem::Update(float dt) {
    // Reserved for animated / dynamic light behaviour.
    (void)dt;
}

} // namespace NF
