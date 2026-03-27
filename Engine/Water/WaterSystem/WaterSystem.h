#pragma once
/**
 * @file WaterSystem.h
 * @brief Water simulation: buoyancy, swimming state, Gerstner waves, wetness.
 *
 * Features:
 *   - Gerstner wave superposition (up to 8 wave trains)
 *   - Query world-space water height and surface normal at any XZ position
 *   - Buoyancy: register rigid bodies with volume/density; applies impulse each Tick
 *   - Swimming state machine (wading, swimming, diving) per actor
 *   - Wetness accumulation / dry-out over time per entity
 *   - Water bodies (lakes, rivers, ocean) with flow velocity field
 *   - JSON serialisation of bodies and wave parameters
 *
 * Typical usage:
 * @code
 *   WaterSystem ws;
 *   ws.Init();
 *   ws.AddWaterBody({"ocean", 0.f, {1e9f,1e9f}, WaterBodyType::Ocean});
 *   ws.AddWave({0.8f, 12.f, 1.5f, 0.f});
 *   ws.RegisterBuoyancyBody(entityId, {1.0f, 0.5f, {0,0,0}});
 *   // per-frame
 *   ws.Tick(dt);
 *   float h = ws.GetWaterHeight(x, z);
 *   SwimState s = ws.GetSwimState(actorId);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class WaterBodyType : uint8_t { Ocean=0, Lake, River };

struct WaterBodyDesc {
    std::string     id;
    float           baseHeight{0.f};        ///< Y of calm surface
    float           extentXZ[2]{1000.f,1000.f}; ///< half-extents centred at origin
    WaterBodyType   type{WaterBodyType::Ocean};
    float           flowVelocity[3]{};      ///< for rivers
    float           density{1025.f};        ///< kg/m³ (salt water default)
};

/// Single Gerstner wave train
struct WaveDesc {
    float amplitude{0.5f};      ///< metres
    float wavelength{10.f};     ///< metres
    float speed{1.f};           ///< m/s
    float dirAngle{0.f};        ///< radians from +X
    float steepness{0.3f};      ///< Gerstner Q factor [0,1]
};

struct BuoyancyBodyDesc {
    uint32_t entityId{0};
    float    volume{1.f};       ///< m³
    float    mass{1.f};         ///< kg
    float    centreOfMass[3]{}; ///< local offset
    float    dragLinear{0.8f};
    float    dragAngular{0.5f};
    std::function<void(const float vel[3], const float torque[3])> applyImpulse;
};

struct BuoyancyState {
    uint32_t entityId{0};
    float    submersionRatio{0.f};   ///< 0=dry, 1=fully submerged
    bool     inWater{false};
};

enum class SwimState : uint8_t { Dry=0, Wading, Swimming, Diving };

struct ActorWaterState {
    uint32_t  actorId{0};
    SwimState swimState{SwimState::Dry};
    float     wetnessLevel{0.f};     ///< [0,1]
    float     depth{0.f};            ///< metres below surface (negative = above)
};

struct WaterSample {
    float height{0.f};
    float normal[3]{0,1,0};
    float flowVelocity[3]{};
    bool  inWater{false};
};

class WaterSystem {
public:
    WaterSystem();
    ~WaterSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Water bodies
    void          AddWaterBody(const WaterBodyDesc& desc);
    void          RemoveWaterBody(const std::string& id);
    bool          IsInWater(float x, float y, float z) const;

    // Waves
    void          AddWave(const WaveDesc& wave);
    void          ClearWaves();
    float         GetWaterHeight(float x, float z, float time=-1.f) const;
    void          GetWaterNormal(float x, float z, float outN[3], float time=-1.f) const;
    WaterSample   Sample(float x, float y, float z) const;

    // Buoyancy
    void          RegisterBuoyancyBody(const BuoyancyBodyDesc& desc);
    void          UnregisterBuoyancyBody(uint32_t entityId);
    BuoyancyState GetBuoyancyState(uint32_t entityId) const;

    // Swim / wetness
    void          SetActorPosition(uint32_t actorId, const float pos[3]);
    SwimState     GetSwimState(uint32_t actorId) const;
    float         GetWetness(uint32_t actorId) const;

    // Events
    using WaterEventCb = std::function<void(uint32_t actorId, SwimState newState)>;
    void          SetOnSwimStateChanged(WaterEventCb cb);

    // Serialisation
    bool          SaveToJSON(const std::string& path) const;
    bool          LoadFromJSON(const std::string& path);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
