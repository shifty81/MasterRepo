#include "Tools/SimulationPlayback/SimulationPlayback.h"
#include <fstream>
#include <sstream>
#include <numeric>
#include <ctime>

namespace Tools {

// ── SimRecording helpers ───────────────────────────────────────

double SimRecording::TotalDuration() const {
    if (frames.empty()) return 0.0;
    return frames.back().timeStampSec - frames.front().timeStampSec;
}

// ── Singleton ──────────────────────────────────────────────────

SimulationPlayback& SimulationPlayback::Get() {
    static SimulationPlayback instance;
    return instance;
}

// ── Recording ─────────────────────────────────────────────────

void SimulationPlayback::StartRecording(const std::string& name) {
    m_activeRecording = name;
    m_recording       = true;
    SimRecording rec;
    rec.name = name;
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&t));
    rec.createdAt = buf;
    m_recordings[name] = std::move(rec);
}

void SimulationPlayback::CaptureFrame(const SimFrame& frame) {
    if (!m_recording || m_activeRecording.empty()) return;
    m_recordings[m_activeRecording].frames.push_back(frame);
}

SimRecording SimulationPlayback::StopRecording() {
    m_recording = false;
    auto it = m_recordings.find(m_activeRecording);
    if (it == m_recordings.end()) return {};
    SimRecording result = it->second;
    m_activeRecording.clear();
    return result;
}

// ── Playback ───────────────────────────────────────────────────

void SimulationPlayback::Play(const std::string& name, float speed) {
    auto it = m_recordings.find(name);
    if (it == m_recordings.end()) return;
    m_playbackRecording = name;
    m_playSpeed         = speed;
    m_currentFrame      = 0;
    m_playing           = true;

    // Iterate frames synchronously (async playback tied to engine tick in production)
    const auto& frames = it->second.frames;
    while (m_playing && m_currentFrame < frames.size()) {
        if (m_onFrame) m_onFrame(frames[m_currentFrame]);
        ++m_currentFrame;
    }
    m_playing = false;
    if (m_onFinish) m_onFinish(name);
}

void SimulationPlayback::Pause() { m_playing = false; }
void SimulationPlayback::Stop()  { m_playing = false; m_currentFrame = 0; }

void SimulationPlayback::SeekToFrame(uint64_t frame) {
    auto it = m_recordings.find(m_playbackRecording);
    if (it == m_recordings.end()) return;
    m_currentFrame = std::min(frame, static_cast<uint64_t>(it->second.frames.size() - 1));
}

void SimulationPlayback::SeekToTime(double seconds) {
    auto it = m_recordings.find(m_playbackRecording);
    if (it == m_recordings.end()) return;
    const auto& frames = it->second.frames;
    double origin = frames.empty() ? 0.0 : frames.front().timeStampSec;
    for (uint64_t i = 0; i < frames.size(); ++i) {
        if (frames[i].timeStampSec - origin >= seconds) {
            m_currentFrame = i;
            return;
        }
    }
    m_currentFrame = frames.size() > 0 ? frames.size() - 1 : 0;
}

// ── Comparison ────────────────────────────────────────────────

SimDiff SimulationPlayback::Compare(const std::string& nameA,
                                     const std::string& nameB) const {
    SimDiff diff;
    diff.recordingA = nameA;
    diff.recordingB = nameB;
    diff.identical  = true;

    auto itA = m_recordings.find(nameA);
    auto itB = m_recordings.find(nameB);
    if (itA == m_recordings.end() || itB == m_recordings.end()) {
        diff.differences.push_back("One or both recordings not found.");
        diff.identical = false;
        return diff;
    }

    const auto& framesA = itA->second.frames;
    const auto& framesB = itB->second.frames;

    if (framesA.size() != framesB.size()) {
        diff.differences.push_back("Frame count differs: " +
            std::to_string(framesA.size()) + " vs " + std::to_string(framesB.size()));
        diff.identical = false;
    }

    uint64_t limit = std::min(framesA.size(), framesB.size());
    for (uint64_t i = 0; i < limit; ++i) {
        const auto& fa = framesA[i];
        const auto& fb = framesB[i];
        bool frameDiffers = false;

        // Compare events
        if (fa.events != fb.events) {
            frameDiffers = true;
            diff.differences.push_back("Frame " + std::to_string(i) + ": event mismatch");
        }
        // Compare metrics
        for (auto& [k, va] : fa.metrics) {
            auto it = fb.metrics.find(k);
            if (it == fb.metrics.end() || std::abs(it->second - va) > 1e-6) {
                frameDiffers = true;
                diff.differences.push_back("Frame " + std::to_string(i) +
                    ": metric '" + k + "' differs");
            }
        }
        if (frameDiffers && diff.identical) {
            diff.firstDifferentFrame = i;
            diff.identical = false;
        }
        if (diff.differences.size() >= 20) break; // cap output
    }
    return diff;
}

// ── Persistence ────────────────────────────────────────────────

bool SimulationPlayback::Save(const std::string& name, const std::string& filePath) const {
    auto it = m_recordings.find(name);
    if (it == m_recordings.end()) return false;
    std::ofstream f(filePath);
    if (!f.is_open()) return false;
    const auto& rec = it->second;
    f << "name=" << rec.name << "\n";
    f << "frames=" << rec.frames.size() << "\n";
    for (const auto& frame : rec.frames) {
        f << "frame " << frame.frameNumber << " t=" << frame.timeStampSec
          << " dt=" << frame.deltaTime << "\n";
    }
    return true;
}

bool SimulationPlayback::Load(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) return false;
    SimRecording rec;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("name=", 0) == 0)   rec.name = line.substr(5);
    }
    if (!rec.name.empty()) m_recordings[rec.name] = std::move(rec);
    return true;
}

void SimulationPlayback::LoadRecording(const std::string& filePath) {
    Load(filePath);
}

// ── Callbacks ──────────────────────────────────────────────────

void SimulationPlayback::OnFrame(FrameCallback cb)   { m_onFrame  = std::move(cb); }
void SimulationPlayback::OnFinish(FinishCallback cb) { m_onFinish = std::move(cb); }

// ── Queries ────────────────────────────────────────────────────

const SimRecording* SimulationPlayback::GetRecording(const std::string& name) const {
    auto it = m_recordings.find(name);
    return (it != m_recordings.end()) ? &it->second : nullptr;
}

} // namespace Tools
