#pragma once
/**
 * @file EntityQuery.h
 * @brief Cached typed ECS component query: register archetype, iterate results.
 *
 * Features:
 *   - Register a "query" matching entities that have all required component type IDs
 *   - CreateQuery(queryId, requiredTypeIds[]) → bool
 *   - DestroyQuery(queryId)
 *   - Rebuild(world): scan world entities and cache matching entity IDs
 *   - GetResults(queryId, out[]) → uint32_t: entity IDs matching query
 *   - GetResultCount(queryId) → uint32_t
 *   - IsStale(queryId) → bool: dirty since last Rebuild
 *   - MarkDirty(queryId): force refresh on next Rebuild
 *   - ForEach(queryId, fn(entityId)): iterate without copying
 *   - AddRequiredType(queryId, typeId) / RemoveRequiredType(queryId, typeId)
 *   - SetOnRebuild(queryId, cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Core {

class EntityQuery {
public:
    EntityQuery();
    ~EntityQuery();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Query definition
    bool CreateQuery (uint32_t queryId,
                      const std::vector<uint32_t>& requiredTypeIds);
    void DestroyQuery(uint32_t queryId);

    // Type management
    void AddRequiredType   (uint32_t queryId, uint32_t typeId);
    void RemoveRequiredType(uint32_t queryId, uint32_t typeId);

    // Refresh
    // worldEntities: all entity IDs; hasComponent(entity,typeId) → bool
    void Rebuild(const std::vector<uint32_t>& worldEntities,
                 std::function<bool(uint32_t entity, uint32_t typeId)> hasComponent);

    // Query state
    void MarkDirty(uint32_t queryId);
    bool IsStale  (uint32_t queryId) const;

    // Results
    uint32_t GetResults    (uint32_t queryId, std::vector<uint32_t>& out) const;
    uint32_t GetResultCount(uint32_t queryId) const;
    void     ForEach       (uint32_t queryId,
                            std::function<void(uint32_t entityId)> fn) const;

    // Callback
    void SetOnRebuild(uint32_t queryId,
                      std::function<void(uint32_t count)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
