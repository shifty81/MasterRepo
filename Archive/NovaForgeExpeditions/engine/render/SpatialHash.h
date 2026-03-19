#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace atlas::render {

/// An entity with a position and bounding radius for spatial queries.
struct SpatialEntity {
    uint32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float radius = 0.0f;
};

/// Spatial hash grid for fast proximity queries.
/// Maps entity positions to cells using a hash function on quantised coordinates,
/// enabling O(1) average-case insertion/removal and efficient range queries.
class SpatialHash {
public:
    explicit SpatialHash(float cellSize = 1.0f);

    void Insert(const SpatialEntity& entity);
    void Remove(uint32_t entityId);
    void Update(uint32_t entityId, float x, float y, float z);

    /// Returns ids of entities within the given radius of (x, y, z).
    std::vector<uint32_t> QueryRadius(float x, float y, float z, float radius) const;

    /// Returns ids of entities overlapping an axis-aligned bounding box.
    std::vector<uint32_t> QueryAABB(float minX, float minY, float minZ,
                                    float maxX, float maxY, float maxZ) const;

    /// Returns up to `count` nearest entity ids to the given point.
    std::vector<uint32_t> GetNearestNeighbors(float x, float y, float z,
                                              uint32_t count) const;

    void Clear();
    uint32_t EntityCount() const;
    float CellSize() const { return m_cellSize; }
    uint32_t OccupiedCellCount() const;

private:
    struct CellCoord {
        int32_t cx, cy, cz;
        bool operator==(const CellCoord& o) const {
            return cx == o.cx && cy == o.cy && cz == o.cz;
        }
    };

    struct CellCoordHash {
        std::size_t operator()(const CellCoord& c) const {
            // Large primes for spatial hashing
            std::size_t h = static_cast<std::size_t>(c.cx) * 73856093u
                          ^ static_cast<std::size_t>(c.cy) * 19349663u
                          ^ static_cast<std::size_t>(c.cz) * 83492791u;
            return h;
        }
    };

    CellCoord ToCellCoord(float x, float y, float z) const;
    void InsertIntoGrid(const SpatialEntity& entity);
    void RemoveFromGrid(uint32_t entityId);

    float m_cellSize;
    float m_invCellSize;

    // Map from cell coordinate to list of entity ids in that cell
    std::unordered_map<CellCoord, std::vector<uint32_t>, CellCoordHash> m_grid;
    // Map from entity id to entity data
    std::unordered_map<uint32_t, SpatialEntity> m_entities;
};

} // namespace atlas::render
