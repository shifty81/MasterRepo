#include "InstancedRenderer.h"
#include <algorithm>

namespace atlas::render {

void InstancedRenderer::Init(uint32_t maxInstancesPerBatch) {
    m_maxInstancesPerBatch = maxInstancesPerBatch;
    m_batches.clear();
    m_nextBatchId = 1;
    m_drawCalls = 0;
    m_initialized = true;
}

void InstancedRenderer::Shutdown() {
    m_batches.clear();
    m_initialized = false;
    m_drawCalls = 0;
}

uint32_t InstancedRenderer::CreateBatch(uint32_t meshId, uint32_t materialId) {
    if (!m_initialized) return 0;

    InstanceBatch batch;
    batch.meshId = meshId;
    batch.materialId = materialId;
    batch.id = m_nextBatchId++;
    uint32_t id = batch.id;
    m_batches[id] = std::move(batch);
    return id;
}

void InstancedRenderer::RemoveBatch(uint32_t batchId) {
    m_batches.erase(batchId);
}

uint32_t InstancedRenderer::AddInstance(uint32_t batchId, const InstanceData& data) {
    auto it = m_batches.find(batchId);
    if (it == m_batches.end()) return 0;

    auto& instances = it->second.instances;
    if (static_cast<uint32_t>(instances.size()) >= m_maxInstancesPerBatch) {
        return 0;
    }

    instances.push_back(data);
    return static_cast<uint32_t>(instances.size() - 1);
}

void InstancedRenderer::RemoveInstance(uint32_t batchId, uint32_t instanceIndex) {
    auto it = m_batches.find(batchId);
    if (it == m_batches.end()) return;

    auto& instances = it->second.instances;
    if (instanceIndex < static_cast<uint32_t>(instances.size())) {
        instances.erase(instances.begin() + instanceIndex);
    }
}

void InstancedRenderer::UpdateInstance(uint32_t batchId, uint32_t instanceIndex,
                                       const InstanceData& data) {
    auto it = m_batches.find(batchId);
    if (it == m_batches.end()) return;

    auto& instances = it->second.instances;
    if (instanceIndex < static_cast<uint32_t>(instances.size())) {
        instances[instanceIndex] = data;
    }
}

void InstancedRenderer::ClearBatch(uint32_t batchId) {
    auto it = m_batches.find(batchId);
    if (it != m_batches.end()) {
        it->second.instances.clear();
    }
}

const InstanceBatch* InstancedRenderer::GetBatch(uint32_t batchId) const {
    auto it = m_batches.find(batchId);
    if (it != m_batches.end()) {
        return &it->second;
    }
    return nullptr;
}

uint32_t InstancedRenderer::BatchCount() const {
    return static_cast<uint32_t>(m_batches.size());
}

uint32_t InstancedRenderer::TotalInstanceCount() const {
    uint32_t total = 0;
    for (const auto& pair : m_batches) {
        total += static_cast<uint32_t>(pair.second.instances.size());
    }
    return total;
}

void InstancedRenderer::SubmitAll() {
    if (!m_initialized) return;

    m_drawCalls = 0;
    for (const auto& pair : m_batches) {
        if (!pair.second.instances.empty()) {
            ++m_drawCalls;
        }
    }
}

} // namespace atlas::render
