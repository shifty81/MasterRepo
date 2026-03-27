#include "Engine/Audio/Mixer/AudioMixer/AudioMixer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static constexpr float kSilenceDB = -96.f;

static float LinearToDb(float linear) {
    if (linear<=0.f) return kSilenceDB;
    return 20.f * std::log10(linear);
}

struct BusData {
    uint32_t    id{0};
    uint32_t    parentId{0};
    std::string name;
    float       volume{1.f};
    float       pitch {1.f};
    float       pan   {0.f};
    bool        muted {false};
    bool        soloed{false};
    LimiterDesc limiter;
    LPFDesc     lpf;
    VUMeter     vu;
    // Sends: dstBus → sendLevel
    std::unordered_map<uint32_t,float> sends;
};

struct SendKey { uint32_t src,dst; };

struct AudioMixer::Impl {
    std::vector<BusData> buses;
    uint32_t             nextId{1};
    uint32_t             masterId{0};

    std::vector<MixerSnapshot> snapshots;

    BusData* Find(uint32_t id) {
        for(auto& b:buses) if(b.id==id) return &b; return nullptr;
    }
    const BusData* Find(uint32_t id) const {
        for(auto& b:buses) if(b.id==id) return &b; return nullptr;
    }
};

AudioMixer::AudioMixer()  : m_impl(new Impl) {}
AudioMixer::~AudioMixer() { Shutdown(); delete m_impl; }

void AudioMixer::Init() {
    // Create master bus
    BusData master; master.id=m_impl->nextId++; master.name="Master";
    m_impl->buses.push_back(master);
    m_impl->masterId = master.id;
}
void AudioMixer::Shutdown() { m_impl->buses.clear(); }

uint32_t AudioMixer::AddBus(const std::string& name, uint32_t parentBus) {
    BusData b; b.id=m_impl->nextId++; b.name=name;
    b.parentId = parentBus ? parentBus : m_impl->masterId;
    m_impl->buses.push_back(b);
    return m_impl->buses.back().id;
}
void AudioMixer::RemoveBus(uint32_t id) {
    if (id==m_impl->masterId) return;
    auto& v=m_impl->buses;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& b){return b.id==id;}),v.end());
}
uint32_t AudioMixer::MasterBus() const { return m_impl->masterId; }
uint32_t AudioMixer::FindBus(const std::string& name) const {
    for(auto& b:m_impl->buses) if(b.name==name) return b.id; return 0;
}
bool        AudioMixer::HasBus(uint32_t id) const { return m_impl->Find(id)!=nullptr; }
std::string AudioMixer::BusName(uint32_t id) const { const auto* b=m_impl->Find(id); return b?b->name:""; }

void  AudioMixer::SetVolume(uint32_t id, float v) { auto* b=m_impl->Find(id); if(b) b->volume=std::max(0.f,v); }
void  AudioMixer::SetPitch (uint32_t id, float p) { auto* b=m_impl->Find(id); if(b) b->pitch=std::max(0.01f,p); }
void  AudioMixer::SetPan   (uint32_t id, float p) { auto* b=m_impl->Find(id); if(b) b->pan=std::max(-1.f,std::min(1.f,p)); }
void  AudioMixer::SetMute  (uint32_t id, bool m) { auto* b=m_impl->Find(id); if(b) b->muted=m; }
void  AudioMixer::SetSolo  (uint32_t id, bool s) { auto* b=m_impl->Find(id); if(b) b->soloed=s; }
float AudioMixer::GetVolume(uint32_t id) const { const auto* b=m_impl->Find(id); return b?b->volume:0.f; }
float AudioMixer::GetPitch (uint32_t id) const { const auto* b=m_impl->Find(id); return b?b->pitch:1.f; }
float AudioMixer::GetPan   (uint32_t id) const { const auto* b=m_impl->Find(id); return b?b->pan:0.f; }
bool  AudioMixer::IsMuted  (uint32_t id) const { const auto* b=m_impl->Find(id); return b&&b->muted; }
bool  AudioMixer::IsSoloed (uint32_t id) const { const auto* b=m_impl->Find(id); return b&&b->soloed; }

float AudioMixer::GetEffectiveVolume(uint32_t id) const {
    const auto* b=m_impl->Find(id); if(!b) return 0.f;
    if (b->muted) return 0.f;
    float v = b->volume;
    if (b->parentId && b->parentId!=id) v *= GetEffectiveVolume(b->parentId);
    return v;
}

void AudioMixer::SetLimiter(uint32_t id, const LimiterDesc& d) { auto* b=m_impl->Find(id); if(b) b->limiter=d; }
void AudioMixer::SetLPF    (uint32_t id, const LPFDesc&     d) { auto* b=m_impl->Find(id); if(b) b->lpf=d; }

void AudioMixer::AddSend(uint32_t src, uint32_t dst, float level) {
    auto* b=m_impl->Find(src); if(b) b->sends[dst]=level;
}
void AudioMixer::SetSend(uint32_t src, uint32_t dst, float level) {
    auto* b=m_impl->Find(src); if(b) b->sends[dst]=level;
}
void AudioMixer::RemoveSend(uint32_t src, uint32_t dst) {
    auto* b=m_impl->Find(src); if(b) b->sends.erase(dst);
}

VUMeter AudioMixer::GetVU(uint32_t id) const {
    const auto* b=m_impl->Find(id); if(!b) return {};
    return b->vu;
}

void AudioMixer::SaveSnapshot(const std::string& name) {
    MixerSnapshot snap; snap.name=name;
    for (auto& b:m_impl->buses) {
        MixerSnapshot::BusState bs;
        bs.busId=b.id; bs.volume=b.volume; bs.pitch=b.pitch;
        bs.pan=b.pan; bs.muted=b.muted;
        snap.buses.push_back(bs);
    }
    // Replace or add
    for (auto& s:m_impl->snapshots) if(s.name==name){s=snap;return;}
    m_impl->snapshots.push_back(snap);
}
bool AudioMixer::ApplySnapshot(const std::string& name, float /*blendTime*/) {
    for (auto& snap:m_impl->snapshots) {
        if (snap.name!=name) continue;
        for (auto& bs:snap.buses) {
            auto* b=m_impl->Find(bs.busId); if(!b) continue;
            b->volume=bs.volume; b->pitch=bs.pitch; b->pan=bs.pan; b->muted=bs.muted;
        }
        return true;
    }
    return false;
}
std::vector<std::string> AudioMixer::GetSnapshots() const {
    std::vector<std::string> out; for(auto& s:m_impl->snapshots) out.push_back(s.name); return out;
}

std::vector<uint32_t> AudioMixer::GetAllBuses() const {
    std::vector<uint32_t> out; for(auto& b:m_impl->buses) out.push_back(b.id); return out;
}
uint32_t AudioMixer::BusCount() const { return (uint32_t)m_impl->buses.size(); }

void AudioMixer::Tick(float dt) {
    // Simulate VU decay
    for (auto& b:m_impl->buses) {
        b.vu.peakDB = std::max(kSilenceDB, b.vu.peakDB - 10.f * dt);
        b.vu.rmsDB  = std::max(kSilenceDB, b.vu.rmsDB  -  5.f * dt);
    }
}

} // namespace Engine
