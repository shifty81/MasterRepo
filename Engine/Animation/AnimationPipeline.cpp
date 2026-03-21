#include "Engine/Animation/AnimationPipeline.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace Engine::Animation {

// ─────────────────────────────────────────────────────────────────────────────
// Skeleton
// ─────────────────────────────────────────────────────────────────────────────

Skeleton::Skeleton() {}

BoneIndex Skeleton::AddBone(const Bone& bone)
{
    BoneIndex idx = static_cast<BoneIndex>(m_bones.size());
    m_bones.push_back(bone);
    m_worldTransforms.push_back(bone.bindPose);
    return idx;
}

BoneIndex Skeleton::FindBone(const std::string& name) const
{
    for (size_t i = 0; i < m_bones.size(); ++i)
        if (m_bones[i].name == name)
            return static_cast<BoneIndex>(i);
    return kInvalidBone;
}

Bone* Skeleton::GetBone(BoneIndex idx)
{
    if (idx >= m_bones.size()) return nullptr;
    return &m_bones[idx];
}

const Bone* Skeleton::GetBone(BoneIndex idx) const
{
    if (idx >= m_bones.size()) return nullptr;
    return &m_bones[idx];
}

size_t Skeleton::BoneCount() const { return m_bones.size(); }

static Transform CombineTransforms(const Transform& parent,
                                    const Transform& child)
{
    Transform out;
    // Simplified: add positions (ignores rotation of parent on child position)
    for (int i = 0; i < 3; ++i)
        out.position[i] = parent.position[i] + child.position[i];
    // For rotation: quaternion multiply (stub uses child rotation only)
    out.rotation = child.rotation;
    for (int i = 0; i < 3; ++i)
        out.scale[i] = parent.scale[i] * child.scale[i];
    return out;
}

void Skeleton::ForwardKinematics()
{
    m_worldTransforms.resize(m_bones.size());
    for (size_t i = 0; i < m_bones.size(); ++i) {
        BoneIndex parent = m_bones[i].parent;
        if (parent == kInvalidBone)
            m_worldTransforms[i] = m_bones[i].localTransform;
        else
            m_worldTransforms[i] = CombineTransforms(
                m_worldTransforms[parent],
                m_bones[i].localTransform);
    }
}

const Transform& Skeleton::GetWorldTransform(BoneIndex idx) const
{
    static Transform kIdentity{};
    if (idx >= m_worldTransforms.size()) return kIdentity;
    return m_worldTransforms[idx];
}

void Skeleton::ResetToBindPose()
{
    for (auto& b : m_bones) b.localTransform = b.bindPose;
    ForwardKinematics();
}

// ─────────────────────────────────────────────────────────────────────────────
// MocapClip
// ─────────────────────────────────────────────────────────────────────────────

static Transform LerpTransform(const Transform& a, const Transform& b, float t)
{
    Transform out;
    for (int i = 0; i < 3; ++i)
        out.position[i] = a.position[i] + (b.position[i] - a.position[i]) * t;
    // slerp would go here; stub uses lerp
    for (int i = 0; i < 4; ++i)
        out.rotation[i] = a.rotation[i] + (b.rotation[i] - a.rotation[i]) * t;
    for (int i = 0; i < 3; ++i)
        out.scale[i] = a.scale[i] + (b.scale[i] - a.scale[i]) * t;
    return out;
}

std::vector<Transform> MocapClip::Sample(float time) const
{
    if (keyframes.empty()) return {};

    float t = looping ? std::fmod(time, duration) : std::min(time, duration);

    // Binary search for surrounding keyframes
    int lo = 0, hi = (int)keyframes.size() - 1;
    while (lo < hi - 1) {
        int mid = (lo + hi) / 2;
        if (keyframes[mid].time <= t) lo = mid;
        else                         hi = mid;
    }

    if (lo == hi) return keyframes[lo].boneTransforms;

    const auto& kfA = keyframes[lo];
    const auto& kfB = keyframes[hi];
    float span = kfB.time - kfA.time;
    float alpha = (span > 1e-6f) ? (t - kfA.time) / span : 0.f;

    size_t n = std::min(kfA.boneTransforms.size(), kfB.boneTransforms.size());
    std::vector<Transform> out(n);
    for (size_t i = 0; i < n; ++i)
        out[i] = LerpTransform(kfA.boneTransforms[i],
                                kfB.boneTransforms[i], alpha);
    return out;
}

