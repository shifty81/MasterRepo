#pragma once
/**
 * @file RagdollSystem.h
 * @brief Per-bone rigid bodies, joint limits, animation blend, death-trigger, impulse.
 *
 * Features:
 *   - Ragdoll definition: bone count, per-bone mass / collider (capsule), parent index
 *   - Joint limits: cone half-angle and twist limit per joint
 *   - Blend weight: 0=full animation, 1=full ragdoll; lerp between
 *   - Death trigger: SetDead(ragdollId) → switches blend to physics
 *   - Apply impulse to any bone in world space
 *   - Tick: semi-implicit Euler per rigid body + joint constraints (sequential impulse)
 *   - Get bone world transform (position + quaternion)
 *   - Set bone initial pose from animation (array of transforms)
 *   - On-sleep callback (all bones below linear velocity threshold)
 *   - Multiple ragdolls per system
 *
 * Typical usage:
 * @code
 *   RagdollSystem rs;
 *   rs.Init();
 *   RagdollDesc d; d.boneCount=20; // fill d.bones...
 *   uint32_t id = rs.CreateRagdoll(d);
 *   rs.SetPose(id, animPoseArray);
 *   rs.SetBlendWeight(id, 0.f); // full animation
 *   rs.SetDead(id);              // switch to physics
 *   rs.AddImpulse(id, 2, {0,800,200}); // hit spine
 *   rs.Tick(dt);
 *   auto tr = rs.GetBoneTransform(id, 0);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct BoneDesc {
    int32_t parentIdx    {-1};
    float   localPos     [3]{};
    float   mass         {3.f};
    float   capsuleRadius{0.08f};
    float   capsuleLength{0.3f};
    float   coneHalfAngle{45.f};  ///< degrees
    float   twistLimit   {30.f};  ///< degrees
};

struct RagdollDesc {
    uint32_t  boneCount{0};
    BoneDesc  bones[64];
    float     gravity[3]{0,-9.81f,0};
    float     sleepThreshold{0.05f};  ///< m/s
};

struct BoneTransform {
    float position   [3]{};
    float orientation[4]{0,0,0,1};
};

class RagdollSystem {
public:
    RagdollSystem();
    ~RagdollSystem();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    uint32_t CreateRagdoll(const RagdollDesc& desc);
    void     DestroyRagdoll(uint32_t id);
    bool     HasRagdoll    (uint32_t id) const;

    void SetPose       (uint32_t id, const BoneTransform* poseArray); ///< bone count elements
    void SetBlendWeight(uint32_t id, float w);       ///< 0=anim, 1=physics
    float GetBlendWeight(uint32_t id) const;
    void SetDead       (uint32_t id);
    bool IsDead        (uint32_t id) const;
    bool IsSleeping    (uint32_t id) const;

    void AddImpulse(uint32_t id, uint32_t boneIdx, const float impulse[3]);

    BoneTransform GetBoneTransform(uint32_t id, uint32_t boneIdx) const;
    uint32_t      BoneCount       (uint32_t id) const;

    void SetOnSleep(std::function<void(uint32_t id)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
