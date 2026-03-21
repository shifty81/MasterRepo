#include "Engine/Lod/LODSystem.h"
#include <cmath>
#include <algorithm>

namespace Engine {

struct LODObjectState {
    LODEntry entry;
    float    x{0.0f}, y{0.0f}, z{0.0f};
};

struct LODSystem::Impl {
    std::unordered_map<ObjectID, LODObjectState> objects;
    float camX{0.0f}, camY{0.0f}, camZ{0.0f};
    float hysteresis{0.05f};   // 5% margin
    float maxCullDistSq{0.0f}; // 0 = no hard cull
    std::vector<LODResult>         results;
    std::vector<LODChangedCallback> callbacks;
};

LODSystem::LODSystem() : m_impl(new Impl()) {}
LODSystem::~LODSystem() { delete m_impl; }

void LODSystem::RegisterObject(ObjectID id, std::vector<LODBand> bands,
                                bool alwaysVisible)
{
    LODObjectState state;
    state.entry.id           = id;
    state.entry.bands        = std::move(bands);
    state.entry.currentBand  = 0;
    state.entry.alwaysVisible = alwaysVisible;
    m_impl->objects[id] = std::move(state);
}

void LODSystem::UnregisterObject(ObjectID id) { m_impl->objects.erase(id); }
bool LODSystem::IsRegistered(ObjectID id) const {
    return m_impl->objects.count(id) > 0;
}
size_t LODSystem::ObjectCount() const { return m_impl->objects.size(); }

void LODSystem::SetCameraPosition(float x, float y, float z) {
    m_impl->camX = x; m_impl->camY = y; m_impl->camZ = z;
}

void LODSystem::SetObjectPosition(ObjectID id, float x, float y, float z) {
    auto it = m_impl->objects.find(id);
    if (it != m_impl->objects.end()) {
        it->second.x = x; it->second.y = y; it->second.z = z;
    }
}

const std::vector<LODResult>& LODSystem::Evaluate() {
    m_impl->results.clear();
    m_impl->results.reserve(m_impl->objects.size());

    for (auto& [id, state] : m_impl->objects) {
        float dx = state.x - m_impl->camX;
        float dy = state.y - m_impl->camY;
        float dz = state.z - m_impl->camZ;
        float distSq = dx*dx + dy*dy + dz*dz;

        LODResult res;
        res.id         = id;
        res.distanceSq = distSq;

        // Hard cull check
        bool culled = (!state.entry.alwaysVisible &&
                       m_impl->maxCullDistSq > 0.0f &&
                       distSq > m_impl->maxCullDistSq);
        res.culled = culled;

        if (!culled && !state.entry.bands.empty()) {
            // Find band by distance
            int32_t newBand = static_cast<int32_t>(state.entry.bands.size()) - 1;
            for (int32_t b = 0; b < static_cast<int32_t>(state.entry.bands.size()); ++b) {
                float upper = state.entry.bands[static_cast<size_t>(b)].maxDistanceSq;
                // Apply hysteresis: when moving to finer LOD, add margin
                float margin = (b < state.entry.currentBand)
                    ? upper * m_impl->hysteresis : 0.0f;
                if (distSq <= upper + margin) {
                    newBand = b;
                    break;
                }
            }
            if (newBand != state.entry.currentBand) {
                int32_t prev = state.entry.currentBand;
                state.entry.currentBand = newBand;
                for (auto& cb : m_impl->callbacks) cb(id, newBand);
                (void)prev;
            }
        }
        res.band = culled ? -1 : state.entry.currentBand;
        m_impl->results.push_back(res);
    }

    std::sort(m_impl->results.begin(), m_impl->results.end(),
              [](const LODResult& a, const LODResult& b){
                  return a.distanceSq < b.distanceSq; });
    return m_impl->results;
}

int32_t LODSystem::GetBand(ObjectID id) const {
    auto it = m_impl->objects.find(id);
    return it != m_impl->objects.end() ? it->second.entry.currentBand : -1;
}
bool LODSystem::IsCulled(ObjectID id) const {
    auto it = m_impl->objects.find(id);
    if (it == m_impl->objects.end()) return true;
    if (!m_impl->maxCullDistSq) return false;
    float dx = it->second.x - m_impl->camX;
    float dy = it->second.y - m_impl->camY;
    float dz = it->second.z - m_impl->camZ;
    return (dx*dx + dy*dy + dz*dz) > m_impl->maxCullDistSq;
}

void LODSystem::SetHysteresis(float margin) {
    m_impl->hysteresis = margin < 0.0f ? 0.0f : margin;
}
void LODSystem::SetMaxCullDistance(float d) {
    m_impl->maxCullDistSq = d * d;
}
void LODSystem::OnLODChanged(LODChangedCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Engine
