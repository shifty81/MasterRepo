#include "Engine/Audio/SoundSystem/SoundSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct SoundCb {
    uint32_t instanceId{0};
    std::function<void()> onComplete;
};

struct SoundSystem::Impl {
    std::unordered_map<std::string, SoundClipDesc> clips;
    std::unordered_map<std::string, SoundCueDesc>  cues;
    std::vector<SoundInstance>  instances;
    std::vector<SoundCb>        callbacks;
    uint32_t maxInstances{32};
    uint32_t nextId{1};
    float groupVolumes[(int)SoundGroup::COUNT]{1,1,1,1,1};
    float listenerPos[3]{};
    float listenerFwd[3]{0,0,1};
    float listenerUp [3]{0,1,0};

    SoundInstance* FindInstance(uint32_t id) {
        for (auto& si : instances) if (si.instanceId==id) return &si;
        return nullptr;
    }

    uint32_t SpawnInstance(const std::string& clipId, const SoundPlayParams& params,
                            const float* pos3D)
    {
        // Evict stopped instances first
        for (auto& si : instances)
            if (si.state == SoundState::Stopped) { si = SoundInstance{}; }

        // Count active
        uint32_t active=0;
        SoundInstance* slot = nullptr;
        for (auto& si : instances) {
            if (si.instanceId==0) { if (!slot) slot=&si; }
            else active++;
        }
        if (!slot) {
            if ((uint32_t)instances.size() < maxInstances)
                { instances.emplace_back(); slot = &instances.back(); }
            else {
                // Evict lowest priority
                SoundInstance* victim = nullptr;
                for (auto& si : instances)
                    if (!victim || si.state==SoundState::Stopped) victim=&si;
                if (!victim) return 0;
                slot = victim;
            }
        }

        *slot = SoundInstance{};
        slot->instanceId = nextId++;
        slot->clipId     = clipId;
        slot->volume     = params.volume;
        slot->pitch      = params.pitch;
        slot->loop       = params.loop;
        slot->group      = params.group;
        slot->state      = SoundState::Playing;
        slot->duration   = 3.f; // simulated
        if (pos3D) {
            slot->is3D=true;
            slot->position[0]=pos3D[0]; slot->position[1]=pos3D[1]; slot->position[2]=pos3D[2];
        }
        if (params.fadeIn > 0.f) {
            slot->fadeTimer    = 0.f;
            slot->fadeDuration = params.fadeIn;
            slot->fadeTargetVol= slot->volume;
            slot->volume       = 0.f;
            slot->state        = SoundState::Fading;
        }
        return slot->instanceId;
    }
};

SoundSystem::SoundSystem()  : m_impl(new Impl) {}
SoundSystem::~SoundSystem() { Shutdown(); delete m_impl; }

void SoundSystem::Init(uint32_t maxInstances)
{
    m_impl->maxInstances = maxInstances;
    m_impl->instances.reserve(maxInstances);
}

void SoundSystem::Shutdown()
{
    m_impl->instances.clear();
    m_impl->clips.clear();
}

void SoundSystem::RegisterClip(const SoundClipDesc& desc) { m_impl->clips[desc.id]=desc; }
void SoundSystem::RegisterClip(const std::string& id, const std::string& path)
{
    SoundClipDesc d; d.id=id; d.filePath=path;
    RegisterClip(d);
}
void SoundSystem::RegisterCue(const SoundCueDesc& desc)  { m_impl->cues[desc.cueId]=desc; }
void SoundSystem::UnregisterClip(const std::string& id)  { m_impl->clips.erase(id); }
bool SoundSystem::HasClip(const std::string& id) const   { return m_impl->clips.count(id)>0; }

uint32_t SoundSystem::Play2D(const std::string& clipId, SoundGroup group,
                               float volume, float pitch, bool loop, float fadeIn)
{
    SoundPlayParams p; p.volume=volume; p.pitch=pitch; p.loop=loop;
    p.fadeIn=fadeIn; p.group=group;
    return m_impl->SpawnInstance(clipId, p, nullptr);
}

uint32_t SoundSystem::Play3D(const std::string& clipId, const float position[3],
                               SoundGroup group, float volume, float pitch,
                               bool loop, float fadeIn)
{
    SoundPlayParams p; p.volume=volume; p.pitch=pitch; p.loop=loop;
    p.fadeIn=fadeIn; p.group=group;
    return m_impl->SpawnInstance(clipId, p, position);
}

uint32_t SoundSystem::Play(const std::string& clipId, const SoundPlayParams& params,
                            const float* pos3D)
{
    return m_impl->SpawnInstance(clipId, params, pos3D);
}

