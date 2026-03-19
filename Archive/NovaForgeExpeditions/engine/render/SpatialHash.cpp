#include "SpatialHash.h"
#include <algorithm>
#include <cmath>

namespace atlas::render {

SpatialHash::SpatialHash(float cellSize)
    : m_cellSize(cellSize)
    , m_invCellSize(1.0f / cellSize) {
}

SpatialHash::CellCoord SpatialHash::ToCellCoord(float x, float y, float z) const {
    return {
        static_cast<int32_t>(std::floor(x * m_invCellSize)),
        static_cast<int32_t>(std::floor(y * m_invCellSize)),
        static_cast<int32_t>(std::floor(z * m_invCellSize))
    };
}

void SpatialHash::InsertIntoGrid(const SpatialEntity& entity) {
    // Insert entity into all cells it overlaps (accounting for radius)
    float r = entity.radius;
    CellCoord cMin = ToCellCoord(entity.x - r, entity.y - r, entity.z - r);
    CellCoord cMax = ToCellCoord(entity.x + r, entity.y + r, entity.z + r);

    for (int32_t cx = cMin.cx; cx <= cMax.cx; ++cx) {
        for (int32_t cy = cMin.cy; cy <= cMax.cy; ++cy) {
            for (int32_t cz = cMin.cz; cz <= cMax.cz; ++cz) {
                m_grid[{cx, cy, cz}].push_back(entity.id);
            }
        }
    }
}

void SpatialHash::RemoveFromGrid(uint32_t entityId) {
    auto entIt = m_entities.find(entityId);
    if (entIt == m_entities.end()) return;

    const auto& e = entIt->second;
    float r = e.radius;
    CellCoord cMin = ToCellCoord(e.x - r, e.y - r, e.z - r);
    CellCoord cMax = ToCellCoord(e.x + r, e.y + r, e.z + r);

    for (int32_t cx = cMin.cx; cx <= cMax.cx; ++cx) {
        for (int32_t cy = cMin.cy; cy <= cMax.cy; ++cy) {
            for (int32_t cz = cMin.cz; cz <= cMax.cz; ++cz) {
                CellCoord cc{cx, cy, cz};
                auto gridIt = m_grid.find(cc);
                if (gridIt != m_grid.end()) {
                    auto& vec = gridIt->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), entityId), vec.end());
                    if (vec.empty()) {
                        m_grid.erase(gridIt);
                    }
                }
            }
        }
    }
}

void SpatialHash::Insert(const SpatialEntity& entity) {
    // Remove existing entity with same id if present
    if (m_entities.count(entity.id)) {
        Remove(entity.id);
    }
    m_entities[entity.id] = entity;
    InsertIntoGrid(entity);
}

void SpatialHash::Remove(uint32_t entityId) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;

    const auto& e = it->second;
    float r = e.radius;
    CellCoord cMin = ToCellCoord(e.x - r, e.y - r, e.z - r);
    CellCoord cMax = ToCellCoord(e.x + r, e.y + r, e.z + r);

    for (int32_t cx = cMin.cx; cx <= cMax.cx; ++cx) {
        for (int32_t cy = cMin.cy; cy <= cMax.cy; ++cy) {
            for (int32_t cz = cMin.cz; cz <= cMax.cz; ++cz) {
                CellCoord cc{cx, cy, cz};
                auto gridIt = m_grid.find(cc);
                if (gridIt != m_grid.end()) {
                    auto& vec = gridIt->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), entityId), vec.end());
                    if (vec.empty()) {
                        m_grid.erase(gridIt);
                    }
                }
            }
        }
    }

    m_entities.erase(it);
}

void SpatialHash::Update(uint32_t entityId, float x, float y, float z) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;

    SpatialEntity updated = it->second;

    // Remove from old cells
    Remove(entityId);

    // Re-insert at new position
    updated.x = x;
    updated.y = y;
    updated.z = z;
    Insert(updated);
}

