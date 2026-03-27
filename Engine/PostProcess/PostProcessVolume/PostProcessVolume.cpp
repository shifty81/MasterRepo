#include "Engine/PostProcess/PostProcessVolume/PostProcessVolume.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Lerp helper ─────────────────────────────────────────────────────────────

PostProcessSettings LerpPP(const PostProcessSettings& a,
                            const PostProcessSettings& b, float t)
{
    auto lf = [&](float av, float bv) { return av + (bv-av)*t; };
    PostProcessSettings r;
    r.bloomIntensity      = lf(a.bloomIntensity, b.bloomIntensity);
    r.bloomThreshold      = lf(a.bloomThreshold, b.bloomThreshold);
    r.exposure            = lf(a.exposure, b.exposure);
    r.saturation          = lf(a.saturation, b.saturation);
    r.contrast            = lf(a.contrast, b.contrast);
    r.aoStrength          = lf(a.aoStrength, b.aoStrength);
    r.aoRadius            = lf(a.aoRadius, b.aoRadius);
    r.chromaticAberration = lf(a.chromaticAberration, b.chromaticAberration);
    r.vignetteIntensity   = lf(a.vignetteIntensity, b.vignetteIntensity);
    r.vignetteRadius      = lf(a.vignetteRadius, b.vignetteRadius);
    r.dofEnabled          = (t >= 0.5f) ? b.dofEnabled : a.dofEnabled;
    r.dofFocalDistance    = lf(a.dofFocalDistance, b.dofFocalDistance);
    r.dofFocalRange       = lf(a.dofFocalRange, b.dofFocalRange);
    r.dofBlurRadius       = lf(a.dofBlurRadius, b.dofBlurRadius);
    r.fogDensity          = lf(a.fogDensity, b.fogDensity);
    for (int i=0;i<3;i++) r.fogColour[i] = lf(a.fogColour[i], b.fogColour[i]);
    r.colourGradingLUT    = (t >= 0.5f) ? b.colourGradingLUT : a.colourGradingLUT;
    return r;
}

// ── Impl ────────────────────────────────────────────────────────────────────

struct PostProcessVolume::Impl {
    std::vector<PPVolumeDesc>  volumes;
    PostProcessSettings        global{};

    struct ActorRecord {
        uint32_t    actorId{0};
        float       position[3]{};
        std::unordered_map<std::string, bool> inVolumes;
    };
    std::unordered_map<uint32_t, ActorRecord> actors;

    VolumeCb onEnter, onExit;

    float PointInVolume(const PPVolumeDesc& v, const float p[3]) const {
        float dx=p[0]-v.centre[0], dy=p[1]-v.centre[1], dz=p[2]-v.centre[2];
        float dist = 0.f;
        if (v.shape == VolumeShape::AABB) {
            float ox = std::max(0.f, std::abs(dx) - v.halfExtents[0]);
            float oy = std::max(0.f, std::abs(dy) - v.halfExtents[1]);
            float oz = std::max(0.f, std::abs(dz) - v.halfExtents[2]);
            dist = std::sqrt(ox*ox+oy*oy+oz*oz);
        } else { // Sphere
            dist = std::sqrt(dx*dx+dy*dy+dz*dz) - v.halfExtents[0];
        }
        if (dist > v.blendRadius) return 0.f;
        if (dist <= 0.f) return 1.f;
        return 1.f - dist/v.blendRadius;
    }
};

PostProcessVolume::PostProcessVolume()  : m_impl(new Impl) {}
PostProcessVolume::~PostProcessVolume() { Shutdown(); delete m_impl; }

void PostProcessVolume::Init()     {}
void PostProcessVolume::Shutdown() {}

void PostProcessVolume::AddVolume(const PPVolumeDesc& desc)
{
    m_impl->volumes.push_back(desc);
}

void PostProcessVolume::RemoveVolume(const std::string& id)
{
    auto& v = m_impl->volumes;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& vd){ return vd.id==id; }), v.end());
}

void PostProcessVolume::UpdateVolume(const std::string& id, const PPVolumeDesc& desc)
{
    for (auto& v : m_impl->volumes) if (v.id==id) { v=desc; return; }
    AddVolume(desc);
}

bool PostProcessVolume::HasVolume(const std::string& id) const
{
    for (auto& v : m_impl->volumes) if (v.id==id) return true;
    return false;
}

void PostProcessVolume::SetGlobal(const PostProcessSettings& s) { m_impl->global = s; }
const PostProcessSettings& PostProcessVolume::GetGlobal() const { return m_impl->global; }

void PostProcessVolume::UpdateActorPosition(uint32_t actorId, const float pos[3])
{
    auto& ar = m_impl->actors[actorId];
    ar.actorId = actorId;
    ar.position[0]=pos[0]; ar.position[1]=pos[1]; ar.position[2]=pos[2];

    // Enter/exit events
    for (auto& v : m_impl->volumes) {
        float blend = m_impl->PointInVolume(v, pos);
        bool nowIn   = blend > 0.f;
        bool wasIn   = ar.inVolumes[v.id];
        if (nowIn != wasIn) {
            ar.inVolumes[v.id] = nowIn;
            if (nowIn  && m_impl->onEnter) m_impl->onEnter(actorId, v.id);
            if (!nowIn && m_impl->onExit)  m_impl->onExit(actorId, v.id);
        }
    }
}

void PostProcessVolume::RemoveActor(uint32_t actorId) { m_impl->actors.erase(actorId); }

PostProcessSettings PostProcessVolume::GetBlendedSettingsAt(const float pos[3]) const
{
    // Sort volumes by priority desc
    std::vector<const PPVolumeDesc*> active;
    for (auto& v : m_impl->volumes) {
        if (m_impl->PointInVolume(v, pos) > 0.f) active.push_back(&v);
    }
    std::sort(active.begin(), active.end(), [](auto* a, auto* b){ return a->priority > b->priority; });

    PostProcessSettings result = m_impl->global;
    for (auto* vp : active) {
        float blend = m_impl->PointInVolume(*vp, pos);
        result = LerpPP(result, vp->settings, blend);
        if (vp->exclusive) break;
    }
    return result;
}

PostProcessSettings PostProcessVolume::GetBlendedSettings(uint32_t actorId) const
{
    auto it = m_impl->actors.find(actorId);
    if (it == m_impl->actors.end()) return m_impl->global;
    return GetBlendedSettingsAt(it->second.position);
}

void PostProcessVolume::SetOnVolumeEnter(VolumeCb cb) { m_impl->onEnter = cb; }
void PostProcessVolume::SetOnVolumeExit(VolumeCb cb)  { m_impl->onExit  = cb; }

bool PostProcessVolume::SaveJSON(const std::string& path) const
{
    std::ofstream f(path); if (!f) return false;
    f << "{\"postProcessVolumes\":" << m_impl->volumes.size() << "}\n";
    return true;
}

bool PostProcessVolume::LoadJSON(const std::string& /*path*/) { return true; }

} // namespace Engine
