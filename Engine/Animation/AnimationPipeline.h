#pragma once
// AnimationPipeline — Engine-level animation pipeline
//
// Extends the existing AnimationGraph (node-based blend tree) with:
//   • Skeleton / Rig system — bone hierarchy, bind pose, FK/IK
//   • Retargeting — map source skeleton → target skeleton
//   • Mocap data player — plays back BVH or proprietary capture clips
//   • Procedural animation layers — procedural additive IK, jiggle bones,
//     footstep IK, aim offsets
//   • Animation LOD — reduce bone count at distance
//
// Works alongside Engine::Animation::AnimationGraph which handles the
// blend-tree and state-machine aspects.

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Engine::Animation {

// ─────────────────────────────────────────────────────────────────────────────
// Skeleton / Rig
// ─────────────────────────────────────────────────────────────────────────────

using BoneIndex = uint16_t;
static constexpr BoneIndex kInvalidBone = 0xFFFF;

struct Transform {
    std::array<float,3> position = {};
    std::array<float,4> rotation = {0,0,0,1};  // quaternion xyzw
    std::array<float,3> scale    = {1,1,1};
};

struct Bone {
    std::string  name;
    BoneIndex    parent     = kInvalidBone;
    Transform    bindPose;      // rest pose (local space)
    Transform    localTransform; // current animated pose (local space)
    int          lodMinLevel = 0; // hide this bone below LOD level
};

class Skeleton {
public:
    Skeleton();

    BoneIndex AddBone(const Bone& bone);
    BoneIndex FindBone(const std::string& name) const;
    Bone*     GetBone(BoneIndex idx);
    const Bone* GetBone(BoneIndex idx) const;

    size_t BoneCount() const;

    /// Compute world-space transforms from local transforms (FK).
    void ForwardKinematics();

    /// Get world-space transform for a bone (call FK first).
    const Transform& GetWorldTransform(BoneIndex idx) const;

    void ResetToBindPose();

private:
    std::vector<Bone>      m_bones;
    std::vector<Transform> m_worldTransforms;
};

// ─────────────────────────────────────────────────────────────────────────────
// Mocap clip
// ─────────────────────────────────────────────────────────────────────────────

struct MocapKeyframe {
    float                   time = 0.f;       // seconds
    std::vector<Transform>  boneTransforms;   // one per bone in clip skeleton
};

struct MocapClip {
    std::string                  name;
    float                        duration    = 0.f;
    float                        sampleRate  = 30.f;  // fps
    std::vector<std::string>     boneNames;            // skeleton bone order
    std::vector<MocapKeyframe>   keyframes;

    bool  looping = false;

    /// Sample the clip at a given time (linear interpolation between keyframes).
    std::vector<Transform> Sample(float time) const;

    /// Load from a simple CSV (time,bx,by,bz,rx,ry,rz,rw per bone per row).
    bool LoadCSV(const std::string& path);
};

// ─────────────────────────────────────────────────────────────────────────────
// Retargeting
// ─────────────────────────────────────────────────────────────────────────────

struct RetargetMap {
    // source bone name → target bone name
    std::unordered_map<std::string,std::string> boneMap;
    float scaleRatio = 1.0f;
};

class Retargeter {
public:
    explicit Retargeter(RetargetMap map = {});

    /// Apply source skeleton's current pose onto target skeleton.
    void Apply(const Skeleton& source, Skeleton& target) const;

    void SetMap(RetargetMap map);
    const RetargetMap& GetMap() const;

private:
    RetargetMap m_map;
};

// ─────────────────────────────────────────────────────────────────────────────
// Procedural animation
// ─────────────────────────────────────────────────────────────────────────────

enum class ProceduralLayerType {
    JiggleBone,      // secondary motion on a bone
    FootstepIK,      // ground-contact IK
    AimOffset,       // additive look-at rotation
    BreathingCycle,  // subtle breathing sway
    Custom,
};

struct ProceduralLayerDesc {
    ProceduralLayerType type     = ProceduralLayerType::JiggleBone;
    std::string         boneName;
    float               strength = 1.0f;
    float               speed    = 5.0f;
    float               damping  = 0.8f;
    std::array<float,3> axis     = {0,1,0};
    bool                enabled  = true;
};

class ProceduralLayer {
public:
    explicit ProceduralLayer(ProceduralLayerDesc desc);

    void Update(Skeleton& skeleton, float dt);
    void Reset();

    bool IsEnabled() const { return m_desc.enabled; }
    void SetEnabled(bool v) { m_desc.enabled = v; }

    const ProceduralLayerDesc& GetDesc() const { return m_desc; }

private:
    ProceduralLayerDesc  m_desc;
    std::array<float,3>  m_velocity = {};
    float                m_phase    = 0.f;
};

// ─────────────────────────────────────────────────────────────────────────────
// AnimationPipelineConfig
// ─────────────────────────────────────────────────────────────────────────────

struct AnimationPipelineConfig {
    bool  enableProceduralLayers = true;
    bool  enableMocapBlending    = true;
    bool  enableLOD              = true;
    int   maxBonesPerSkeleton    = 256;
    float globalSpeedScale       = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
// AnimationPipeline
// ─────────────────────────────────────────────────────────────────────────────

class AnimationPipeline {
public:
    explicit AnimationPipeline(AnimationPipelineConfig cfg = {});
    ~AnimationPipeline();

    void Init();
    void Shutdown();

    // ── Per-frame ────────────────────────────────────────────────────────────

    void Update(float dt);

    // ── Skeleton management ──────────────────────────────────────────────────

    uint32_t CreateSkeleton(const std::string& name);
    Skeleton* GetSkeleton(uint32_t id);
    void      DestroySkeleton(uint32_t id);

    // ── Mocap ────────────────────────────────────────────────────────────────

    bool   LoadMocapClip(const std::string& id, const std::string& csvPath);
    bool   PlayMocapClip(uint32_t skeletonId, const std::string& clipId,
                          float blendTime = 0.2f);
    void   StopMocap(uint32_t skeletonId);
    float  GetMocapTime(uint32_t skeletonId) const;

    // ── Retargeting ──────────────────────────────────────────────────────────

    void   SetRetargetMap(uint32_t skeletonId, const RetargetMap& map);
    void   RetargetFrom(uint32_t sourceId, uint32_t targetId);

    // ── Procedural layers ────────────────────────────────────────────────────

    uint32_t AddProceduralLayer(uint32_t skeletonId,
                                 const ProceduralLayerDesc& desc);
    void     RemoveProceduralLayer(uint32_t skeletonId, uint32_t layerId);
    void     SetLayerEnabled(uint32_t skeletonId, uint32_t layerId, bool v);

    // ── LOD ─────────────────────────────────────────────────────────────────

    void SetLODLevel(uint32_t skeletonId, int level);

    // ── Stats ────────────────────────────────────────────────────────────────

    size_t SkeletonCount()        const;
    size_t ActiveMocapCount()     const;
    size_t TotalProceduralLayers() const;

    const AnimationPipelineConfig& GetConfig() const;

private:
    struct SkeletonState {
        uint32_t             id;
        std::string          name;
        Skeleton             skeleton;
        Retargeter           retargeter;
        std::vector<std::unique_ptr<ProceduralLayer>> procLayers;

        // Mocap playback
        std::string  currentClip;
        float        clipTime    = 0.f;
        float        blendWeight = 0.f;
        float        blendTime   = 0.2f;
        bool         playing     = false;
        int          lodLevel    = 0;
    };

    AnimationPipelineConfig                             m_cfg;
    std::vector<SkeletonState>                          m_skeletons;
    std::unordered_map<std::string,MocapClip>           m_clips;
    uint32_t                                            m_nextSkeletonId = 1;
    uint32_t                                            m_nextLayerId    = 1;

    SkeletonState* FindSkeleton(uint32_t id);
};

} // namespace Engine::Animation
