#pragma once
/**
 * @file SpatialHashGrid.h
 * @brief 2D/3D spatial hash grid for O(1) average nearest-neighbour and range queries.
 *
 * Features:
 *   - SetCellSize(s): uniform grid cell size
 *   - Insert(objId, x, y, z) / Update(objId, x, y, z) / Remove(objId)
 *   - QueryRadius(cx, cy, cz, radius, outIds[]) → uint32_t: all objects within sphere
 *   - QueryBox(minX,minY,minZ, maxX,maxY,maxZ, outIds[]) → uint32_t: AABB query
 *   - GetNearest(cx, cy, cz, outId) → bool: nearest single object
 *   - GetCount() → uint32_t: total inserted objects
 *   - GetCellCount() → uint32_t: occupied cells
 *   - SetOnInsert(cb) / SetOnRemove(cb)
 *   - Clear()
 *   - Shutdown()
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

class SpatialHashGrid {
public:
    SpatialHashGrid();
    ~SpatialHashGrid();

    void Init    (float cellSize = 1.f);
    void Shutdown();
    void Clear   ();

    void SetCellSize(float s);

    // Object management
    void Insert(uint32_t objId, float x, float y, float z);
    void Update(uint32_t objId, float x, float y, float z);
    void Remove(uint32_t objId);

    // Queries
    uint32_t QueryRadius(float cx, float cy, float cz, float radius,
                         std::vector<uint32_t>& outIds) const;
    uint32_t QueryBox   (float minX, float minY, float minZ,
                         float maxX, float maxY, float maxZ,
                         std::vector<uint32_t>& outIds) const;
    bool     GetNearest (float cx, float cy, float cz, uint32_t& outId) const;

    // Stats
    uint32_t GetCount    () const;
    uint32_t GetCellCount() const;

    // Callbacks
    void SetOnInsert(std::function<void(uint32_t objId)> cb);
    void SetOnRemove(std::function<void(uint32_t objId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
