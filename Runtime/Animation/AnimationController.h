#pragma once
/**
 * @file AnimationController.h
 * @brief Per-entity runtime animation state manager.
 *
 * AnimationController bridges the ECS entity layer with
 * Engine::Animation::AnimationPipeline.  Each entity gets an
 * AnimationState that tracks the active clip, blend weight,
 * playback speed, and loop mode.  The controller's Update()
 * is called once per tick from the animation system.
 */

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace Runtime {

using EntityID = uint32_t;

/// Playback mode for an animation clip.
enum class AnimPlayMode { Once, Loop, PingPong, Hold };

/// State of a single animation slot on an entity.
struct AnimState {
    EntityID    entityId{0};
    std::string clipName;
    float       time{0.0f};       ///< Current time within the clip (seconds)
    float       duration{1.0f};   ///< Clip duration (seconds)
    float       speed{1.0f};      ///< Playback speed multiplier
    float       weight{1.0f};     ///< Blend weight 0–1
    AnimPlayMode mode{AnimPlayMode::Loop};
    bool        playing{false};
    bool        finished{false};
};

/// Event fired when a clip ends (Once mode) or loops.
struct AnimEvent {
    EntityID    entityId{0};
    std::string clipName;
    bool        looped{false};   ///< true = looped, false = finished
};

using AnimEventCallback = std::function<void(const AnimEvent&)>;

/// AnimationController — per-entity runtime animation state manager.
///
/// Clips are registered globally; each entity can play one or more clips
/// simultaneously (slots 0 = base layer, 1+ = additive layers).
/// Update() advances all playing clips each tick.
class AnimationController {
public:
    AnimationController();
    ~AnimationController();

    // ── clip registry ─────────────────────────────────────────
    /// Register a named clip with its duration.
    void RegisterClip(const std::string& name, float duration);
    bool HasClip(const std::string& name) const;
    size_t ClipCount() const;

    // ── playback control ──────────────────────────────────────
    void Play(EntityID entity, const std::string& clip,
              AnimPlayMode mode = AnimPlayMode::Loop,
              float speed = 1.0f, int layer = 0);
    void Stop(EntityID entity, int layer = 0);
    void StopAll(EntityID entity);
    void SetSpeed(EntityID entity, float speed, int layer = 0);
    void SetWeight(EntityID entity, float weight, int layer = 0);
    void SeekTo(EntityID entity, float time, int layer = 0);
    bool IsPlaying(EntityID entity, int layer = 0) const;

    // ── tick ─────────────────────────────────────────────────
    /// Advance all playing animations by dt seconds.
    void Update(float dt);

    // ── query ────────────────────────────────────────────────
    const AnimState* GetState(EntityID entity, int layer = 0) const;
    /// Normalised playback position [0,1].
    float NormalizedTime(EntityID entity, int layer = 0) const;

    // ── callbacks ─────────────────────────────────────────────
    void OnAnimEvent(AnimEventCallback cb);

    // ── management ────────────────────────────────────────────
    void RemoveEntity(EntityID entity);
    void Clear();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
