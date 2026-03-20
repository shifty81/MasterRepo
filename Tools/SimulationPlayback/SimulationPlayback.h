#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Simulation frame snapshot
// ──────────────────────────────────────────────────────────────

struct SimFrame {
    uint64_t    frameNumber   = 0;
    double      timeStampSec  = 0.0;
    double      deltaTime     = 0.0;

    // Entity positions (entity_id → x,y,z)
    std::unordered_map<uint32_t, std::array<float,3>> positions;
    // Arbitrary per-frame metrics
    std::unordered_map<std::string, double> metrics;
    // Events that fired this frame
    std::vector<std::string> events;
};

// ──────────────────────────────────────────────────────────────
// Simulation recording
// ──────────────────────────────────────────────────────────────

struct SimRecording {
    std::string            name;
    std::string            createdAt;
    std::vector<SimFrame>  frames;
    double TotalDuration() const;
    uint64_t FrameCount()  const { return frames.size(); }
};

// ──────────────────────────────────────────────────────────────
// Diff result between two recordings
// ──────────────────────────────────────────────────────────────

struct SimDiff {
    std::string recordingA;
    std::string recordingB;
    uint64_t    firstDifferentFrame = 0;
    bool        identical           = true;
    std::vector<std::string> differences;
};

// ──────────────────────────────────────────────────────────────
// SimulationPlayback — record, replay and compare simulations
// ──────────────────────────────────────────────────────────────

class SimulationPlayback {
public:
    // Recording
    void StartRecording(const std::string& name);
    void CaptureFrame(const SimFrame& frame);
    SimRecording StopRecording();

    // Playback
    void LoadRecording(const std::string& filePath);
    void Play(const std::string& name, float speed = 1.f);
    void Pause();
    void Stop();
    void SeekToFrame(uint64_t frame);
    void SeekToTime(double seconds);

    // Comparison
    SimDiff Compare(const std::string& nameA, const std::string& nameB) const;

    // Persistence
    bool Save(const std::string& name, const std::string& filePath) const;
    bool Load(const std::string& filePath);

    // Callbacks
    using FrameCallback  = std::function<void(const SimFrame&)>;
    using FinishCallback = std::function<void(const std::string& name)>;
    void OnFrame(FrameCallback cb);
    void OnFinish(FinishCallback cb);

    // Queries
    const SimRecording* GetRecording(const std::string& name) const;
    bool                IsPlaying()   const { return m_playing; }
    bool                IsRecording() const { return m_recording; }
    uint64_t            CurrentFrame() const { return m_currentFrame; }

    // Singleton
    static SimulationPlayback& Get();

private:
    std::unordered_map<std::string, SimRecording> m_recordings;
    std::string  m_activeRecording;
    std::string  m_playbackRecording;
    bool         m_recording    = false;
    bool         m_playing      = false;
    float        m_playSpeed    = 1.f;
    uint64_t     m_currentFrame = 0;
    FrameCallback  m_onFrame;
    FinishCallback m_onFinish;
};

} // namespace Tools
