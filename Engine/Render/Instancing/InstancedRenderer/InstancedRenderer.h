#pragma once
/**
 * @file InstancedRenderer.h
 * @brief CPU-side instanced draw-call batcher with per-instance transform, colour, and LOD.
 *
 * Features:
 *   - RegisterMesh(meshId, lodDistances[]) → slot: associate mesh with LOD thresholds
 *   - AddInstance(meshId, transform4x4, colour, customData) → instanceHandle
 *   - RemoveInstance(handle)
 *   - UpdateTransform(handle, transform4x4)
 *   - UpdateColour(handle, colour)
 *   - CullAndBatch(cameraPos, frustumPlanes[6]) → DrawBatch[]: frustum-culled per-LOD batches
 *   - DrawBatch contains: meshId, lodLevel, instanceData[] for GPU upload
 *   - GetInstanceCount(meshId) → active count
 *   - SetMaxInstances(n): hard cap with overflow rejection
 *   - Clear(meshId) / ClearAll()
 *   - Stats: TotalInstances(), VisibleInstances() (from last cull), DrawCallCount()
 */

#include <cstdint>
#include <vector>

namespace Engine {

struct InstMat4  { float m[16]; };
struct InstColor { float r,g,b,a; };

struct InstanceData {
    InstMat4  transform;
    InstColor colour;
    float     custom[4];   ///< app-defined per-instance payload
};

struct DrawBatch {
    uint32_t              meshId;
    uint32_t              lodLevel;
    std::vector<InstanceData> instances;
};

class InstancedRenderer {
public:
    InstancedRenderer();
    ~InstancedRenderer();

    void Init    (uint32_t maxInstances=65536);
    void Shutdown();

    // Mesh registration
    void RegisterMesh  (uint32_t meshId, std::vector<float> lodDistances);
    void UnregisterMesh(uint32_t meshId);

    // Instance management
    uint32_t AddInstance   (uint32_t meshId, const InstMat4& xform,
                            InstColor colour={1,1,1,1}, const float* custom=nullptr);
    void     RemoveInstance(uint32_t handle);
    void     UpdateTransform(uint32_t handle, const InstMat4& xform);
    void     UpdateColour   (uint32_t handle, InstColor colour);

    // Culling + batching
    // frustumPlanes: 6 planes each as (nx,ny,nz,d) — point inside if dot>=0
    std::vector<DrawBatch> CullAndBatch(float camX, float camY, float camZ,
                                        const float frustumPlanes[24]) const;

    // Query
    uint32_t GetInstanceCount(uint32_t meshId) const;
    uint32_t TotalInstances  ()                const;
    uint32_t VisibleInstances()                const; ///< from last CullAndBatch
    uint32_t DrawCallCount   ()                const;

    void Clear   (uint32_t meshId);
    void ClearAll();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
