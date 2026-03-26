#include "Engine/Cinematic/CinematicSystem.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>

namespace Engine {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

// ── Catmull-Rom interpolation helper ─────────────────────────────────────────

static float CatmullRom(float p0, float p1, float p2, float p3, float t) {
    return 0.5f * ((2.f*p1) + (-p0+p2)*t +
                   (2.f*p0-5.f*p1+4.f*p2-p3)*t*t +
                   (-p0+3.f*p1-3.f*p2+p3)*t*t*t);
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct CinematicSystem::Impl {
    std::unordered_map<uint32_t, CinemaSequence>   sequences;
    uint32_t                                        nextId{1};
    uint32_t                                        activeSeq{0};

    std::function<void(uint32_t)> onStart;
    std::function<void(uint32_t)> onEnd;
    std::function<void(uint32_t, uint32_t)> onKeyframe;

    void RecalcDuration(CinemaSequence& seq) {
        if (seq.keyframes.empty()) { seq.duration = 0.f; return; }
        seq.duration = seq.keyframes.back().time;
    }
};

CinematicSystem::CinematicSystem() : m_impl(new Impl()) {}
CinematicSystem::~CinematicSystem() { delete m_impl; }

void CinematicSystem::Init()     {}
void CinematicSystem::Shutdown() { m_impl->sequences.clear(); }

uint32_t CinematicSystem::CreateSequence(const std::string& name) {
    uint32_t id = m_impl->nextId++;
    CinemaSequence& seq = m_impl->sequences[id];
    seq.id   = id;
    seq.name = name;
    return id;
}

void CinematicSystem::DestroySequence(uint32_t id) {
    m_impl->sequences.erase(id);
    if (m_impl->activeSeq == id) m_impl->activeSeq = 0;
}

void CinematicSystem::AddKeyframe(uint32_t id, const CinemaKeyframe& kf) {
    auto it = m_impl->sequences.find(id);
    if (it == m_impl->sequences.end()) return;
    CinemaSequence& seq = it->second;
    // Insert sorted by time
    auto pos = std::lower_bound(seq.keyframes.begin(), seq.keyframes.end(), kf,
                                [](const CinemaKeyframe& a, const CinemaKeyframe& b){
                                    return a.time < b.time; });
    seq.keyframes.insert(pos, kf);
    m_impl->RecalcDuration(seq);
}

void CinematicSystem::ClearKeyframes(uint32_t id) {
    auto it = m_impl->sequences.find(id);
    if (it != m_impl->sequences.end()) {
        it->second.keyframes.clear();
        it->second.duration = 0.f;
    }
}

CinemaSequence CinematicSystem::GetSequence(uint32_t id) const {
    auto it = m_impl->sequences.find(id);
    if (it == m_impl->sequences.end()) return {};
    return it->second;
}

std::vector<CinemaSequence> CinematicSystem::AllSequences() const {
    std::vector<CinemaSequence> out;
    out.reserve(m_impl->sequences.size());
    for (auto& [k, v] : m_impl->sequences) out.push_back(v);
    return out;
}

void CinematicSystem::Play(uint32_t id) {
    auto it = m_impl->sequences.find(id);
    if (it == m_impl->sequences.end()) return;
    it->second.state    = CinemaState::Playing;
    it->second.playhead = 0.f;
    m_impl->activeSeq   = id;
    if (m_impl->onStart) m_impl->onStart(id);
}

void CinematicSystem::Pause(uint32_t id) {
    auto it = m_impl->sequences.find(id);
    if (it != m_impl->sequences.end() && it->second.state == CinemaState::Playing)
        it->second.state = CinemaState::Paused;
}

void CinematicSystem::Stop(uint32_t id) {
    auto it = m_impl->sequences.find(id);
    if (it == m_impl->sequences.end()) return;
    it->second.state    = CinemaState::Idle;
    it->second.playhead = 0.f;
    if (m_impl->activeSeq == id) m_impl->activeSeq = 0;
}

void CinematicSystem::Seek(uint32_t id, float t) {
    auto it = m_impl->sequences.find(id);
    if (it != m_impl->sequences.end())
        it->second.playhead = std::max(0.f, std::min(t, it->second.duration));
}

void CinematicSystem::SetLoop(uint32_t id, bool loop) {
    auto it = m_impl->sequences.find(id);
    if (it != m_impl->sequences.end()) it->second.loop = loop;
}

CameraPose CinematicSystem::Tick(float dt) {
    CameraPose out{};
    if (m_impl->activeSeq == 0) return out;
    auto it = m_impl->sequences.find(m_impl->activeSeq);
    if (it == m_impl->sequences.end()) return out;
    CinemaSequence& seq = it->second;
    if (seq.state != CinemaState::Playing) return out;
    if (seq.keyframes.size() < 2) return out;

    seq.playhead += dt;
    if (seq.playhead >= seq.duration) {
        if (seq.loop) {
            // Handle multiple loop iterations for large dt values
            seq.playhead = seq.duration > 0.f
                           ? std::fmod(std::max(0.f, seq.playhead), seq.duration)
                           : 0.f;
        } else {
            seq.playhead = seq.duration;
            seq.state    = CinemaState::Finished;
            if (m_impl->onEnd) m_impl->onEnd(seq.id);
            m_impl->activeSeq = 0;
        }
    }

    // Find surrounding keyframes
    const auto& kfs = seq.keyframes;
    uint32_t i = 0;
    while (i + 1 < kfs.size() && kfs[i+1].time <= seq.playhead) ++i;
    uint32_t j = std::min<uint32_t>(i + 1, (uint32_t)kfs.size() - 1);

    float span = kfs[j].time - kfs[i].time;
    float t    = span > 0.f ? (seq.playhead - kfs[i].time) / span : 0.f;

    // Clamp Catmull-Rom indices
    uint32_t i0 = i > 0 ? i-1 : 0;
    uint32_t i3 = j+1 < kfs.size() ? j+1 : j;

    for (int c = 0; c < 3; ++c)
        out.position[c] = CatmullRom(kfs[i0].position[c], kfs[i].position[c],
                                     kfs[j].position[c],  kfs[i3].position[c], t);
    // Slerp rotation (simplified lerp for brevity)
    for (int c = 0; c < 4; ++c)
        out.rotation[c] = kfs[i].rotation[c] * (1.f-t) + kfs[j].rotation[c] * t;
    out.fov      = kfs[i].fov      * (1.f-t) + kfs[j].fov      * t;
    out.exposure = kfs[i].exposure * (1.f-t) + kfs[j].exposure * t;
    return out;
}

bool     CinematicSystem::IsPlaying(uint32_t id) const {
    auto it = m_impl->sequences.find(id);
    return it != m_impl->sequences.end() && it->second.state == CinemaState::Playing;
}

uint32_t CinematicSystem::ActiveSequenceId() const { return m_impl->activeSeq; }

void CinematicSystem::OnSequenceStart(std::function<void(uint32_t)> cb) {
    m_impl->onStart = std::move(cb);
}

void CinematicSystem::OnSequenceEnd(std::function<void(uint32_t)> cb) {
    m_impl->onEnd = std::move(cb);
}

void CinematicSystem::OnKeyframeReached(std::function<void(uint32_t, uint32_t)> cb) {
    m_impl->onKeyframe = std::move(cb);
}

} // namespace Engine
