#pragma once
/**
 * @file ParticleSystem.h
 * @brief CPU-side particle system for visual effects.
 *
 * Provides:
 *   - ParticleEmitter: burst or continuous emission; configurable rate,
 *     lifetime, initial velocity/spread, color-over-life, size-over-life
 *   - ParticleSystem: manages multiple emitters; pool-based particle storage;
 *     frame Update(dt) integrates physics (velocity + gravity + drag)
 *   - Render callback: pass draw data to the renderer without coupling
 *
 * Design: no GPU/OpenGL dependency here — rendering is via callback.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <array>

namespace Engine {

// ── Color ────────────────────────────────────────────────────────────────
struct ParticleColor { float r{1}, g{1}, b{1}, a{1}; };

// ── Single particle instance ─────────────────────────────────────────────
struct Particle {
    float x{0}, y{0}, z{0};        ///< World position
    float vx{0}, vy{0}, vz{0};     ///< Velocity (m/s)
    float life{1.0f};              ///< Remaining life [0,1] (1 = just spawned)
    float totalLife{1.0f};         ///< Total life in seconds
    float size{1.0f};              ///< Current size
    ParticleColor color;
    bool  active{false};
};

// ── Gradient: two-key color-over-life ─────────────────────────────────────
struct ColorGradient {
    ParticleColor start{1,1,1,1};
    ParticleColor end{1,1,1,0};

    ParticleColor Lerp(float t) const;
};

// ── Emitter configuration ─────────────────────────────────────────────────
struct EmitterConfig {
    // Position / spread
    float originX{0}, originY{0}, originZ{0};
    float spreadAngle{15.0f};    ///< Half-angle cone spread (degrees)

    // Velocity
    float speedMin{2.0f};
    float speedMax{5.0f};
    float gravityY{-9.8f};       ///< Gravity acceleration
    float drag{0.1f};            ///< Linear drag coefficient

    // Lifetime
    float lifetimeMin{0.5f};
    float lifetimeMax{2.0f};

    // Size
    float sizeStart{0.1f};
    float sizeEnd{0.0f};         ///< Shrinks to this size over lifetime

    // Color
    ColorGradient colorGradient;

    // Emission
    float     emitRate{20.0f};   ///< Particles per second (0 = burst only)
    uint32_t  burstCount{0};     ///< Extra burst on Start()
    uint32_t  maxParticles{500};

    bool      loop{true};
    float     duration{0.0f};    ///< Total emit duration (0 = infinite if loop)
};

// ── Draw data passed to renderer ─────────────────────────────────────────
struct ParticleDrawData {
    float x, y, z;
    float size;
    ParticleColor color;
};

using ParticleRenderCallback =
    std::function<void(const std::vector<ParticleDrawData>&)>;

// ── Emitter ───────────────────────────────────────────────────────────────
class ParticleEmitter {
public:
    explicit ParticleEmitter(const EmitterConfig& cfg = {});

    void Start();
    void Stop();
    void Reset();

    bool IsActive() const { return m_active; }

    /// Advance simulation by dt seconds.
    void Update(float dt);

    const EmitterConfig& Config() const { return m_cfg; }
    EmitterConfig&       Config()       { return m_cfg; }

    size_t AliveCount() const;
    const std::vector<Particle>& Particles() const { return m_pool; }

    std::string name;

private:
    EmitterConfig        m_cfg;
    std::vector<Particle> m_pool;
    float                m_accumulator{0.0f};
    float                m_elapsed{0.0f};
    bool                 m_active{false};

    void spawnParticle();
    static float randRange(float lo, float hi, uint32_t& rng);
    uint32_t     m_rng{12345};
};

// ── System ────────────────────────────────────────────────────────────────
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    // Emitter management
    uint32_t AddEmitter(const EmitterConfig& cfg, const std::string& name = "");
    void     RemoveEmitter(uint32_t id);
    ParticleEmitter* GetEmitter(uint32_t id);
    size_t           EmitterCount() const;

    void StartAll();
    void StopAll();

    // Per-frame update
    void Update(float dt);

    // Collect draw data for all active particles
    std::vector<ParticleDrawData> CollectDrawData() const;

    // Render callback (optional — called from Update if set)
    void SetRenderCallback(ParticleRenderCallback cb);

private:
    struct EmitterSlot {
        uint32_t        id{0};
        ParticleEmitter emitter;
    };
    std::vector<EmitterSlot>   m_emitters;
    uint32_t                   m_nextId{1};
    ParticleRenderCallback     m_renderCb;
};

} // namespace Engine
