#include "Tools/ReplayTimeline/ReplayTimeline.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Binary I/O helpers
// ─────────────────────────────────────────────────────────────────────────────

static void WriteU32(std::ostream& o, uint32_t v) {
    o.write(reinterpret_cast<const char*>(&v), sizeof(v));
}
static void WriteFloat(std::ostream& o, float v) {
    o.write(reinterpret_cast<const char*>(&v), sizeof(v));
}
static void WriteU64(std::ostream& o, uint64_t v) {
    o.write(reinterpret_cast<const char*>(&v), sizeof(v));
}
static void WriteString(std::ostream& o, const std::string& s) {
    uint32_t len = static_cast<uint32_t>(s.size());
    WriteU32(o, len);
    o.write(s.data(), len);
}

static bool ReadU32(std::istream& i, uint32_t& v) {
    return static_cast<bool>(i.read(reinterpret_cast<char*>(&v), sizeof(v)));
}
static bool ReadFloat(std::istream& i, float& v) {
    return static_cast<bool>(i.read(reinterpret_cast<char*>(&v), sizeof(v)));
}
static bool ReadU64(std::istream& i, uint64_t& v) {
    return static_cast<bool>(i.read(reinterpret_cast<char*>(&v), sizeof(v)));
}
static bool ReadString(std::istream& i, std::string& s) {
    uint32_t len = 0;
    if (!ReadU32(i, len)) return false;
    s.resize(len);
    return static_cast<bool>(i.read(s.data(), len));
}

// Magic header to verify file integrity
static constexpr uint32_t kMagic   = 0x52504C59; // "RPLY"
static constexpr uint32_t kVersion = 1;

// ─────────────────────────────────────────────────────────────────────────────
// Recording
// ─────────────────────────────────────────────────────────────────────────────

void ReplayTimeline::StartRecording() {
    m_frames.clear();
    m_currentIndex = 0;
    m_state = ReplayState::Recording;
}

void ReplayTimeline::StopRecording() {
    if (m_state == ReplayState::Recording)
        m_state = ReplayState::Idle;
}

void ReplayTimeline::RecordFrame(float simTime, float dt,
                                  const std::vector<uint8_t>& blob,
                                  const std::unordered_map<std::string, std::string>& meta) {
    if (m_state != ReplayState::Recording) return;
    ReplayFrame frame;
    frame.frameIndex = static_cast<uint32_t>(m_frames.size());
    frame.simTime    = simTime;
    frame.deltaTime  = dt;
    frame.stateBlob  = blob;
    frame.metadata   = meta;
    m_frames.push_back(std::move(frame));
}

// ─────────────────────────────────────────────────────────────────────────────
// Playback
// ─────────────────────────────────────────────────────────────────────────────

void ReplayTimeline::Play() {
    if (m_frames.empty()) return;
    m_state = ReplayState::Playing;
    if (m_frameChangedCb && m_currentIndex < m_frames.size())
        m_frameChangedCb(m_frames[m_currentIndex]);
}

void ReplayTimeline::Pause() {
    if (m_state == ReplayState::Playing)
        m_state = ReplayState::Paused;
}

void ReplayTimeline::Stop() {
    m_state = ReplayState::Idle;
    m_currentIndex = 0;
}

void ReplayTimeline::SeekToFrame(uint32_t frameIndex) {
    if (m_frames.empty()) return;
    m_currentIndex = std::min(frameIndex, static_cast<uint32_t>(m_frames.size() - 1));
    if (m_frameChangedCb)
        m_frameChangedCb(m_frames[m_currentIndex]);
}

void ReplayTimeline::SeekToTime(float simTime) {
    if (m_frames.empty()) return;
    // Find the frame whose simTime is closest (lower_bound style)
    uint32_t best = 0;
    float bestDiff = std::abs(m_frames[0].simTime - simTime);
    for (uint32_t i = 1; i < static_cast<uint32_t>(m_frames.size()); ++i) {
        float diff = std::abs(m_frames[i].simTime - simTime);
        if (diff < bestDiff) { bestDiff = diff; best = i; }
    }
    SeekToFrame(best);
}

void ReplayTimeline::SetPlaybackSpeed(float speed) {
    m_playbackSpeed = speed;
}

