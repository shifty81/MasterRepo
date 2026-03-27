#pragma once
/**
 * @file MorphTargetSystem.h
 * @brief Blend-shape / morph-target animation: per-vertex delta accumulation and weight control.
 *
 * Features:
 *   - CreateMesh(id, baseVertices[], count): register a mesh with base positions
 *   - AddTarget(meshId, name, deltaPositions[], deltaNormals[]) → targetId
 *   - SetWeight(meshId, targetId, weight): blend weight [0,1]
 *   - GetWeight(meshId, targetId) → float
 *   - Evaluate(meshId, outPositions[], outNormals[]): compute final blended mesh
 *   - SetAnimCurve(meshId, targetId, times[], weights[], count): keyframe animation
 *   - TickAnimation(meshId, dt): advance keyframe time
 *   - ResetWeights(meshId): set all weights to zero
 *   - GetTargetCount(meshId) → uint32_t
 *   - GetVertexCount(meshId) → uint32_t
 *   - GetTargetId(meshId, name) → uint32_t
 *   - RemoveTarget(meshId, targetId)
 *   - DestroyMesh(id)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

struct MTVec3 { float x,y,z; };

class MorphTargetSystem {
public:
    MorphTargetSystem();
    ~MorphTargetSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Mesh management
    void CreateMesh (uint32_t meshId, const MTVec3* basePos, const MTVec3* baseNorm,
                     uint32_t vertexCount);
    void DestroyMesh(uint32_t meshId);

    // Target management
    uint32_t AddTarget   (uint32_t meshId, const std::string& name,
                          const MTVec3* deltaPos, const MTVec3* deltaNorm);
    void     RemoveTarget(uint32_t meshId, uint32_t targetId);
    uint32_t GetTargetId (uint32_t meshId, const std::string& name) const;

    // Weight control
    void  SetWeight    (uint32_t meshId, uint32_t targetId, float weight);
    float GetWeight    (uint32_t meshId, uint32_t targetId) const;
    void  ResetWeights (uint32_t meshId);

    // Evaluation
    void Evaluate(uint32_t meshId, MTVec3* outPos, MTVec3* outNorm) const;

    // Keyframe animation
    void SetAnimCurve   (uint32_t meshId, uint32_t targetId,
                         const float* times, const float* weights, uint32_t count);
    void TickAnimation  (uint32_t meshId, float dt);
    void SetAnimLooping (uint32_t meshId, uint32_t targetId, bool loop);

    // Queries
    uint32_t GetTargetCount(uint32_t meshId) const;
    uint32_t GetVertexCount(uint32_t meshId) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
