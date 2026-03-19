#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace atlas::render {

/// Per-instance data for instanced rendering.
struct InstanceData {
    float transform[16] = {
        1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1
    };
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    uint32_t entityId = 0;
};

/// A batch of instances sharing the same mesh and material.
struct InstanceBatch {
    uint32_t meshId = 0;
    uint32_t materialId = 0;
    std::vector<InstanceData> instances;
    uint32_t id = 0;
};

/// Renderer for large numbers of instances sharing mesh/material pairs.
/// Groups instances into batches and submits them as instanced draw calls.
class InstancedRenderer {
public:
    void Init(uint32_t maxInstancesPerBatch = 10000);
    void Shutdown();

    /// Create a new batch for the given mesh and material. Returns batch id.
    uint32_t CreateBatch(uint32_t meshId, uint32_t materialId);

    /// Remove a batch by id.
    void RemoveBatch(uint32_t batchId);

    /// Add an instance to a batch. Returns the instance index within the batch.
    uint32_t AddInstance(uint32_t batchId, const InstanceData& data);

    void RemoveInstance(uint32_t batchId, uint32_t instanceIndex);
    void UpdateInstance(uint32_t batchId, uint32_t instanceIndex, const InstanceData& data);
    void ClearBatch(uint32_t batchId);

    const InstanceBatch* GetBatch(uint32_t batchId) const;

    uint32_t BatchCount() const;
    uint32_t TotalInstanceCount() const;

    /// Submit all batches for rendering (increments draw call counter).
    void SubmitAll();

    uint32_t DrawCallCount() const { return m_drawCalls; }
    uint32_t MaxInstancesPerBatch() const { return m_maxInstancesPerBatch; }
    bool IsInitialized() const { return m_initialized; }

private:
    std::unordered_map<uint32_t, InstanceBatch> m_batches;
    uint32_t m_maxInstancesPerBatch = 10000;
    uint32_t m_nextBatchId = 1;
    uint32_t m_drawCalls = 0;
    bool m_initialized = false;
};

} // namespace atlas::render
