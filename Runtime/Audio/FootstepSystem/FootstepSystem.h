#pragma once
/**
 * @file FootstepSystem.h
 * @brief Surface-aware footstep audio and VFX dispatch.
 *
 * Features:
 *   - Actors register with a stride length; system emits step events on threshold
 *   - Surface tag lookup: maps surface material → sound cue + particle FX
 *   - Left/right foot alternation
 *   - Speed-scaled volume and rate (walk vs run vs sprint)
 *   - Landing impact events with fall-height scaling
 *   - Jump-launch whoosh events
 *   - Multiple terrain blend weights (e.g. 60% grass + 40% mud)
 *   - Callback-driven (no direct audio/VFX dependency)
 *
 * Typical usage:
 * @code
 *   FootstepSystem fs;
 *   fs.Init();
 *   fs.RegisterSurface("concrete", {"sfx/step_concrete", "vfx/dust"});
 *   fs.RegisterSurface("water",    {"sfx/step_splash",   "vfx/splash"});
 *   uint32_t id = fs.RegisterActor({actorId, 0.65f});
 *   fs.SetOnStep([](const StepEvent& e){ PlaySound(e.soundCue, e.position); });
 *   // per-frame
 *   fs.UpdateActor(id, position, velocity, surfaceTag, isGrounded);
 *   fs.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class FootSide : uint8_t { Left=0, Right };

struct SurfaceAudioDesc {
    std::string soundCue;           ///< identifier for audio subsystem
    std::string particleEffect;     ///< identifier for VFX subsystem
    float       volumeMultiplier{1.f};
    float       pitchVariance{0.1f}; ///< ± random pitch offset
};

struct StepEvent {
    uint32_t    actorId{0};
    FootSide    foot{FootSide::Left};
    float       position[3]{};
    std::string soundCue;
    std::string particleEffect;
    float       volume{1.f};
    float       pitch{1.f};
    float       speed{0.f};         ///< actor speed at time of step
};

struct LandingEvent {
    uint32_t    actorId{0};
    float       position[3]{};
    std::string soundCue;
    float       impactScale{1.f};   ///< 0-1 based on fall height
    float       fallHeight{0.f};    ///< metres
};

struct ActorFootstepDesc {
    uint32_t    actorId{0};
    float       strideLength{0.65f};    ///< metres per step
    float       walkSpeed{1.8f};        ///< m/s threshold walk/run boundary
    float       runSpeed{4.5f};         ///< m/s threshold run/sprint boundary
    std::string defaultSurface{"concrete"};
};

struct ActorFootstepState {
    uint32_t    handle{0};
    ActorFootstepDesc desc;
    float       distanceSinceLastStep{0.f};
    FootSide    nextFoot{FootSide::Left};
    float       lastY{0.f};             ///< for fall-height detection
    bool        wasGrounded{true};
    float       airTime{0.f};
    float       currentSpeed{0.f};
    std::string currentSurface;
};

struct SurfaceBlend {
    std::string surface;
    float       weight{1.f};
};

class FootstepSystem {
public:
    FootstepSystem();
    ~FootstepSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Surface registry
    void    RegisterSurface(const std::string& tag, const SurfaceAudioDesc& desc);
    void    UnregisterSurface(const std::string& tag);

    // Actor management
    uint32_t RegisterActor(const ActorFootstepDesc& desc);
    void     UnregisterActor(uint32_t handle);
    void     UpdateActor(uint32_t handle, const float position[3],
                         const float velocity[3], const std::string& surfaceTag,
                         bool isGrounded);
    void     UpdateActorBlended(uint32_t handle, const float position[3],
                                const float velocity[3],
                                const std::vector<SurfaceBlend>& surfaces,
                                bool isGrounded);

    // Events
    using StepCb    = std::function<void(const StepEvent&)>;
    using LandingCb = std::function<void(const LandingEvent&)>;
    using JumpCb    = std::function<void(uint32_t actorId, const float pos[3])>;

    void SetOnStep(StepCb cb);
    void SetOnLanding(LandingCb cb);
    void SetOnJump(JumpCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
