#pragma once
/**
 * @file ReplayRecorder.h
 * @brief Gameplay replay recording and deterministic playback.
 *
 * ReplayRecorder captures a timestamped stream of game-state snapshots and
 * input events for later deterministic playback or frame-by-frame inspection:
 *
 *   - ReplayFrame: timestamp (μs), compressed state blob, and input events.
 *   - InputEvent: player id, type (key/axis/button), code, and value.
 *   - Record(stateBlob, inputs): append a frame during gameplay.
 *   - Seek(timestampUs): random-access to the nearest recorded frame.
 *   - Play(speed): advance playback cursor at the given speed multiplier.
 *   - Export(path): write the full replay to a binary file.
 *   - Import(path): load a previously exported replay.
 *   - TrimTo(startUs, endUs): discard frames outside a time range.
 *   - Stats: total frames, duration, uncompressed size, compressed size.
 *   - Callbacks: OnFrameReady (playback), OnRecordingEnd.
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <functional>

namespace Tools {

// ── Input event ───────────────────────────────────────────────────────────
struct ReplayInputEvent {
    uint8_t  playerId{0};
    uint8_t  type{0};      ///< 0=key, 1=axis, 2=button
    uint16_t code{0};
    float    value{0.0f};
};

// ── Replay frame ──────────────────────────────────────────────────────────
struct ReplayFrame {
    uint64_t                     timestampUs{0}; ///< Microseconds from recording start
    std::vector<uint8_t>         stateBlob;      ///< Caller-provided serialised state
    std::vector<ReplayInputEvent> inputs;
    uint32_t                     frameIndex{0};
};

// ── Replay stats ──────────────────────────────────────────────────────────
struct ReplayStats {
    uint32_t totalFrames{0};
    uint64_t durationUs{0};
    size_t   uncompressedBytes{0};
    size_t   compressedBytes{0};
    double   avgFrameMs{0};
};

// ── Recorder ──────────────────────────────────────────────────────────────
class ReplayRecorder {
public:
    ReplayRecorder();
    ~ReplayRecorder();

    ReplayRecorder(const ReplayRecorder&) = delete;
    ReplayRecorder& operator=(const ReplayRecorder&) = delete;

    // ── recording ─────────────────────────────────────────────
    void StartRecording();
    void StopRecording();
    bool IsRecording() const;

    void Record(const std::vector<uint8_t>& stateBlob,
                const std::vector<ReplayInputEvent>& inputs,
                uint64_t timestampUs = 0);  ///< 0 = auto-timestamp

    // ── playback ──────────────────────────────────────────────
    bool     StartPlayback();
    void     StopPlayback();
    bool     IsPlaying() const;

    bool            Seek(uint64_t timestampUs);
    const ReplayFrame* CurrentFrame() const;
    bool            Advance(float speedMultiplier = 1.0f, uint64_t elapsedUs = 0);
    bool            AtEnd() const;

    // ── access ────────────────────────────────────────────────
    const ReplayFrame*          FrameAt(uint32_t index) const;
    const ReplayFrame*          FrameAtTime(uint64_t timestampUs) const;
    uint32_t                    FrameCount() const;
    uint64_t                    DurationUs() const;

    // ── editing ───────────────────────────────────────────────
    void TrimTo(uint64_t startUs, uint64_t endUs);
    void Clear();

    // ── persistence ───────────────────────────────────────────
    bool Export(const std::string& filePath) const;
    bool Import(const std::string& filePath);

    // ── stats ─────────────────────────────────────────────────
    ReplayStats Stats() const;

    // ── callbacks ─────────────────────────────────────────────
    using FrameReadyCb     = std::function<void(const ReplayFrame&)>;
    using RecordingEndCb   = std::function<void(const ReplayStats&)>;
    void OnFrameReady(FrameReadyCb cb);
    void OnRecordingEnd(RecordingEndCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
