#pragma once
/**
 * @file ReplaySystem.h
 * @brief High-level game-state snapshot replay: frame capture, playback, speed control.
 *
 * Features:
 *   - FrameSnapshot: arbitrary serialisable state blob per entity
 *   - Capture mode: AppendFrame(entityStates) every Tick
 *   - Configurable snapshot rate (frames per second)
 *   - Configurable buffer duration (seconds); oldest frames evicted
 *   - Playback mode: interpolates between nearest frames
 *   - Playback speed: 0.25×, 0.5×, 1×, 2× (or arbitrary)
 *   - Seek: jump to timestamp
 *   - Loop playback option
 *   - Per-entity interpolation callback: replay system calls it with blended state
 *   - Pause/Resume
 *   - On-end callback
 *   - JSON export/import
 *   - Stats: frame count, total duration, memory estimate
 *
 * Typical usage:
 * @code
 *   ReplaySystem rs;
 *   rs.Init(10.f, 30);  // 10-second buffer, 30fps
 *   rs.StartCapture();
 *   rs.AppendFrame(gameState);   // called each tick
 *   rs.StopCapture();
 *   rs.SetOnApplyFrame([](const ReplayFrame& f){ ApplyToWorld(f); });
 *   rs.StartPlayback(false);
 *   rs.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct EntityState {
    uint32_t    entityId{0};
    std::vector<uint8_t> data;      ///< serialised transform / state bytes
};

struct ReplayFrame {
    float                    timestamp{0.f};
    std::vector<EntityState> entities;
};

enum class ReplayMode : uint8_t { Idle=0, Capturing, Playing, Paused };

class ReplaySystem {
public:
    ReplaySystem();
    ~ReplaySystem();

    void Init    (float bufferSeconds=30.f, uint32_t captureRate=20);
    void Shutdown();
    void Tick    (float dt);

    // Capture
    void StartCapture();
    void StopCapture ();
    bool IsCapturing () const;
    void AppendFrame (const std::vector<EntityState>& states);

    // Playback
    void StartPlayback(bool loop=false);
    void StopPlayback ();
    void Pause        (bool paused);
    bool IsPlaying    () const;
    bool IsPaused     () const;
    void Seek         (float timestamp);
    void SetPlaybackSpeed(float speed);
    float PlaybackSpeed() const;

    // Frame apply callback
    void SetOnApplyFrame(std::function<void(const ReplayFrame&)> cb);
    void SetOnEnd       (std::function<void()> cb);

    // Queries
    ReplayMode Mode         () const;
    float      CurrentTime  () const;
    float      Duration     () const;
    float      Progress     () const;
    uint32_t   FrameCount   () const;
    size_t     MemoryBytes  () const;

    // Serialise
    bool SaveJSON(const std::string& path) const;
    bool LoadJSON(const std::string& path);

    void Clear();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