uint32_t SoundSystem::PlayCue(const std::string& cueId, SoundGroup group)
{
    auto it = m_impl->cues.find(cueId);
    if (it==m_impl->cues.end() || it->second.clipIds.empty()) return 0;
    const auto& clips = it->second.clipIds;
    uint32_t idx = it->second.randomOrder ? (uint32_t)(std::rand()%clips.size()) : 0;
    return Play2D(clips[idx], group);
}

void SoundSystem::Stop(uint32_t id, float fadeOut)
{
    auto* si = m_impl->FindInstance(id);
    if (!si) return;
    if (fadeOut > 0.f) {
        si->fadeDuration  = fadeOut;
        si->fadeTimer     = 0.f;
        si->fadeTargetVol = 0.f;
        si->state         = SoundState::Fading;
    } else {
        si->state = SoundState::Stopped;
    }
}

void SoundSystem::StopAll(SoundGroup group, float fadeOut)
{
    for (auto& si : m_impl->instances)
        if (si.group==group && si.state!=SoundState::Stopped) Stop(si.instanceId, fadeOut);
}

void SoundSystem::Pause(uint32_t id)
{
    auto* si = m_impl->FindInstance(id);
    if (si && si->state==SoundState::Playing) si->state=SoundState::Paused;
}

void SoundSystem::Resume(uint32_t id)
{
    auto* si = m_impl->FindInstance(id);
    if (si && si->state==SoundState::Paused) si->state=SoundState::Playing;
}

void SoundSystem::PauseGroup(SoundGroup group)
{
    for (auto& si : m_impl->instances) if (si.group==group) Pause(si.instanceId);
}

void SoundSystem::ResumeGroup(SoundGroup group)
{
    for (auto& si : m_impl->instances) if (si.group==group) Resume(si.instanceId);
}

void SoundSystem::SetVolume(uint32_t id, float vol)
{
    auto* si = m_impl->FindInstance(id); if(si) si->volume=vol;
}

void SoundSystem::SetPitch(uint32_t id, float pitch)
{
    auto* si = m_impl->FindInstance(id); if(si) si->pitch=pitch;
}

void SoundSystem::SetPosition(uint32_t id, const float pos[3])
{
    auto* si = m_impl->FindInstance(id);
    if (si) { si->is3D=true; for(int i=0;i<3;i++) si->position[i]=pos[i]; }
}

bool SoundSystem::IsPlaying(uint32_t id) const
{
    const auto* si = m_impl->FindInstance(id);
    return si && (si->state==SoundState::Playing||si->state==SoundState::Fading);
}

void  SoundSystem::SetGroupVolume(SoundGroup g, float v)  { m_impl->groupVolumes[(int)g]=v; }
float SoundSystem::GetGroupVolume(SoundGroup g) const     { return m_impl->groupVolumes[(int)g]; }

void SoundSystem::SetListenerTransform(const float pos[3], const float fwd[3], const float up[3])
{
    for(int i=0;i<3;i++){m_impl->listenerPos[i]=pos[i]; m_impl->listenerFwd[i]=fwd[i]; m_impl->listenerUp[i]=up[i];}
}

void SoundSystem::SetOnComplete(uint32_t id, std::function<void()> cb)
{
    m_impl->callbacks.push_back({id, cb});
}

const SoundInstance* SoundSystem::GetInstance(uint32_t id) const
{
    return m_impl->FindInstance(id);
}

uint32_t SoundSystem::ActiveCount() const
{
    uint32_t n=0;
    for (auto& si : m_impl->instances)
        if (si.instanceId && si.state!=SoundState::Stopped) n++;
    return n;
}

void SoundSystem::Tick(float dt)
{
    for (auto& si : m_impl->instances) {
        if (!si.instanceId || si.state==SoundState::Stopped||si.state==SoundState::Paused) continue;

        si.elapsed += dt;

        // Fade
        if (si.state==SoundState::Fading && si.fadeDuration>0.f) {
            si.fadeTimer += dt;
            float t = std::min(1.f, si.fadeTimer/si.fadeDuration);
            si.volume = si.volume + (si.fadeTargetVol-si.volume)*t;
            if (t>=1.f) {
                si.volume = si.fadeTargetVol;
                si.state  = si.fadeTargetVol<=0.f ? SoundState::Stopped : SoundState::Playing;
            }
        }

        // Simulate duration
        if (si.duration>0.f && si.elapsed>=si.duration) {
            if (si.loop) si.elapsed=0.f;
            else {
                si.state=SoundState::Stopped;
                // Fire complete callbacks
                for (auto& cb : m_impl->callbacks)
                    if (cb.instanceId==si.instanceId && cb.onComplete) cb.onComplete();
            }
        }
    }
}

} // namespace Engine
