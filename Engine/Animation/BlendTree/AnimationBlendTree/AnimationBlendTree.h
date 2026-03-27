#pragma once
/**
 * @file AnimationBlendTree.h
 * @brief Per-bone blend weights, additive layers, 1D/2D blend spaces, masked blending.
 *
 * Features:
 *   - Blend nodes: Single clip, 1D BlendSpace, 2D BlendSpace, Additive layer
 *   - Per-bone mask: define which joints a blend node affects (0-1 weight per joint)
 *   - Additive blend: base pose + delta pose (scaled by blend weight)
 *   - 1D blend space: lerp between N poses along a single parameter axis
 *   - 2D blend space: barycentric blend of up to 9 poses on a 2D parameter plane
 *   - Override node: replace specific joints entirely
 *   - Blend tree instance: shares definition, each instance has its own parameters
 *   - Parameter map (float parameters driving blend weights/spaces)
 *   - Output pose: flat array of JointPose (compatible with SkeletonSystem)
 *   - JSON tree definition loading
 *
 * Typical usage:
 * @code
 *   AnimationBlendTree abt;
 *   abt.Init();
 *   uint32_t treeId = abt.CreateTree("humanoid");
 *   uint32_t locomotion = abt.Add1DBlendSpace(treeId, "speed", {"idle","walk","run"}, {0,1,3});
 *   uint32_t aim = abt.AddAdditiveLayer(treeId, locomotion, "aimLayer", 1.f);
 *   uint32_t instId = abt.CreateInstance(treeId);
 *   abt.SetParam(instId, "speed", 1.5f);
 *   abt.Evaluate(instId, dt, outPose);  // outPose → feed to SkeletonSystem
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct JointPose;  ///< defined in SkeletonSystem.h

struct BlendParam {
    std::string name;
    float       value{0.f};
};

struct BlendSpaceEntry1D {
    std::string clipId;
    float       position{0.f};  ///< parameter value at this clip
};

struct BlendSpaceEntry2D {
    std::string clipId;
    float       posX{0.f};
    float       posY{0.f};
};

enum class BlendNodeType : uint8_t {
    Clip=0, BlendSpace1D, BlendSpace2D, Additive, Override
};

struct BlendNodeDesc {
    BlendNodeType type{BlendNodeType::Clip};
    std::string   name;
    std::string   clipId;                ///< Clip nodes
    std::string   paramName;             ///< 1D/2D parameter name
    std::string   paramNameY;            ///< 2D second axis
    std::vector<BlendSpaceEntry1D> entries1D;
    std::vector<BlendSpaceEntry2D> entries2D;
    float         additiveWeight{1.f};
    std::vector<float> jointMask;        ///< per-joint 0-1 weight, empty=all
    uint32_t      baseNodeId{0};         ///< Additive: node id of base pose
    uint32_t      additiveNodeId{0};     ///< Additive: node id of delta pose
};

class AnimationBlendTree {
public:
    AnimationBlendTree();
    ~AnimationBlendTree();

    void Init    ();
    void Shutdown();

    // Tree definition
    uint32_t CreateTree  (const std::string& skeletonDefId);
    void     DestroyTree (uint32_t treeId);
    uint32_t AddNode     (uint32_t treeId, const BlendNodeDesc& desc);
    void     SetRootNode (uint32_t treeId, uint32_t nodeId);

    // Convenience helpers
    uint32_t AddClip         (uint32_t treeId, const std::string& name, const std::string& clipId);
    uint32_t Add1DBlendSpace (uint32_t treeId, const std::string& paramName,
                               const std::vector<std::string>& clipIds,
                               const std::vector<float>& positions);
    uint32_t Add2DBlendSpace (uint32_t treeId, const std::string& paramX,
                               const std::string& paramY,
                               const std::vector<BlendSpaceEntry2D>& entries);
    uint32_t AddAdditiveLayer(uint32_t treeId, uint32_t baseNodeId,
                               uint32_t additiveNodeId, float weight=1.f);

    // Instances
    uint32_t CreateInstance (uint32_t treeId);
    void     DestroyInstance(uint32_t instanceId);
    bool     HasInstance    (uint32_t instanceId) const;

    // Parameter control
    void  SetParam  (uint32_t instanceId, const std::string& name, float value);
    float GetParam  (uint32_t instanceId, const std::string& name) const;
    void  SetWeight (uint32_t instanceId, uint32_t nodeId, float weight);

    // Evaluation — fills outPose (caller provides vector sized to joint count)
    using ClipSampleFn = std::function<bool(const std::string& clipId, float t,
                                             std::vector<JointPose>& outPose)>;
    void SetClipSampleFn(ClipSampleFn fn);
    void Evaluate(uint32_t instanceId, float dt, std::vector<JointPose>& outPose);

    // JSON definition loading
    bool LoadTree(const std::string& jsonPath);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
