#pragma once
/**
 * @file ParticleSystem.h
 * @brief CPU particle emitter: spawn rates, lifetime, forces, colour/size over lifetime.
 *
 * Features:
 *   - Emitter shapes: Point, Sphere, Box, Cone, Ring
 *   - Spawn: rate-over-time, burst, one-shot
 *   - Per-particle: position, velocity, acceleration, lifetime, size, colour, rotation
 *   - Curves: size-over-lifetime, colour-over-lifetime (gradient), speed-scale
 *   - Forces: gravity (global), point attractor/repulsor, noise displacement
 *   - Collision: simple ground plane collision with bounce
 *   - Emitter transform: world position + orientation
 *   - Particle capacity: fixed pool (no dynamic alloc per frame)
 *   - Multiple simultaneous emitters (each has its own pool)
 *   - Render output: flat float arrays (positions, colours, sizes) for sprite batch
 *   - On-particle-die callback
 *
 * Typical usage:
 * @code
 *   ParticleSystem ps;
 *   ps.Init();
 *   uint32_t emId = ps.CreateEmitter(EmitterDesc{EmitterShape::Cone, 50, 2.f});
 *   ps.SetPosition(emId, {0,1,0});
 *   ps.SetGravity(emId, {0,-9.81f,0});
 *   ps.SetEmitting(emId, true);
 *   ps.Tick(dt);
 *   auto& data = ps.GetRenderData(emId);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class EmitterShape : uint8_t {
    Point=0, Sphere, Box, Cone, Ring
};

struct ColorKey {
    float t{0.f};       ///< 0-1 lifetime fraction
    float r{1},g{1},b{1},a{1};
};

struct SizeKey {
    float t{0.f};
    float size{1.f};
};

struct EmitterDesc {
    EmitterShape shape        {EmitterShape::Point};
    uint32_t     capacity    {256};
    float        emitRate    {50.f};  ///< particles/second
    float        burstCount  {0.f};   ///< extra burst on spawn
    float        lifeMin     {1.f};
    float        lifeMax     {2.f};
    float        speedMin    {1.f};
    float        speedMax    {3.f};
    float        sizeMin     {0.1f};
    float        sizeMax     {0.3f};
    float        shapeRadius {0.5f};
    float        coneAngle   {30.f};
    std::vector<ColorKey> colorKeys;
    std::vector<SizeKey>  sizeKeys;
    bool         worldSpace  {true};
    bool         collide     {false};
    float        groundY     {0.f};
    float        bounce      {0.3f};
};

struct ParticleRenderData {
    std::vector<float> positions;   ///< xyz per particle
    std::vector<float> colours;     ///< rgba per particle
    std::vector<float> sizes;       ///< scalar per particle
    std::vector<float> rotations;   ///< radians per particle
    uint32_t           count{0};
};

class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Emitter management
    uint32_t CreateEmitter (const EmitterDesc& desc);
    void     DestroyEmitter(uint32_t id);
    bool     HasEmitter    (uint32_t id) const;

    // Emitter control
    void SetEmitting  (uint32_t id, bool emitting);
    void Burst        (uint32_t id, uint32_t count);
    void StopAndClear (uint32_t id);
    bool IsEmitting   (uint32_t id) const;

    // Emitter transform
    void SetPosition   (uint32_t id, const float pos[3]);
    void SetOrientation(uint32_t id, const float quat[4]);

    // Per-emitter physics
    void SetGravity  (uint32_t id, const float g[3]);
    void SetWindForce(uint32_t id, const float w[3]);

    // Edit desc at runtime
    void UpdateDesc(uint32_t id, const EmitterDesc& desc);

    // Render data (call after Tick)
    const ParticleRenderData& GetRenderData(uint32_t id) const;

    // Query
    uint32_t ActiveCount(uint32_t id) const;
    uint32_t EmitterCount()           const;

    // Callback
    void SetOnParticleDie(uint32_t id, std::function<void(const float pos[3])> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