const ReplayFrame* ReplayTimeline::GetCurrentFrame() const {
    if (m_frames.empty() || m_currentIndex >= m_frames.size()) return nullptr;
    return &m_frames[m_currentIndex];
}

const ReplayFrame* ReplayTimeline::Step() {
    if (m_state != ReplayState::Playing && m_state != ReplayState::Paused)
        return nullptr;
    if (IsAtEnd()) return nullptr;

    ++m_currentIndex;
    if (m_currentIndex >= m_frames.size()) {
        m_currentIndex = static_cast<uint32_t>(m_frames.size());
        m_state = ReplayState::Idle;
        return nullptr;
    }
    const ReplayFrame* f = &m_frames[m_currentIndex];
    if (m_frameChangedCb) m_frameChangedCb(*f);
    return f;
}

bool ReplayTimeline::IsAtEnd() const {
    return m_frames.empty() || m_currentIndex + 1 >= static_cast<uint32_t>(m_frames.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

ReplayState ReplayTimeline::GetState()          const { return m_state; }
uint32_t    ReplayTimeline::FrameCount()        const { return static_cast<uint32_t>(m_frames.size()); }
uint32_t    ReplayTimeline::CurrentFrameIndex() const { return m_currentIndex; }
float       ReplayTimeline::PlaybackSpeed()     const { return m_playbackSpeed; }

float ReplayTimeline::TotalDuration() const {
    if (m_frames.empty()) return 0.0f;
    return m_frames.back().simTime;
}

float ReplayTimeline::CurrentTime() const {
    if (m_frames.empty() || m_currentIndex >= m_frames.size()) return 0.0f;
    return m_frames[m_currentIndex].simTime;
}

// ─────────────────────────────────────────────────────────────────────────────
// Serialization
// ─────────────────────────────────────────────────────────────────────────────

bool ReplayTimeline::SaveToFile(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    WriteU32(f, kMagic);
    WriteU32(f, kVersion);
    WriteU32(f, static_cast<uint32_t>(m_frames.size()));

    for (const auto& frame : m_frames) {
        WriteU32(f,  frame.frameIndex);
        WriteFloat(f, frame.simTime);
        WriteFloat(f, frame.deltaTime);
        WriteU32(f,  static_cast<uint32_t>(frame.stateBlob.size()));
        if (!frame.stateBlob.empty())
            f.write(reinterpret_cast<const char*>(frame.stateBlob.data()),
                    static_cast<std::streamsize>(frame.stateBlob.size()));
        WriteU32(f, static_cast<uint32_t>(frame.metadata.size()));
        for (const auto& [k, v] : frame.metadata) {
            WriteString(f, k);
            WriteString(f, v);
        }
    }
    return f.good();
}

bool ReplayTimeline::LoadFromFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    uint32_t magic = 0, version = 0, count = 0;
    if (!ReadU32(f, magic) || magic != kMagic) return false;
    if (!ReadU32(f, version) || version != kVersion) return false;
    if (!ReadU32(f, count)) return false;

    m_frames.clear();
    m_frames.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        ReplayFrame frame;
        if (!ReadU32(f,  frame.frameIndex))   return false;
        if (!ReadFloat(f, frame.simTime))     return false;
        if (!ReadFloat(f, frame.deltaTime))   return false;
        uint32_t blobSize = 0;
        if (!ReadU32(f, blobSize))            return false;
        if (blobSize > 0) {
            frame.stateBlob.resize(blobSize);
            if (!f.read(reinterpret_cast<char*>(frame.stateBlob.data()),
                        static_cast<std::streamsize>(blobSize))) return false;
        }
        uint32_t metaCount = 0;
        if (!ReadU32(f, metaCount)) return false;
        for (uint32_t m = 0; m < metaCount; ++m) {
            std::string k, v;
            if (!ReadString(f, k) || !ReadString(f, v)) return false;
            frame.metadata[k] = v;
        }
        m_frames.push_back(std::move(frame));
    }
    m_currentIndex = 0;
    m_state = ReplayState::Idle;
    return true;
}

void ReplayTimeline::Clear() {
    m_frames.clear();
    m_currentIndex = 0;
    m_state = ReplayState::Idle;
}

void ReplayTimeline::SetOnFrameChanged(FrameCallback cb) {
    m_frameChangedCb = std::move(cb);
}

} // namespace Tools
