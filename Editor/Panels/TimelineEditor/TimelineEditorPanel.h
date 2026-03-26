#pragma once
/**
 * @file TimelineEditorPanel.h
 * @brief Visual timeline editor panel for cinematics and animation clips.
 *
 * Rendered using the Atlas custom UI (no ImGui — see Atlas Rule).
 * The panel shows:
 *   - Tracks (one per animated property or audio cue)
 *   - Keyframes as draggable diamonds on each track
 *   - Playhead position with scrubbing
 *   - Transport controls: play, pause, stop, loop
 *   - Zoom and horizontal scroll
 *
 * Integrates with `Engine/Cinematic/CinematicSystem` for live preview.
 *
 * Typical usage:
 * @code
 *   TimelineEditorPanel panel;
 *   panel.Init(cinematicSystemPtr);
 *   panel.LoadSequence(seqId);
 *   // Each frame:
 *   panel.OnMouseMove(x, y);
 *   panel.OnMouseButton(0, pressed, x, y);
 *   panel.Draw(ctx, x, y, w, h);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine { class CinematicSystem; }

namespace Editor {

// ── A timeline track ──────────────────────────────────────────────────────────

struct TimelineTrack {
    uint32_t    id{0};
    std::string name;
    std::string propertyPath;  ///< e.g. "camera.position.x"
    std::vector<float> keyframeTimes;
    uint32_t    colour{0xFF8844FF}; ///< RGBA
    bool        muted{false};
    bool        locked{false};
    bool        expanded{true};
};

// ── Panel state ───────────────────────────────────────────────────────────────

struct TimelineEditorState {
    float    playhead{0.f};      ///< current playback position in seconds
    float    zoomLevel{1.f};     ///< pixels per second
    float    scrollOffsetX{0.f};
    float    duration{10.f};     ///< total timeline length in seconds
    bool     playing{false};
    bool     looping{false};
    uint32_t selectedTrack{0};
    uint32_t selectedKeyframe{~0u};
    uint32_t activeSeqId{0};
};

// ── Minimal render context stub ───────────────────────────────────────────────

struct UIRenderContext;

// ── TimelineEditorPanel ───────────────────────────────────────────────────────

class TimelineEditorPanel {
public:
    TimelineEditorPanel();
    ~TimelineEditorPanel();

    void Init(Engine::CinematicSystem* cinematic = nullptr);
    void Shutdown();

    // ── Scene loading ─────────────────────────────────────────────────────────

    void LoadSequence(uint32_t seqId);
    void Clear();

    // ── Track management ──────────────────────────────────────────────────────

    uint32_t AddTrack(const std::string& name, const std::string& propertyPath = "");
    void     RemoveTrack(uint32_t trackId);
    void     AddKeyframe(uint32_t trackId, float time);
    void     RemoveKeyframe(uint32_t trackId, float time);
    void     MoveKeyframe(uint32_t trackId, float oldTime, float newTime);

    TimelineTrack GetTrack(uint32_t trackId) const;
    std::vector<TimelineTrack> AllTracks() const;

    // ── Transport ─────────────────────────────────────────────────────────────

    void Play();
    void Pause();
    void Stop();
    void SetLooping(bool loop);
    void SetPlayhead(float time);
    float GetPlayhead() const;

    // ── Render ────────────────────────────────────────────────────────────────

    void Draw(int panelX, int panelY, int panelW, int panelH);
    void Tick(float deltaSeconds);

    // ── Input ─────────────────────────────────────────────────────────────────

    void OnMouseMove(int x, int y);
    void OnMouseButton(int button, bool pressed, int x, int y);
    void OnMouseScroll(float dx, float dy);
    void OnKey(int key, bool pressed);

    // ── State ─────────────────────────────────────────────────────────────────

    TimelineEditorState GetState() const;
    void SetDuration(float seconds);
    void SetZoom(float pixelsPerSecond);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnPlayheadMoved(std::function<void(float time)> cb);
    void OnKeyframeSelected(std::function<void(uint32_t track, float time)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Editor
