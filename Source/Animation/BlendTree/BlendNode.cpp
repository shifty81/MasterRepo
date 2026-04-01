#include "Animation/BlendTree/BlendNode.h"
#include <algorithm>

namespace NF {

// ---------------------------------------------------------------------------
// ClipNode
// ---------------------------------------------------------------------------

ClipNode::ClipNode(std::string clipName, float playbackSpeed) noexcept
    : ClipName(std::move(clipName))
    , PlaybackSpeed(playbackSpeed)
    , CurrentTime(0.f) {}

void ClipNode::Evaluate(float dt, Pose& outPose) {
    CurrentTime += dt * PlaybackSpeed;

    // Stub: a real implementation would sample the named clip at CurrentTime
    // and write joint transforms into outPose.
    (void)outPose;
}

// ---------------------------------------------------------------------------
// BlendNode2
// ---------------------------------------------------------------------------

BlendNode2::BlendNode2(std::shared_ptr<BlendNode> nodeA,
                       std::shared_ptr<BlendNode> nodeB,
                       float alpha) noexcept
    : Alpha(alpha)
    , NodeA(std::move(nodeA))
    , NodeB(std::move(nodeB)) {}

void BlendNode2::Evaluate(float dt, Pose& outPose) {
    if (!NodeA || !NodeB) return;

    Pose poseA = outPose;
    Pose poseB = outPose;
    NodeA->Evaluate(dt, poseA);
    NodeB->Evaluate(dt, poseB);

    const float a = std::clamp(Alpha, 0.f, 1.f);
    const std::size_t count = std::min(poseA.BoneTransforms.size(),
                                       poseB.BoneTransforms.size());

    outPose.BoneTransforms.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        // Per-element lerp of matrix components.
        auto& mA = poseA.BoneTransforms[i].M;
        auto& mB = poseB.BoneTransforms[i].M;
        auto& mO = outPose.BoneTransforms[i].M;
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                mO[c][r] = mA[c][r] + a * (mB[c][r] - mA[c][r]);
    }
}

} // namespace NF
