#include "Runtime/UI/Marker/ObjectiveMarker/ObjectiveMarker.h"
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace Runtime {

struct Marker {
    float wx{}, wy{}, wz{};
    std::string label;
    float r{1}, g{1}, b{1}, a{1};
    uint32_t category{0};
    bool visible{true};
};

struct ObjectiveMarker::Impl {
    std::unordered_map<uint32_t, Marker> markers;
    std::unordered_set<uint32_t>         prevOnScreen;
    std::unordered_set<uint32_t>         catHidden;
    float viewMat[16]{}, projMat[16]{};
    float screenW{1920}, screenH{1080};
    float maxDist{500}, fadeStart{400};
    float camX{}, camY{}, camZ{};
    std::function<void(uint32_t)> onEnter, onExit;
};

ObjectiveMarker::ObjectiveMarker()  { m_impl = new Impl; }
ObjectiveMarker::~ObjectiveMarker() { delete m_impl; }
void ObjectiveMarker::Init    () {}
void ObjectiveMarker::Shutdown() { Reset(); }
void ObjectiveMarker::Reset   () { m_impl->markers.clear(); m_impl->prevOnScreen.clear(); }

bool ObjectiveMarker::AddMarker(uint32_t id, float wx, float wy, float wz,
                                 const std::string& label,
                                 float r, float g, float b, float a) {
    if (m_impl->markers.count(id)) return false;
    Marker m; m.wx = wx; m.wy = wy; m.wz = wz; m.label = label;
    m.r = r; m.g = g; m.b = b; m.a = a;
    m_impl->markers[id] = m;
    return true;
}
void ObjectiveMarker::RemoveMarker(uint32_t id) { m_impl->markers.erase(id); }
void ObjectiveMarker::RemoveAll   () { m_impl->markers.clear(); }
void ObjectiveMarker::SetMarkerVisible  (uint32_t id, bool on)            { auto it = m_impl->markers.find(id); if (it != m_impl->markers.end()) it->second.visible = on; }
void ObjectiveMarker::SetMarkerPosition (uint32_t id, float x, float y, float z) { auto it = m_impl->markers.find(id); if (it != m_impl->markers.end()) { it->second.wx = x; it->second.wy = y; it->second.wz = z; } }
void ObjectiveMarker::SetMarkerLabel    (uint32_t id, const std::string& l) { auto it = m_impl->markers.find(id); if (it != m_impl->markers.end()) it->second.label = l; }
void ObjectiveMarker::SetMarkerColor    (uint32_t id, float r, float g, float b, float a) { auto it = m_impl->markers.find(id); if (it != m_impl->markers.end()) { it->second.r=r; it->second.g=g; it->second.b=b; it->second.a=a; } }
void ObjectiveMarker::SetMarkerCategory (uint32_t id, uint32_t cat)       { auto it = m_impl->markers.find(id); if (it != m_impl->markers.end()) it->second.category = cat; }
void ObjectiveMarker::SetCategoryVisible(uint32_t cat, bool on)           { if (on) m_impl->catHidden.erase(cat); else m_impl->catHidden.insert(cat); }

void ObjectiveMarker::UpdateCamera(const float view[16], const float proj[16],
                                    float sw, float sh) {
    for (int i=0;i<16;++i) { m_impl->viewMat[i] = view[i]; m_impl->projMat[i] = proj[i]; }
    m_impl->screenW = sw; m_impl->screenH = sh;
    // extract camera position from inverse view (column 3 of view-inverse = -view[12..14])
    m_impl->camX = -(view[0]*view[12]+view[1]*view[13]+view[2]*view[14]);
    m_impl->camY = -(view[4]*view[12]+view[5]*view[13]+view[6]*view[14]);
    m_impl->camZ = -(view[8]*view[12]+view[9]*view[13]+view[10]*view[14]);
}

bool ObjectiveMarker::ProjectMarker(uint32_t id, float& sx, float& sy, float& depth) const {
    auto it = m_impl->markers.find(id);
    if (it == m_impl->markers.end()) return false;
    const auto& m = it->second;
    const float* v = m_impl->viewMat; const float* p = m_impl->projMat;
    // transform world → clip
    float vx = v[0]*m.wx + v[4]*m.wy + v[8]*m.wz  + v[12];
    float vy = v[1]*m.wx + v[5]*m.wy + v[9]*m.wz  + v[13];
    float vz = v[2]*m.wx + v[6]*m.wy + v[10]*m.wz + v[14];
    float vw = v[3]*m.wx + v[7]*m.wy + v[11]*m.wz + v[15];
    float cx = p[0]*vx + p[4]*vy + p[8]*vz  + p[12]*vw;
    float cy = p[1]*vx + p[5]*vy + p[9]*vz  + p[13]*vw;
    float cw = p[3]*vx + p[7]*vy + p[11]*vz + p[15]*vw;
    if (cw <= 0) return false;
    depth = cw;
    sx = ((cx/cw)*0.5f + 0.5f) * m_impl->screenW;
    sy = ((1.f - cy/cw)*0.5f)  * m_impl->screenH;
    return true;
}

uint32_t ObjectiveMarker::GetVisibleMarkers(std::vector<uint32_t>& out) const {
    for (auto& [id, m] : m_impl->markers) {
        if (!m.visible) continue;
        if (m_impl->catHidden.count(m.category)) continue;
        float sx, sy, depth; if (!ProjectMarker(id, sx, sy, depth)) continue;
        float d = std::sqrt(
            std::pow(m.wx - m_impl->camX, 2) +
            std::pow(m.wy - m_impl->camY, 2) +
            std::pow(m.wz - m_impl->camZ, 2));
        if (d > m_impl->maxDist) continue;
        out.push_back(id);
    }
    return (uint32_t)out.size();
}

float ObjectiveMarker::GetDistance(uint32_t id) const {
    auto it = m_impl->markers.find(id); if (it == m_impl->markers.end()) return 0;
    const auto& m = it->second;
    return std::sqrt(std::pow(m.wx-m_impl->camX,2)+std::pow(m.wy-m_impl->camY,2)+std::pow(m.wz-m_impl->camZ,2));
}

void ObjectiveMarker::SetMaxDistance(float d) { m_impl->maxDist   = d; }
void ObjectiveMarker::SetFadeStart  (float d) { m_impl->fadeStart = d; }
void ObjectiveMarker::SetOnEnterScreen(std::function<void(uint32_t)> cb) { m_impl->onEnter = std::move(cb); }
void ObjectiveMarker::SetOnExitScreen (std::function<void(uint32_t)> cb) { m_impl->onExit  = std::move(cb); }

} // namespace Runtime
