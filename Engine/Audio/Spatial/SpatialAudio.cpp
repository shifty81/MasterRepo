#include "Engine/Audio/Spatial/SpatialAudio.h"
#include <cmath>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace Engine {

// ── Distance attenuation (inverse linear) ─────────────────────────────────────

static float Attenuate(float dist, float minDist, float maxDist, float rolloff) {
    if (dist <= minDist) return 1.f;
    if (dist >= maxDist) return 0.f;
    return minDist / (minDist + rolloff * (dist - minDist));
}

static float Distance3(const float a[3], const float b[3]) {
    float dx = a[0]-b[0], dy = a[1]-b[1], dz = a[2]-b[2];
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

struct SpatialSourceData {
    SpatialSourceInfo info;
    float             occlusionFactor{1.f};
    bool              shouldPlay{false};
    bool              valid{true};
};

struct SpatialAudio::Impl {
    SpatialAudioConfig    config;
    float                 listenerPos[3]{};
    float                 listenerFwd[3]{0,0,-1};
    float                 listenerUp[3]{0,1,0};
    float                 listenerVel[3]{};
    float                 masterVolume{1.f};

    std::unordered_map<uint32_t, SpatialSourceData> sources;
    std::unordered_map<uint32_t, ReverbZone>        reverbZones;
    uint32_t nextSourceId{1};
    uint32_t nextZoneId{1};

    std::function<void(uint32_t)> onFinished;
};

SpatialAudio::SpatialAudio() : m_impl(new Impl()) {}
SpatialAudio::~SpatialAudio() { delete m_impl; }

void SpatialAudio::Init(const SpatialAudioConfig& cfg) { m_impl->config = cfg; }
void SpatialAudio::Shutdown() { m_impl->sources.clear(); }

void SpatialAudio::SetListenerTransform(const float pos[3], const float fwd[3],
                                        const float up[3]) {
    for (int i = 0; i < 3; ++i) {
        m_impl->listenerPos[i] = pos[i];
        m_impl->listenerFwd[i] = fwd[i];
        m_impl->listenerUp[i]  = up[i];
    }
}

void SpatialAudio::SetListenerVelocity(const float vel[3]) {
    for (int i = 0; i < 3; ++i) m_impl->listenerVel[i] = vel[i];
}

uint32_t SpatialAudio::CreateSource(const std::string& assetPath) {
    uint32_t id = m_impl->nextSourceId++;
    auto& sd = m_impl->sources[id];
    sd.info.id        = id;
    sd.info.assetPath = assetPath;
    sd.info.maxDistance = m_impl->config.maxDistance;
    return id;
}

void SpatialAudio::DestroySource(uint32_t id) { m_impl->sources.erase(id); }

void SpatialAudio::SetSourcePosition(uint32_t id, const float pos[3]) {
    auto it = m_impl->sources.find(id);
    if (it == m_impl->sources.end()) return;
    for (int i = 0; i < 3; ++i) it->second.info.position[i] = pos[i];
}

void SpatialAudio::SetSourceVelocity(uint32_t id, const float vel[3]) {
    auto it = m_impl->sources.find(id);
    if (it == m_impl->sources.end()) return;
    for (int i = 0; i < 3; ++i) it->second.info.velocity[i] = vel[i];
}

void SpatialAudio::SetSourceVolume(uint32_t id, float v)  { auto it = m_impl->sources.find(id); if(it!=m_impl->sources.end()) it->second.info.volume=v; }
void SpatialAudio::SetSourcePitch(uint32_t id, float p)   { auto it = m_impl->sources.find(id); if(it!=m_impl->sources.end()) it->second.info.pitch=p; }
void SpatialAudio::SetSourceLooping(uint32_t id, bool l)  { auto it = m_impl->sources.find(id); if(it!=m_impl->sources.end()) it->second.info.looping=l; }
void SpatialAudio::SetSourceDistances(uint32_t id, float mn, float mx) {
    auto it = m_impl->sources.find(id);
    if (it != m_impl->sources.end()) { it->second.info.minDistance=mn; it->second.info.maxDistance=mx; }
}

void SpatialAudio::PlaySource(uint32_t id)  { auto it=m_impl->sources.find(id); if(it!=m_impl->sources.end()) { it->second.info.playing=true;  it->second.shouldPlay=true; } }
void SpatialAudio::StopSource(uint32_t id)  { auto it=m_impl->sources.find(id); if(it!=m_impl->sources.end()) it->second.info.playing=false; }
void SpatialAudio::PauseSource(uint32_t id) { auto it=m_impl->sources.find(id); if(it!=m_impl->sources.end()) it->second.info.playing=false; }

bool SpatialAudio::IsPlaying(uint32_t id) const {
    auto it = m_impl->sources.find(id);
    return it != m_impl->sources.end() && it->second.info.playing;
}

SpatialSourceInfo SpatialAudio::GetSourceInfo(uint32_t id) const {
    auto it = m_impl->sources.find(id);
    return it != m_impl->sources.end() ? it->second.info : SpatialSourceInfo{};
}

std::vector<uint32_t> SpatialAudio::ActiveSourceIds() const {
    std::vector<uint32_t> out;
    for (auto& [id, s] : m_impl->sources) if (s.info.playing) out.push_back(id);
    return out;
}

uint32_t SpatialAudio::AddReverbZone(const ReverbZone& zone) {
    uint32_t id = m_impl->nextZoneId++;
    ReverbZone z = zone; z.id = id;
    m_impl->reverbZones[id] = z;
    return id;
}

void SpatialAudio::RemoveReverbZone(uint32_t id) { m_impl->reverbZones.erase(id); }

void SpatialAudio::UpdateReverbZone(uint32_t id, const ReverbZone& zone) {
    auto it = m_impl->reverbZones.find(id);
    if (it != m_impl->reverbZones.end()) { it->second = zone; it->second.id = id; }
}

void SpatialAudio::SetSourceOcclusion(uint32_t id, float factor) {
    auto it = m_impl->sources.find(id);
    if (it != m_impl->sources.end()) {
        it->second.occlusionFactor   = factor;
        it->second.info.occluded     = factor < 0.5f;
    }
}

void SpatialAudio::Update(float /*dt*/) {
    // Compute per-source effective volume = spatial_atten * occlusion * master
    for (auto& [id, sd] : m_impl->sources) {
        if (!sd.info.playing) continue;
        float dist = Distance3(sd.info.position, m_impl->listenerPos);
        float atten = Attenuate(dist, sd.info.minDistance, sd.info.maxDistance,
                                m_impl->config.rolloffFactor);
        (void)atten; // platform audio backend would apply this
        // Doppler (platform stub)
        // Reverb zone check (platform stub)
    }
}

void SpatialAudio::SetMasterVolume(float v) { m_impl->masterVolume = v; }
void SpatialAudio::PauseAll()  { for (auto& [id,s]:m_impl->sources) s.info.playing=false; }
void SpatialAudio::ResumeAll() { for (auto& [id,s]:m_impl->sources) if(s.shouldPlay) s.info.playing=true; }
void SpatialAudio::StopAll()   { for (auto& [id,s]:m_impl->sources) { s.info.playing=false; s.shouldPlay=false; } }

void SpatialAudio::OnSourceFinished(std::function<void(uint32_t)> cb) {
    m_impl->onFinished = std::move(cb);
}

} // namespace Engine
