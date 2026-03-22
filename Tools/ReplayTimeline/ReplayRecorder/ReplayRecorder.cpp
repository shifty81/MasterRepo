#include "Tools/ReplayTimeline/ReplayRecorder/ReplayRecorder.h"
#include <chrono>
#include <fstream>
#include <algorithm>
#include <cstring>

namespace Tools {

// ── Timing helper ─────────────────────────────────────────────────────────
static uint64_t nowUs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct ReplayRecorder::Impl {
    std::vector<ReplayFrame>        frames;
    bool                            recording{false};
    bool                            playing{false};
    uint64_t                        recordStartUs{0};
    uint32_t                        playbackCursor{0};
    uint64_t                        playbackTimeUs{0};

    std::vector<FrameReadyCb>       frameReadyCbs;
    std::vector<RecordingEndCb>     recordingEndCbs;
};

ReplayRecorder::ReplayRecorder() : m_impl(new Impl()) {}
ReplayRecorder::~ReplayRecorder() { delete m_impl; }

void ReplayRecorder::StartRecording() {
    m_impl->frames.clear();
    m_impl->recording     = true;
    m_impl->recordStartUs = nowUs();
}

void ReplayRecorder::StopRecording() {
    if (!m_impl->recording) return;
    m_impl->recording = false;
    ReplayStats s = Stats();
    for (auto& cb : m_impl->recordingEndCbs) cb(s);
}

bool ReplayRecorder::IsRecording() const { return m_impl->recording; }

void ReplayRecorder::Record(const std::vector<uint8_t>& stateBlob,
                             const std::vector<ReplayInputEvent>& inputs,
                             uint64_t timestampUs) {
    if (!m_impl->recording) return;
    ReplayFrame f;
    f.timestampUs = (timestampUs == 0)
        ? (nowUs() - m_impl->recordStartUs) : timestampUs;
    f.stateBlob  = stateBlob;
    f.inputs     = inputs;
    f.frameIndex = static_cast<uint32_t>(m_impl->frames.size());
    m_impl->frames.push_back(std::move(f));
}

bool ReplayRecorder::StartPlayback() {
    if (m_impl->frames.empty()) return false;
    m_impl->playing       = true;
    m_impl->playbackCursor = 0;
    m_impl->playbackTimeUs = 0;
    return true;
}

void ReplayRecorder::StopPlayback() { m_impl->playing = false; }
bool ReplayRecorder::IsPlaying() const { return m_impl->playing; }

bool ReplayRecorder::Seek(uint64_t timestampUs) {
    if (m_impl->frames.empty()) return false;
    // Binary search for nearest frame
    auto it = std::lower_bound(m_impl->frames.begin(), m_impl->frames.end(),
        timestampUs, [](const ReplayFrame& f, uint64_t t){ return f.timestampUs < t; });
    if (it == m_impl->frames.end()) --it;
    m_impl->playbackCursor = static_cast<uint32_t>(
        std::distance(m_impl->frames.begin(), it));
    m_impl->playbackTimeUs = timestampUs;
    return true;
}

const ReplayFrame* ReplayRecorder::CurrentFrame() const {
    if (!m_impl->playing || m_impl->playbackCursor >= m_impl->frames.size())
        return nullptr;
    return &m_impl->frames[m_impl->playbackCursor];
}

bool ReplayRecorder::Advance(float speedMultiplier, uint64_t elapsedUs) {
    if (!m_impl->playing || m_impl->frames.empty()) return false;
    m_impl->playbackTimeUs += static_cast<uint64_t>(
        static_cast<float>(elapsedUs) * speedMultiplier);

    while (m_impl->playbackCursor < m_impl->frames.size() &&
           m_impl->frames[m_impl->playbackCursor].timestampUs <= m_impl->playbackTimeUs) {
        const auto& f = m_impl->frames[m_impl->playbackCursor];
        for (auto& cb : m_impl->frameReadyCbs) cb(f);
        ++m_impl->playbackCursor;
    }
    if (m_impl->playbackCursor >= m_impl->frames.size()) {
        m_impl->playing = false;
        return false;
    }
    return true;
}

bool ReplayRecorder::AtEnd() const {
    return m_impl->playbackCursor >= m_impl->frames.size();
}

const ReplayFrame* ReplayRecorder::FrameAt(uint32_t index) const {
    return index < m_impl->frames.size() ? &m_impl->frames[index] : nullptr;
}

const ReplayFrame* ReplayRecorder::FrameAtTime(uint64_t timestampUs) const {
    if (m_impl->frames.empty()) return nullptr;
    auto it = std::lower_bound(m_impl->frames.begin(), m_impl->frames.end(),
        timestampUs, [](const ReplayFrame& f, uint64_t t){ return f.timestampUs < t; });
    if (it == m_impl->frames.end()) --it;
    return &(*it);
}

uint32_t ReplayRecorder::FrameCount() const {
    return static_cast<uint32_t>(m_impl->frames.size());
}

uint64_t ReplayRecorder::DurationUs() const {
    return m_impl->frames.empty() ? 0
        : m_impl->frames.back().timestampUs - m_impl->frames.front().timestampUs;
}

void ReplayRecorder::TrimTo(uint64_t startUs, uint64_t endUs) {
    m_impl->frames.erase(
        std::remove_if(m_impl->frames.begin(), m_impl->frames.end(),
            [startUs, endUs](const ReplayFrame& f) {
                return f.timestampUs < startUs || f.timestampUs > endUs;
            }),
        m_impl->frames.end());
    for (uint32_t i = 0; i < m_impl->frames.size(); ++i)
        m_impl->frames[i].frameIndex = i;
}

void ReplayRecorder::Clear() {
    m_impl->frames.clear();
    m_impl->recording = m_impl->playing = false;
}

// ── Binary export / import ────────────────────────────────────────────────
bool ReplayRecorder::Export(const std::string& filePath) const {
    std::ofstream out(filePath, std::ios::binary);
    if (!out) return false;
    // Header: magic + version + frame count
    const char magic[4] = {'R','P','L','Y'};
    out.write(magic, 4);
    uint32_t version = 1;
    uint32_t count   = static_cast<uint32_t>(m_impl->frames.size());
    out.write(reinterpret_cast<const char*>(&version), 4);
    out.write(reinterpret_cast<const char*>(&count),   4);
    for (const auto& f : m_impl->frames) {
        out.write(reinterpret_cast<const char*>(&f.timestampUs), 8);
        out.write(reinterpret_cast<const char*>(&f.frameIndex),  4);
        uint32_t blobSize = static_cast<uint32_t>(f.stateBlob.size());
        out.write(reinterpret_cast<const char*>(&blobSize), 4);
        if (blobSize) out.write(reinterpret_cast<const char*>(f.stateBlob.data()), blobSize);
        uint32_t inputCount = static_cast<uint32_t>(f.inputs.size());
        out.write(reinterpret_cast<const char*>(&inputCount), 4);
        for (const auto& ev : f.inputs)
            out.write(reinterpret_cast<const char*>(&ev), sizeof(ev));
    }
    return true;
}

bool ReplayRecorder::Import(const std::string& filePath) {
    std::ifstream in(filePath, std::ios::binary);
    if (!in) return false;
    char magic[4]; in.read(magic, 4);
    if (std::memcmp(magic, "RPLY", 4) != 0) return false;
    uint32_t version = 0, count = 0;
    in.read(reinterpret_cast<char*>(&version), 4);
    in.read(reinterpret_cast<char*>(&count),   4);
    m_impl->frames.clear();
    m_impl->frames.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        ReplayFrame f;
        in.read(reinterpret_cast<char*>(&f.timestampUs), 8);
        in.read(reinterpret_cast<char*>(&f.frameIndex),  4);
        uint32_t blobSize = 0;
        in.read(reinterpret_cast<char*>(&blobSize), 4);
        f.stateBlob.resize(blobSize);
        if (blobSize) in.read(reinterpret_cast<char*>(f.stateBlob.data()), blobSize);
        uint32_t inputCount = 0;
        in.read(reinterpret_cast<char*>(&inputCount), 4);
        f.inputs.resize(inputCount);
        for (auto& ev : f.inputs)
            in.read(reinterpret_cast<char*>(&ev), sizeof(ev));
        m_impl->frames.push_back(std::move(f));
    }
    return static_cast<bool>(in);
}

ReplayStats ReplayRecorder::Stats() const {
    ReplayStats s;
    s.totalFrames = static_cast<uint32_t>(m_impl->frames.size());
    s.durationUs  = DurationUs();
    for (const auto& f : m_impl->frames)
        s.uncompressedBytes += f.stateBlob.size() + f.inputs.size() * sizeof(ReplayInputEvent);
    s.compressedBytes = s.uncompressedBytes; // no compression in this impl
    if (s.totalFrames > 0)
        s.avgFrameMs = static_cast<double>(s.durationUs) / s.totalFrames / 1000.0;
    return s;
}

void ReplayRecorder::OnFrameReady(FrameReadyCb cb) {
    m_impl->frameReadyCbs.push_back(std::move(cb));
}
void ReplayRecorder::OnRecordingEnd(RecordingEndCb cb) {
    m_impl->recordingEndCbs.push_back(std::move(cb));
}

} // namespace Tools
