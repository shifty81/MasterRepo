#pragma once
/**
 * @file InputRecorder.h
 * @brief Record & replay input streams: serialise/deserialise, playback speed control.
 *
 * Features:
 *   - Record mode: captures InputEvent frames with timestamps
 *   - Replay mode: playback at 1× or scaled speed; loop option
 *   - InputEvent: action press/release, axis value, timestamp
 *   - Recording: Start/StopRecording, saved as binary or JSON
 *   - Playback: StartReplay(path/buffer), Tick advances position
 *   - Playback callbacks: OnEvent fired for each replayed event
 *   - Seek: jump to timestamp
 *   - On-end callback for replay completion
 *   - Compatible with InputSystem: IInputProvider interface
 *   - Stats: total frames, total duration, current position
 *
 * Typical usage:
 * @code
 *   InputRecorder ir;
 *   ir.Init();
 *   ir.StartRecording();
 *   ir.RecordEvent({InputEventType::ActionPressed, "Jump", 0.f, 1.23f});
 *   ir.StopRecording();
 *   ir.SaveJSON("demo.json");
 *
 *   ir.LoadJSON("demo.json");
 *   ir.SetPlaybackSpeed(1.5f);
 *   ir.StartReplay();
 *   ir.SetOnEvent([](const InputEvent& e){ HandleInput(e); });
 *   ir.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class InputEventType : uint8_t {
    ActionPressed=0, ActionReleased, AxisChanged, MouseMoved, MouseScrolled
};

struct InputEvent {
    InputEventType type  {InputEventType::ActionPressed};
    std::string    name;     ///< action/axis name
    float          value {0.f};
    float          timestamp{0.f};
    float          x    {0.f};  ///< mouse/axis X
    float          y    {0.f};  ///< mouse/axis Y
};

enum class RecorderState : uint8_t { Idle=0, Recording, Replaying };

class InputRecorder {
public:
    InputRecorder();
    ~InputRecorder();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);   ///< advances replay position

    // Recording
    void StartRecording();
    void StopRecording ();
    bool IsRecording()  const;
    void RecordEvent(const InputEvent& event);

    // Serialise
    bool SaveJSON  (const std::string& path) const;
    bool LoadJSON  (const std::string& path);
    bool SaveBinary(const std::string& path) const;
    bool LoadBinary(const std::string& path);

    // Replay
    void StartReplay(bool loop=false);
    void StopReplay ();
    void PauseReplay(bool paused);
    bool IsReplaying()  const;
    bool IsPaused()     const;
    void Seek(float timestamp);
    void SetPlaybackSpeed(float speed);
    float PlaybackSpeed() const;

    // Callbacks
    void SetOnEvent(std::function<void(const InputEvent&)> cb);
    void SetOnEnd  (std::function<void()> cb);

    // Stats
    RecorderState State()          const;
    float         Duration()       const;
    float         CurrentTime()    const;
    float         Progress()       const;  ///< 0-1
    uint32_t      EventCount()     const;
    uint32_t      FrameCount()     const;

    // Buffer access
    const std::vector<InputEvent>& Events() const;
    void Clear();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
