#pragma once
/**
 * @file PatrolSystem.h
 * @brief Patrol-path AI: waypoints, wait time, modes (loop/ping-pong/random), FOV alert.
 *
 * Features:
 *   - PatrolDesc: mode (Loop/PingPong/Random), speed, waitTime, fovAngle, fovRange
 *   - Create(desc) → patrolId
 *   - AddWaypoint(id, pos[3], waitOverride)
 *   - Tick(dt): advance all patrols
 *   - GetCurrentTarget(id) → float[3]
 *   - GetCurrentPos(id) → float[3] (caller must set with UpdatePos)
 *   - UpdatePos(id, pos[3]): feed actual agent position
 *   - IsWaiting(id) / IsAlerted(id)
 *   - AlertTarget(id, pos[3]): inject a threat position → alerted state
 *   - ClearAlert(id)
 *   - SetOnAlert/SetOnResumePatrol callbacks
 *   - GetWaypoints(id) → vector of float[3]
 *   - Pause/Resume(id)
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

enum class PatrolMode : uint8_t { Loop=0, PingPong, Random };

struct PatrolDesc {
    PatrolMode mode      {PatrolMode::Loop};
    float      speed     {3.f};
    float      waitTime  {1.f};
    float      fovAngle  {60.f};  ///< degrees half-angle
    float      fovRange  {10.f};
    bool       autoAlert {true};  ///< alert when target enters FOV
};

class PatrolSystem {
public:
    PatrolSystem();
    ~PatrolSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    uint32_t Create (const PatrolDesc& desc);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    void AddWaypoint (uint32_t id, const float pos[3], float waitOverride=-1);
    void ClearWaypoints(uint32_t id);
    const std::vector<std::vector<float>>& GetWaypoints(uint32_t id) const;

    void         UpdatePos        (uint32_t id, const float pos[3]);
    const float* GetCurrentPos    (uint32_t id) const;
    const float* GetCurrentTarget (uint32_t id) const;

    bool IsWaiting (uint32_t id) const;
    bool IsAlerted (uint32_t id) const;
    bool IsPaused  (uint32_t id) const;

    void Pause (uint32_t id);
    void Resume(uint32_t id);

    void AlertTarget(uint32_t id, const float threatPos[3]);
    void ClearAlert (uint32_t id);

    // Check if a world-space point is within this patrol's FOV
    bool PointInFOV(uint32_t id, const float point[3]) const;

    void SetOnAlert        (std::function<void(uint32_t id, const float pos[3])> cb);
    void SetOnResumePatrol (std::function<void(uint32_t id)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
