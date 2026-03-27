#pragma once
/**
 * @file AnimationRetargeter.h
 * @brief Bone-name mapping between skeletons, pose copy/lerp, length scaling.
 *
 * Features:
 *   - RetargetMap: list of (sourceBoneName → targetBoneName) pairs
 *   - BuildMap(sourceSkeleton, targetSkeleton) → auto-matches by name/heuristic
 *   - SetCustomMapping(src, dst)
 *   - RetargetPose(srcPose, srcSkel, dstSkel) → dstPose
 *   - Bone length scaling: scale rotation contributions by limb-length ratio
 *   - Root motion retarget: optional root displacement scale
 *   - Lerp between two retargeted poses (blending)
 *   - Unmapped bone: keep target bind pose or zero
 *   - Supports max 256 bones per skeleton
 *
 * Typical usage:
 * @code
 *   AnimationRetargeter rt;
 *   rt.Init();
 *   rt.BuildMap(humanSkeleton, alienSkeleton);
 *   rt.SetCustomMapping("spine_01", "body_root");
 *   auto dstPose = rt.RetargetPose(srcPose, humanSkeleton, alienSkeleton);
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct RetargetBone {
    float position   [3]{};
    float orientation[4]{0,0,0,1};
    float scale      [3]{1,1,1};
};

struct RetargetSkeleton {
    uint32_t    boneCount{0};
    std::string boneNames[256];
    int32_t     parentIdx[256];
    float       bindLength[256]{};   ///< bone length in bind pose
};

class AnimationRetargeter {
public:
    AnimationRetargeter();
    ~AnimationRetargeter();

    void Init    ();
    void Shutdown();

    // Build mapping between two skeletons
    void BuildMap       (const RetargetSkeleton& src, const RetargetSkeleton& dst);
    void SetCustomMapping(const std::string& srcBone, const std::string& dstBone);
    void RemoveMapping  (const std::string& srcBone);
    void ClearMappings  ();

    // Retarget
    void RetargetPose(const RetargetBone* srcPose, const RetargetSkeleton& srcSkel,
                       const RetargetSkeleton& dstSkel,
                       RetargetBone* outDstPose) const;

    void LerpPoses(const RetargetBone* a, const RetargetBone* b, uint32_t boneCount,
                    float t, RetargetBone* out) const;

    // Options
    void SetLengthScalingEnabled(bool enabled);
    void SetRootMotionScale     (float scale);
    void SetUnmappedPolicy      (bool keepBindPose);  ///< true=bind, false=zero

    // Inspect
    uint32_t    MappingCount() const;
    std::string GetMappedTarget(const std::string& srcBone) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
