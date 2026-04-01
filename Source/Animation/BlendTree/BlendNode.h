#pragma once
#include "Animation/Skeleton/Skeleton.h"
#include <string>
#include <memory>

namespace NF {

/// @brief Abstract node in an animation blend tree.
class BlendNode {
public:
    virtual ~BlendNode() = default;

    /// @brief Evaluate this node and write the resulting pose.
    /// @param dt      Time delta in seconds since the last evaluation.
    /// @param outPose Pose to be filled or blended into.
    virtual void Evaluate(float dt, Pose& outPose) = 0;
};

/// @brief Leaf node that plays back a named animation clip.
class ClipNode : public BlendNode {
public:
    std::string ClipName;      ///< Name of the animation clip to play.
    float       PlaybackSpeed; ///< Multiplier applied to playback rate (1 = normal).
    float       CurrentTime;   ///< Current playback position in seconds.

    explicit ClipNode(std::string clipName = {},
                      float playbackSpeed   = 1.f) noexcept;

    /// @brief Advance CurrentTime and write the clip's pose at that instant.
    void Evaluate(float dt, Pose& outPose) override;
};

/// @brief 1-D blend node that linearly interpolates between two child nodes.
class BlendNode2 : public BlendNode {
public:
    /// @brief Blend weight: 0 = fully NodeA, 1 = fully NodeB.
    float Alpha{0.f};

    std::shared_ptr<BlendNode> NodeA; ///< Source node (Alpha = 0).
    std::shared_ptr<BlendNode> NodeB; ///< Target node (Alpha = 1).

    BlendNode2(std::shared_ptr<BlendNode> nodeA,
               std::shared_ptr<BlendNode> nodeB,
               float alpha = 0.f) noexcept;

    /// @brief Evaluate both children and blend the resulting poses by Alpha.
    void Evaluate(float dt, Pose& outPose) override;
};

} // namespace NF
