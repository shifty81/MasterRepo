#pragma once
/**
 * @file ProceduralAnimation.h
 * @brief Runtime procedural animation — IK solving, foot placement, ragdoll
 *        blending, and secondary motion (jiggle, look-at, spring bones).
 *
 * Works alongside the existing AnimationController (Engine/Animation) by
 * layering procedural overrides on top of sampled animation data.
 *
 * Features:
 *   - 2-bone and FABRIK IK chains
 *   - Terrain-aware foot planting (raycasts for ground height)
 *   - Look-at (head / spine chain tracks a world-space target)
 *   - Spring bone physics (hair, cloth, tails)
 *   - Ragdoll blend-in/out on death or impact
 *
 * Typical usage:
 * @code
 *   ProceduralAnimation pa;
 *   pa.Init();
 *   uint32_t ikId = pa.CreateIKChain("left_foot", {"thigh_l","shin_l","foot_l"});
 *   pa.SetIKTarget(ikId, groundPos);
 *   pa.CreateLookAt("head_look", "neck", "head", cameraPos);
 *   pa.Solve(skeletonPose, dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── IK algorithm ─────────────────────────────────────────────────────────────

enum class IKSolver : uint8_t { TwoBone = 0, FABRIK = 1 };

// ── IK chain ─────────────────────────────────────────────────────────────────

struct IKChain {
    uint32_t              id{0};
    std::string           name;
    std::vector<std::string> joints;  ///< root → tip order
    float                 target[3]{};
    float                 poleTarget[3]{};
    float                 weight{1.f};   ///< 0 = disabled, 1 = full
    IKSolver              solver{IKSolver::TwoBone};
    uint32_t              fabrikIterations{10};
    bool                  enabled{true};
};

// ── Look-at constraint ────────────────────────────────────────────────────────

struct LookAtConstraint {
    uint32_t              id{0};
    std::string           name;
    std::vector<std::string> chain;     ///< joints to rotate (neck, head, …)
    float                 target[3]{};
    float                 weight{0.7f};
    float                 clampAngleDeg{60.f};
    bool                  enabled{true};
};

// ── Spring bone ───────────────────────────────────────────────────────────────

struct SpringBone {
    uint32_t    id{0};
    std::string jointName;
    float       stiffness{50.f};
    float       damping{5.f};
    float       gravity[3]{0.f, -9.8f, 0.f};
    bool        enabled{true};
};

// ── Procedural animation context ─────────────────────────────────────────────

struct ProceduralAnimContext {
    uint32_t skeletonId{0};
};

// ── ProceduralAnimation ───────────────────────────────────────────────────────

class ProceduralAnimation {
public:
    ProceduralAnimation();
    ~ProceduralAnimation();

    void Init();
    void Shutdown();

    // ── IK chains ─────────────────────────────────────────────────────────────

    uint32_t CreateIKChain(const std::string& name,
                            const std::vector<std::string>& joints,
                            IKSolver solver = IKSolver::TwoBone);
    void     DestroyIKChain(uint32_t id);
    void     SetIKTarget(uint32_t id, const float target[3]);
    void     SetIKPoleTarget(uint32_t id, const float poleTarget[3]);
    void     SetIKWeight(uint32_t id, float weight);
    void     SetIKEnabled(uint32_t id, bool enabled);
    IKChain  GetIKChain(uint32_t id) const;

    // ── Look-at ───────────────────────────────────────────────────────────────

    uint32_t CreateLookAt(const std::string& name,
                           const std::vector<std::string>& chain,
                           const float target[3]);
    void     SetLookAtTarget(uint32_t id, const float target[3]);
    void     SetLookAtWeight(uint32_t id, float weight);
    void     SetLookAtEnabled(uint32_t id, bool enabled);

    // ── Spring bones ──────────────────────────────────────────────────────────

    uint32_t AddSpringBone(const std::string& jointName,
                            float stiffness = 50.f, float damping = 5.f);
    void     SetSpringBoneEnabled(uint32_t id, bool enabled);

    // ── Terrain foot placement ────────────────────────────────────────────────

    void EnableTerrainFootPlacement(uint32_t leftIK, uint32_t rightIK);
    void DisableTerrainFootPlacement();
    void SetTerrainHeight(float height);

    // ── Solve ─────────────────────────────────────────────────────────────────

    /// Apply all active constraints to the skeleton pose.
    void Solve(float dt);

    // ── Ragdoll ───────────────────────────────────────────────────────────────

    void BlendInRagdoll(float blendTimeSeconds = 0.5f);
    void BlendOutRagdoll(float blendTimeSeconds = 0.5f);
    bool IsRagdollActive() const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnSolveComplete(std::function<void()> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
