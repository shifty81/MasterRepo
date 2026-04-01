#pragma once
#include "Core/Math/Matrix.h"
#include <string>
#include <vector>
#include <cstdint>

namespace NF {

/// @brief A single joint in a skeletal hierarchy.
struct Bone {
    std::string Name;        ///< Unique bone name.
    int32_t     ParentIndex; ///< Index of the parent bone; -1 for root bones.
    Matrix4x4   BindPose;    ///< Inverse bind-pose transform.
};

/// @brief A flat array of bone transforms representing a single frame of pose data.
struct Pose {
    std::vector<Matrix4x4> BoneTransforms; ///< One entry per bone, in skeleton order.
};

/// @brief Hierarchical skeleton used to drive skinned mesh animation.
class Skeleton {
public:
    /// @brief Append a bone to the skeleton.
    /// @param bone  Bone descriptor to add.
    void AddBone(Bone bone);

    /// @brief Look up the index of a bone by name.
    /// @param name  The bone name to search for.
    /// @return Bone index, or -1 if not found.
    [[nodiscard]] int32_t GetBoneIndex(std::string_view name) const;

    /// @brief Retrieve a bone by index.
    /// @param index  Zero-based bone index.
    /// @return Const reference to the bone.
    [[nodiscard]] const Bone& GetBone(int32_t index) const;

    /// @brief Total number of bones in this skeleton.
    [[nodiscard]] int32_t BoneCount() const noexcept;

private:
    std::vector<Bone> m_Bones;
};

} // namespace NF
