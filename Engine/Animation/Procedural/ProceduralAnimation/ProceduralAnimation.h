#pragma once
/**
 * @file ProceduralAnimation.h
 * @brief Runtime-computed animation: IK chains, look-at, foot placement, secondary motion.
 *
 * Features:
 *   - RegisterBoneChain(chainId, boneIds[], count): define an IK chain
 *   - SetIKTarget(agentId, chainId, targetPos, weight): place end-effector goal
 *   - SolveIK(agentId, chainId, iterations): FABRIK solve → outJointPositions[]
 *   - SetLookAtTarget(agentId, headBoneId, targetPos, weight): aim bone toward target
 *   - SolveLookAt(agentId) → outRotation
 *   - SetFootPlacementEnabled(agentId, on)
 *   - UpdateFootPlacement(agentId, leftFootPos, rightFootPos, groundQuery): raycast-based
 *   - AddSecondaryBone(boneId, stiffness, damping, inertia): spring-follow parent
 *   - TickSecondary(agentId, dt): advance secondary motion simulation
 *   - GetSecondaryOffset(agentId, boneId) → Vec3
 *   - SetBoneWorldPos(agentId, boneId, pos) / GetBoneWorldPos(agentId, boneId) → Vec3
 *   - Reset(agentId) / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct PAVec3 { float x,y,z; };
struct PAQuat { float x,y,z,w; };

class ProceduralAnimation {
public:
    ProceduralAnimation();
    ~ProceduralAnimation();

    void Init    ();
    void Shutdown();

    // IK chains
    void RegisterBoneChain(uint32_t chainId, const uint32_t* boneIds, uint32_t count);
    void SetIKTarget      (uint32_t agentId, uint32_t chainId, PAVec3 targetPos, float weight=1);
    void SolveIK          (uint32_t agentId, uint32_t chainId,
                           uint32_t iterations, std::vector<PAVec3>& outJoints);

    // Look-at
    void    SetLookAtTarget(uint32_t agentId, uint32_t headBoneId, PAVec3 target, float weight=1);
    PAQuat  SolveLookAt    (uint32_t agentId);

    // Foot placement
    void SetFootPlacementEnabled(uint32_t agentId, bool on);
    void UpdateFootPlacement    (uint32_t agentId,
                                 PAVec3 leftFoot, PAVec3 rightFoot,
                                 std::function<float(PAVec3 origin,PAVec3 dir)> groundQuery);
    PAVec3 GetAdjustedFootPos   (uint32_t agentId, bool leftFoot) const;

    // Secondary motion
    void   AddSecondaryBone (uint32_t boneId, float stiffness, float damping, float inertia);
    void   TickSecondary    (uint32_t agentId, float dt);
    PAVec3 GetSecondaryOffset(uint32_t agentId, uint32_t boneId) const;

    // Bone state
    void   SetBoneWorldPos(uint32_t agentId, uint32_t boneId, PAVec3 pos);
    PAVec3 GetBoneWorldPos(uint32_t agentId, uint32_t boneId) const;

    void Reset(uint32_t agentId);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
