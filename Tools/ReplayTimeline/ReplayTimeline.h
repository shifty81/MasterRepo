#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace Tools {

struct ReplayFrame {
    uint32_t frameIndex = 0;
    float    simTime    = 0.0f;
    float    deltaTime  = 0.0f;
    std::vector<uint8_t> stateBlob;
    std::unordered_map<std::string, std::string> metadata;
};

enum class ReplayState { Idle, Recording, Playing, Paused };

class ReplayTimeline {
public:
    void StartRecording();
    void StopRecording();
    void RecordFrame(float simTime, float dt, const std::vector<uint8_t>& blob,
                     const std::unordered_map<std::string, std::string>& meta = {});

    void Play();
    void Pause();
    void Stop();
    void SeekToFrame(uint32_t frameIndex);
    void SeekToTime(float simTime);
    void SetPlaybackSpeed(float speed);
    const ReplayFrame* GetCurrentFrame() const;
    const ReplayFrame* Step();
    bool IsAtEnd() const;

    ReplayState GetState()          const;
    uint32_t    FrameCount()        const;
    float       TotalDuration()     const;
    uint32_t    CurrentFrameIndex() const;
    float       CurrentTime()       const;
    float       PlaybackSpeed()     const;

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);
    void Clear();

    using FrameCallback = std::function<void(const ReplayFrame&)>;
    void SetOnFrameChanged(FrameCallback cb);

private:
    std::vector<ReplayFrame> m_frames;
    ReplayState              m_state         = ReplayState::Idle;
    uint32_t                 m_currentIndex  = 0;
    float                    m_playbackSpeed = 1.0f;
    FrameCallback            m_frameChangedCb;
};

} // namespace Tools
