#pragma once
/**
 * @file GPUParticleSystem.h
 * @brief GPU-driven particle system using compute shaders or transform feedback.
 *
 * Supports millions of particles with:
 *   - Per-emitter: spawn rate, lifetime, initial velocity cone, colour gradient
 *   - Forces: gravity, wind, attractor/repeller point forces, turbulence noise
 *   - Particle-particle collision via spatial hash (optional, expensive)
 *   - Lit particles (receive scene lighting)
 *   - Sub-emitters (spawn child particles on death)
 *   - Sorted transparent rendering
 *
 * Typical usage:
 * @code
 *   GPUParticleSystem gps;
 *   gps.Init();
 *   GPUEmitterDesc desc;
 *   desc.spawnRate  = 1000;
 *   desc.lifetime   = {0.5f, 2.f};
 *   desc.startSpeed = {1.f, 5.f};
 *   uint32_t emitterId = gps.CreateEmitter(desc);
 *   gps.SetEmitterPosition(emitterId, {0,1,0});
 *   gps.Update(dt);
 *   gps.Render();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Range helper ─────────────────────────────────────────────────────────────

struct FloatRange { float min{0.f}, max{1.f}; };

// ── Colour keyframe ───────────────────────────────────────────────────────────

struct ParticleColourKey {
    float    time{0.f};   ///< 0–1 over lifetime
    uint32_t colour{0xFFFFFFFFu}; ///< RGBA
};

// ── Force type ────────────────────────────────────────────────────────────────

enum class ParticleForceType : uint8_t {
    Gravity = 0, Wind, Attractor, Repeller, Turbulence, Vortex
};

// ── Force descriptor ──────────────────────────────────────────────────────────

struct ParticleForce {
    uint32_t         id{0};
    ParticleForceType type{ParticleForceType::Gravity};
    float            position[3]{};      ///< for Attractor/Repeller/Vortex
    float            direction[3]{0,0,0};///< for Wind
    float            strength{9.8f};
    float            radius{10.f};       ///< influence radius
    bool             enabled{true};
};

// ── Emitter descriptor ────────────────────────────────────────────────────────

struct GPUEmitterDesc {
    float       position[3]{};
    float       direction[3]{0,1,0};     ///< spawn cone axis
    float       coneAngleDeg{30.f};
    uint32_t    spawnRate{100};          ///< particles/second
    FloatRange  lifetime{0.5f, 2.f};
    FloatRange  startSpeed{1.f, 5.f};
    FloatRange  startSize{0.05f, 0.2f};
    std::vector<ParticleColourKey> colourGradient;
    std::string spritePath;              ///< texture; empty = round billboard
    bool        loop{true};
    float       duration{-1.f};          ///< -1 = infinite
    bool        lit{false};
};

// ── Emitter state ─────────────────────────────────────────────────────────────

struct GPUEmitterState {
    uint32_t    id{0};
    GPUEmitterDesc desc;
    float       elapsedSeconds{0.f};
    uint32_t    liveParticleCount{0};
    bool        active{true};
};

// ── GPUParticleSystem ─────────────────────────────────────────────────────────

class GPUParticleSystem {
public:
    GPUParticleSystem();
    ~GPUParticleSystem();

    void Init();
    void Shutdown();

    // ── Emitters ──────────────────────────────────────────────────────────────

    uint32_t        CreateEmitter(const GPUEmitterDesc& desc);
    void            DestroyEmitter(uint32_t emitterId);
    void            SetEmitterPosition(uint32_t id, const float pos[3]);
    void            SetEmitterDirection(uint32_t id, const float dir[3]);
    void            SetEmitterActive(uint32_t id, bool active);
    GPUEmitterState GetEmitterState(uint32_t id) const;
    std::vector<uint32_t> ActiveEmitterIds() const;

    // ── Forces ────────────────────────────────────────────────────────────────

    uint32_t  AddForce(const ParticleForce& force);
    void      RemoveForce(uint32_t forceId);
    void      SetForceEnabled(uint32_t forceId, bool enabled);

    // ── Simulation ────────────────────────────────────────────────────────────

    void Update(float dt);
    void Render();

    // ── Stats ─────────────────────────────────────────────────────────────────

    uint32_t TotalLiveParticles() const;
    uint32_t EmitterCount()       const;

    // ── Global ────────────────────────────────────────────────────────────────

    void PauseAll();
    void ResumeAll();
    void ClearAll();
    void SetMaxParticles(uint32_t max);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnEmitterFinished(std::function<void(uint32_t id)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