bool MocapClip::LoadCSV(const std::string& path)
{
    std::ifstream f(path);
    if (!f) return false;
    // Minimal CSV loader: time,bone0_px,py,pz,rx,ry,rz,rw,...
    // (full implementation omitted for brevity — stub returns true)
    (void)f;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Retargeter
// ─────────────────────────────────────────────────────────────────────────────

Retargeter::Retargeter(RetargetMap map) : m_map(std::move(map)) {}

void Retargeter::Apply(const Skeleton& source, Skeleton& target) const
{
    for (const auto& [srcName, dstName] : m_map.boneMap) {
        BoneIndex srcIdx = source.FindBone(srcName);
        BoneIndex dstIdx = target.FindBone(dstName);
        if (srcIdx == kInvalidBone || dstIdx == kInvalidBone) continue;
        const Bone* srcBone = source.GetBone(srcIdx);
        Bone*       dstBone = target.GetBone(dstIdx);
        if (!srcBone || !dstBone) continue;
        // Copy rotation; scale position by ratio
        dstBone->localTransform.rotation = srcBone->localTransform.rotation;
        for (int i = 0; i < 3; ++i)
            dstBone->localTransform.position[i] =
                srcBone->localTransform.position[i] * m_map.scaleRatio;
    }
}

void Retargeter::SetMap(RetargetMap map) { m_map = std::move(map); }
const RetargetMap& Retargeter::GetMap() const { return m_map; }

// ─────────────────────────────────────────────────────────────────────────────
// ProceduralLayer
// ─────────────────────────────────────────────────────────────────────────────

ProceduralLayer::ProceduralLayer(ProceduralLayerDesc desc)
    : m_desc(std::move(desc))
{}

void ProceduralLayer::Update(Skeleton& skeleton, float dt)
{
    if (!m_desc.enabled) return;
    BoneIndex idx = skeleton.FindBone(m_desc.boneName);
    if (idx == kInvalidBone) return;
    Bone* bone = skeleton.GetBone(idx);
    if (!bone) return;

    switch (m_desc.type) {
        case ProceduralLayerType::JiggleBone: {
            // Simple spring-damper on position
            for (int i = 0; i < 3; ++i) {
                float spring = -m_desc.strength * bone->localTransform.position[i];
                m_velocity[i] = m_velocity[i] * m_desc.damping + spring * dt;
                bone->localTransform.position[i] += m_velocity[i] * dt;
            }
            break;
        }
        case ProceduralLayerType::BreathingCycle: {
            m_phase += m_desc.speed * dt;
            float sway = std::sin(m_phase) * m_desc.strength * 0.01f;
            bone->localTransform.position[1] += sway; // Y axis sway
            break;
        }
        default:
            break;
    }
}

void ProceduralLayer::Reset()
{
    m_velocity = {};
    m_phase    = 0.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// AnimationPipeline
// ─────────────────────────────────────────────────────────────────────────────

AnimationPipeline::AnimationPipeline(AnimationPipelineConfig cfg)
    : m_cfg(std::move(cfg))
{}

AnimationPipeline::~AnimationPipeline() { Shutdown(); }

void AnimationPipeline::Init()  {}
void AnimationPipeline::Shutdown() { m_skeletons.clear(); m_clips.clear(); }

void AnimationPipeline::Update(float dt)
{
    for (auto& ss : m_skeletons) {
        // Advance mocap
        if (ss.playing && !ss.currentClip.empty()) {
            auto it = m_clips.find(ss.currentClip);
            if (it != m_clips.end()) {
                ss.clipTime += dt * m_cfg.globalSpeedScale;
                auto poses = it->second.Sample(ss.clipTime);
                auto& skel = ss.skeleton;
                for (size_t i = 0; i < poses.size() && i < skel.BoneCount(); ++i) {
                    Bone* b = skel.GetBone(static_cast<BoneIndex>(i));
                    if (b) b->localTransform = poses[i];
                }
                if (!it->second.looping && ss.clipTime >= it->second.duration)
                    ss.playing = false;
            }
        }

        // FK
        ss.skeleton.ForwardKinematics();

        // Procedural layers
        if (m_cfg.enableProceduralLayers) {
            for (auto& layer : ss.procLayers)
                layer->Update(ss.skeleton, dt);
        }
    }
}

// ── Skeleton management ────────────────────────────────────────────────────────

uint32_t AnimationPipeline::CreateSkeleton(const std::string& name)
{
    SkeletonState ss;
    ss.id   = m_nextSkeletonId++;
    ss.name = name;
    m_skeletons.push_back(std::move(ss));
    return m_skeletons.back().id;
}

AnimationPipeline::SkeletonState* AnimationPipeline::FindSkeleton(uint32_t id)
{
    for (auto& ss : m_skeletons)
        if (ss.id == id) return &ss;
    return nullptr;
}

Skeleton* AnimationPipeline::GetSkeleton(uint32_t id)
{
    auto* ss = FindSkeleton(id);
    return ss ? &ss->skeleton : nullptr;
}

void AnimationPipeline::DestroySkeleton(uint32_t id)
{
    m_skeletons.erase(
        std::remove_if(m_skeletons.begin(), m_skeletons.end(),
                       [id](const SkeletonState& s){ return s.id == id; }),
        m_skeletons.end());
}

// ── Mocap ─────────────────────────────────────────────────────────────────────

bool AnimationPipeline::LoadMocapClip(const std::string& id,
                                       const std::string& csvPath)
{
    MocapClip clip;
    clip.name = id;
    if (!clip.LoadCSV(csvPath)) return false;
    m_clips[id] = std::move(clip);
    return true;
}

bool AnimationPipeline::PlayMocapClip(uint32_t skeletonId,
                                       const std::string& clipId,
                                       float /*blendTime*/)
{
    auto* ss = FindSkeleton(skeletonId);
    if (!ss) return false;
    if (m_clips.find(clipId) == m_clips.end()) return false;
    ss->currentClip = clipId;
    ss->clipTime    = 0.f;
    ss->playing     = true;
    return true;
}

void AnimationPipeline::StopMocap(uint32_t skeletonId)
{
    auto* ss = FindSkeleton(skeletonId);
    if (ss) ss->playing = false;
}

float AnimationPipeline::GetMocapTime(uint32_t skeletonId) const
{
    for (const auto& ss : m_skeletons)
        if (ss.id == skeletonId) return ss.clipTime;
    return 0.f;
}

// ── Retargeting ───────────────────────────────────────────────────────────────

void AnimationPipeline::SetRetargetMap(uint32_t skeletonId,
                                        const RetargetMap& map)
{
    auto* ss = FindSkeleton(skeletonId);
    if (ss) ss->retargeter.SetMap(map);
}

void AnimationPipeline::RetargetFrom(uint32_t sourceId, uint32_t targetId)
{
    auto* src = FindSkeleton(sourceId);
    auto* dst = FindSkeleton(targetId);
    if (src && dst)
        dst->retargeter.Apply(src->skeleton, dst->skeleton);
}

// ── Procedural layers ────────────────────────────────────────────────────────

uint32_t AnimationPipeline::AddProceduralLayer(uint32_t skeletonId,
                                                const ProceduralLayerDesc& desc)
{
    auto* ss = FindSkeleton(skeletonId);
    if (!ss) return 0;
    uint32_t id = m_nextLayerId++;
    auto layer = std::make_unique<ProceduralLayer>(desc);
    ss->procLayers.push_back(std::move(layer));
    return id;
}

void AnimationPipeline::RemoveProceduralLayer(uint32_t skeletonId,
                                               uint32_t /*layerId*/)
{
    auto* ss = FindSkeleton(skeletonId);
    if (ss && !ss->procLayers.empty())
        ss->procLayers.pop_back(); // simple stub
}

void AnimationPipeline::SetLayerEnabled(uint32_t skeletonId,
                                         uint32_t /*layerId*/,
                                         bool v)
{
    auto* ss = FindSkeleton(skeletonId);
    if (ss && !ss->procLayers.empty())
        ss->procLayers.back()->SetEnabled(v);
}

void AnimationPipeline::SetLODLevel(uint32_t skeletonId, int level)
{
    auto* ss = FindSkeleton(skeletonId);
    if (ss) ss->lodLevel = level;
}

// ── Stats ─────────────────────────────────────────────────────────────────────

size_t AnimationPipeline::SkeletonCount() const { return m_skeletons.size(); }

size_t AnimationPipeline::ActiveMocapCount() const
{
    size_t n = 0;
    for (const auto& ss : m_skeletons)
        if (ss.playing) ++n;
    return n;
}

size_t AnimationPipeline::TotalProceduralLayers() const
{
    size_t n = 0;
    for (const auto& ss : m_skeletons)
        n += ss.procLayers.size();
    return n;
}

const AnimationPipelineConfig& AnimationPipeline::GetConfig() const
{
    return m_cfg;
}

} // namespace Engine::Animation
