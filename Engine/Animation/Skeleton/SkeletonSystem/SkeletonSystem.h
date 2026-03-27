#pragma once
/**
 * @file SkeletonSystem.h
 * @brief Skeleton hierarchy: joints, bind pose, local→world transform propagation.
 *
 * Features:
 *   - Skeleton definition: named joints with parent index and bind-pose transforms
 *   - Pose: per-joint local TRS (translation, rotation-quaternion, scale)
 *   - Forward kinematics: propagate local poses to world-space matrices
 *   - Bind-pose inverse matrices for skinning (pre-computed)
 *   - Skin matrices: worldMat * inverseBind per joint (ready for GPU)
 *   - Multiple skeleton instances share the same definition
 *   - JSON skeleton definition loading
 *   - Skeleton blending: lerp between two poses (blend-shape driven)
 *   - Find joint by name, get world position/matrix of joint
 *
 * Typical usage:
 * @code
 *   SkeletonSystem ss;
 *   ss.Init();
 *   ss.LoadDefinition("Data/Skeletons/humanoid.json");
 *   uint32_t instId = ss.CreateInstance("humanoid");
 *   ss.SetLocalPose(instId, jointIdx, pos, rot, scale);
 *   ss.UpdateWorldMatrices(instId);
 *   const float* skinMats = ss.GetSkinMatrices(instId);  // upload to GPU
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

// Column-major 4x4 matrix as flat float[16]
struct JointPose {
    float translation[3]{0,0,0};
    float rotation   [4]{0,0,0,1}; ///< quaternion xyzw
    float scale      [3]{1,1,1};
};

struct JointDef {
    std::string name;
    int32_t     parentIndex{-1};  ///< -1 = root
    JointPose   bindPose;
};

struct SkeletonDef {
    std::string              id;
    std::vector<JointDef>    joints;
};

struct SkeletonInstance {
    uint32_t              instanceId{0};
    std::string           defId;
    std::vector<JointPose>localPoses;       ///< per-joint local TRS
    std::vector<float>    worldMatrices;    ///< flat float[16] per joint
    std::vector<float>    skinMatrices;     ///< worldMat * inverseBind per joint
    bool                  dirty{true};
};

class SkeletonSystem {
public:
    SkeletonSystem();
    ~SkeletonSystem();

    void Init();
    void Shutdown();

    // Definitions
    void RegisterDefinition(const SkeletonDef& def);
    bool LoadDefinition    (const std::string& jsonPath);
    bool HasDefinition     (const std::string& defId) const;
    const SkeletonDef* GetDefinition(const std::string& defId) const;

    // Instances
    uint32_t CreateInstance (const std::string& defId);
    void     DestroyInstance(uint32_t instanceId);
    bool     HasInstance    (uint32_t instanceId) const;

    // Pose control
    void SetLocalPose(uint32_t instanceId, uint32_t jointIdx,
                      const float translation[3], const float rotation[4],
                      const float scale[3]=nullptr);
    void SetLocalPose(uint32_t instanceId, uint32_t jointIdx,
                      const JointPose& pose);
    void ResetToBindPose(uint32_t instanceId);

    // Forward kinematics
    void UpdateWorldMatrices(uint32_t instanceId);

    // Skinning
    const float* GetSkinMatrices (uint32_t instanceId) const; ///< flat float[16*N]
    const float* GetWorldMatrix  (uint32_t instanceId, uint32_t jointIdx) const;
    void         GetJointWorldPos(uint32_t instanceId, uint32_t jointIdx,
                                   float out[3]) const;

    // Queries
    int32_t  FindJoint   (uint32_t instanceId, const std::string& name) const;
    uint32_t JointCount  (uint32_t instanceId) const;

    // Blending
    void BlendPoses(uint32_t instA, uint32_t instB, uint32_t instOut, float t);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