std::vector<uint32_t> SpatialHash::QueryRadius(float x, float y, float z,
                                                float radius) const {
    std::vector<uint32_t> result;
    float totalRadius = radius;
    CellCoord cMin = ToCellCoord(x - totalRadius, y - totalRadius, z - totalRadius);
    CellCoord cMax = ToCellCoord(x + totalRadius, y + totalRadius, z + totalRadius);

    // Collect unique entity ids from candidate cells
    std::vector<uint32_t> candidates;
    for (int32_t cx = cMin.cx; cx <= cMax.cx; ++cx) {
        for (int32_t cy = cMin.cy; cy <= cMax.cy; ++cy) {
            for (int32_t cz = cMin.cz; cz <= cMax.cz; ++cz) {
                auto gridIt = m_grid.find({cx, cy, cz});
                if (gridIt != m_grid.end()) {
                    for (uint32_t id : gridIt->second) {
                        candidates.push_back(id);
                    }
                }
            }
        }
    }

    // Deduplicate
    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    // Distance check
    for (uint32_t id : candidates) {
        auto it = m_entities.find(id);
        if (it == m_entities.end()) continue;

        const auto& e = it->second;
        float dx = e.x - x;
        float dy = e.y - y;
        float dz = e.z - z;
        float distSq = dx * dx + dy * dy + dz * dz;
        float combinedRadius = radius + e.radius;

        if (distSq <= combinedRadius * combinedRadius) {
            result.push_back(id);
        }
    }

    return result;
}

std::vector<uint32_t> SpatialHash::QueryAABB(float minX, float minY, float minZ,
                                              float maxX, float maxY, float maxZ) const {
    std::vector<uint32_t> result;
    CellCoord cMin = ToCellCoord(minX, minY, minZ);
    CellCoord cMax = ToCellCoord(maxX, maxY, maxZ);

    std::vector<uint32_t> candidates;
    for (int32_t cx = cMin.cx; cx <= cMax.cx; ++cx) {
        for (int32_t cy = cMin.cy; cy <= cMax.cy; ++cy) {
            for (int32_t cz = cMin.cz; cz <= cMax.cz; ++cz) {
                auto gridIt = m_grid.find({cx, cy, cz});
                if (gridIt != m_grid.end()) {
                    for (uint32_t id : gridIt->second) {
                        candidates.push_back(id);
                    }
                }
            }
        }
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

    for (uint32_t id : candidates) {
        auto it = m_entities.find(id);
        if (it == m_entities.end()) continue;

        const auto& e = it->second;
        // Entity's AABB overlaps query AABB
        if (e.x + e.radius >= minX && e.x - e.radius <= maxX &&
            e.y + e.radius >= minY && e.y - e.radius <= maxY &&
            e.z + e.radius >= minZ && e.z - e.radius <= maxZ) {
            result.push_back(id);
        }
    }

    return result;
}

std::vector<uint32_t> SpatialHash::GetNearestNeighbors(float x, float y, float z,
                                                        uint32_t count) const {
    if (count == 0 || m_entities.empty()) return {};

    // Compute distances for all entities
    struct DistEntry {
        uint32_t id;
        float distSq;
    };

    std::vector<DistEntry> entries;
    entries.reserve(m_entities.size());

    for (const auto& pair : m_entities) {
        const auto& e = pair.second;
        float dx = e.x - x;
        float dy = e.y - y;
        float dz = e.z - z;
        entries.push_back({e.id, dx * dx + dy * dy + dz * dz});
    }

    uint32_t n = std::min(count, static_cast<uint32_t>(entries.size()));
    std::partial_sort(entries.begin(), entries.begin() + n, entries.end(),
        [](const DistEntry& a, const DistEntry& b) { return a.distSq < b.distSq; });

    std::vector<uint32_t> result;
    result.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        result.push_back(entries[i].id);
    }
    return result;
}

void SpatialHash::Clear() {
    m_grid.clear();
    m_entities.clear();
}

uint32_t SpatialHash::EntityCount() const {
    return static_cast<uint32_t>(m_entities.size());
}

uint32_t SpatialHash::OccupiedCellCount() const {
    return static_cast<uint32_t>(m_grid.size());
}

} // namespace atlas::render
