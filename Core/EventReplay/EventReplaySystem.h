#pragma once
/**
 * @file EventReplaySystem.h
 * @brief Deterministic event recording and replay for debugging and testing.
 *
 * Records all engine events (input, spawns, network messages) with timestamps
 * during a play session.  The recording can be saved to disk and replayed
 * exactly — useful for reproducing bugs, automated regression testing, and
 * demo playback.
 *
 * Supports:
 *   - Start/stop recording
 *   - Replay at 1×, 2×, 0.5× speed
 *   - Seek to a specific timestamp
 *   - Filter by event category
 *   - JSON serialization (human-readable) or binary (compact)
 *
 * Typical usage:
 * @code
 *   EventReplaySystem replay;
 *   replay.Init();
 *   replay.StartRecording();
 *   // ... game runs ...
 *   replay.StopRecording();
 *   replay.SaveToFile("Replays/session_001.json");
 *
 *   replay.LoadFromFile("Replays/session_001.json");
 *   replay.StartPlayback();
 *   // Each frame:
 *   replay.Tick(dt);
 *   auto events = replay.DrainPending();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core {

// ── An event record ───────────────────────────────────────────────────────────

struct ReplayEvent {
    uint64_t    timestampMs{0};
    std::string category;      ///< "input", "spawn", "net", "custom", …
    std::string type;          ///< event type identifier
    std::string payload;       ///< JSON-encoded payload
};

// ── Replay status ─────────────────────────────────────────────────────────────

enum class ReplayState : uint8_t {
    Idle = 0, Recording, PlayingBack, Paused
};

// ── EventReplaySystem ─────────────────────────────────────────────────────────

class EventReplaySystem {
public:
    EventReplaySystem();
    ~EventReplaySystem();

    void Init();
    void Shutdown();

    // ── Recording ─────────────────────────────────────────────────────────────

    void StartRecording();
    void StopRecording();
    void RecordEvent(const ReplayEvent& event);
    void RecordEvent(const std::string& category, const std::string& type,
                     const std::string& payload = "{}");

    // ── Playback ──────────────────────────────────────────────────────────────

    void StartPlayback(float speed = 1.f);
    void StopPlayback();
    void PausePlayback();
    void ResumePlayback();
    void SetPlaybackSpeed(float speed);  ///< 0.1 – 10.0
    void SeekToMs(uint64_t ms);

    /// Advance playback clock; returns events that fire this tick.
    std::vector<ReplayEvent> Tick(float deltaSeconds);

    /// Drain events that were ready but not yet consumed.
    std::vector<ReplayEvent> DrainPending();

    // ── State ─────────────────────────────────────────────────────────────────

    ReplayState GetState()          const;
    bool        IsRecording()       const;
    bool        IsPlayingBack()     const;
    uint64_t    CurrentTimeMs()     const;  ///< playback cursor
    uint64_t    TotalDurationMs()   const;
    uint32_t    RecordedEventCount()const;
    float       PlaybackProgress()  const;  ///< 0–1

    // ── Filtering ─────────────────────────────────────────────────────────────

    void EnableCategory(const std::string& category);
    void DisableCategory(const std::string& category);

    // ── Serialization ─────────────────────────────────────────────────────────

    bool SaveToFile(const std::string& path) const;
    bool LoadFromFile(const std::string& path);
    void ClearRecording();

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnEventFired(std::function<void(const ReplayEvent&)> cb);
    void OnPlaybackEnd(std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
