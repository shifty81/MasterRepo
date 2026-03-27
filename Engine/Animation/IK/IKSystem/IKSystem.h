#pragma once
/**
 * @file IKSystem.h
 * @brief Inverse Kinematics: 2-bone analytical solver + FABRIK chain solver, end-effector goals.
 *
 * Features:
 *   - Solver types: TwoBone (analytical, exact), FABRIK (iterative, N-bone chain)
 *   - End-effector goal: target position + optional target orientation
 *   - Per-bone constraints: cone angle limits (hinge/ball-and-socket)
 *   - Reach detection: clamp when target out of reach
 *   - Pole vector: guide knee/elbow direction for 2-bone
 *   - Multiple IK chains per skeleton instance
 *   - Blend weight: IK vs. animated pose (0-1)
 *   - Output: modified JointPose array (feed into SkeletonSystem)
 *   - FABRIK iterations configurable (default 10)
 *   - Per-chain enable/disable
 *
 * Typical usage:
 * @code
 *   IKSystem ik;
 *   ik.Init();
 *   uint32_t chain = ik.AddChain({IKSolverType::TwoBone,
 *       {rootJoint, midJoint, tipJoint}, poleVec});
 *   ik.SetGoal(chain, targetPos, targetRot);
 *   ik.SetBlendWeight(chain, 0.8f);
 *   ik.Solve(chain, skeletonPose);  // modifies pose in-place
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct JointPose;  ///< defined in SkeletonSystem.h

enum class IKSolverType : uint8_t { TwoBone=0, FABRIK };

struct IKChainDesc {
    IKSolverType solver  {IKSolverType::TwoBone};
    std::vector<uint32_t> jointIndices;   ///< ordered root→tip
    float        poleVector[3]{0,1,0};    ///< 2-bone knee guide
    uint32_t     maxIterations{10};       ///< FABRIK
    float        tolerance{0.001f};       ///< FABRIK convergence
    float        blendWeight{1.f};        ///< 0=anim, 1=IK
    bool         constrainRoot{false};    ///< keep root fixed
};

struct IKGoal {
    float position   [3]{};
    float orientation[4]{0,0,0,1};
    bool  useOrientation{false};
};

class IKSystem {
public:
    IKSystem();
    ~IKSystem();

    void Init    ();
    void Shutdown();

    // Chain management
    uint32_t AddChain   (const IKChainDesc& desc);
    void     RemoveChain(uint32_t chainId);
    bool     HasChain   (uint32_t chainId) const;

    // Goal
    void SetGoal       (uint32_t chainId, const float targetPos[3],
                        const float targetOri[4]=nullptr);
    void SetPoleVector (uint32_t chainId, const float pole[3]);
    void SetBlendWeight(uint32_t chainId, float weight);
    void SetEnabled    (uint32_t chainId, bool enabled);

    // Solve — modifies pose in-place
    // pose: array of JointPose indexed by joint index
    void Solve    (uint32_t chainId, std::vector<JointPose>& pose) const;
    void SolveAll (std::vector<JointPose>& pose) const;

    // Queries
    float    GetBlendWeight(uint32_t chainId) const;
    bool     IsEnabled     (uint32_t chainId) const;
    bool     InReach       (uint32_t chainId, const float targetPos[3]) const;
    uint32_t ChainCount()                     const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
