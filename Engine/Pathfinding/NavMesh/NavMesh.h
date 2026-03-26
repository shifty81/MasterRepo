#pragma once
/**
 * @file NavMesh.h
 * @brief Navigation mesh for AI pathfinding using A* over a polygon graph.
 *
 * The NavMesh is built from a triangle soup (world geometry) via a
 * voxelisation + region-merge step.  Agents query paths as polyline
 * waypoint lists.  Supports:
 *   - Dynamic obstacle registration (mark navmesh polygons as blocked)
 *   - Area cost modifiers (e.g. water = 3× cost)
 *   - Path smoothing (string-pull / Bezier)
 *   - Multiple agent radii (different clearances)
 *
 * Typical usage:
 * @code
 *   NavMesh nav;
 *   NavMeshBuildConfig cfg;
 *   cfg.cellSize    = 0.3f;
 *   cfg.agentRadius = 0.4f;
 *   cfg.agentHeight = 1.8f;
 *   nav.Init(cfg);
 *   nav.BuildFromTriangles(verts, vertCount, indices, indexCount);
 *   NavPath path = nav.FindPath({0,0,0}, {50,0,30});
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Build configuration ───────────────────────────────────────────────────────

struct NavMeshBuildConfig {
    float    cellSize{0.3f};       ///< voxel width/depth
    float    cellHeight{0.2f};     ///< voxel height
    float    agentRadius{0.4f};
    float    agentHeight{1.8f};
    float    agentMaxSlope{45.f};  ///< degrees
    float    agentMaxClimb{0.5f};  ///< step height
    float    minRegionArea{8.f};   ///< small island cull threshold
};

// ── A polygon (convex) in the nav mesh ───────────────────────────────────────

struct NavPoly {
    uint32_t             id{0};
    std::vector<float>   verts;  ///< flat list: x0,y0,z0, x1,y1,z1, …
    std::vector<uint32_t> neighbours;
    float                areaCost{1.f};
    bool                 blocked{false};
};

// ── Path result ───────────────────────────────────────────────────────────────

struct NavPath {
    bool                succeeded{false};
    std::vector<float>  waypoints;  ///< flat list: x0,y0,z0, x1,y1,z1, …
    float               totalLength{0.f};
    uint32_t            nodeCount{0};
    std::string         errorMessage;
};

// ── NavMesh ───────────────────────────────────────────────────────────────────

class NavMesh {
public:
    NavMesh();
    ~NavMesh();

    void Init(const NavMeshBuildConfig& config = {});
    void Shutdown();

    // ── Build ─────────────────────────────────────────────────────────────────

    bool BuildFromTriangles(const float* verts, uint32_t vertCount,
                            const uint32_t* indices, uint32_t indexCount);

    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;

    bool IsBuilt() const;
    uint32_t PolyCount() const;

    // ── Pathfinding ───────────────────────────────────────────────────────────

    NavPath FindPath(const float start[3], const float end[3]) const;

    /// Smooth a raw polygon path using string-pull algorithm.
    NavPath SmoothPath(const NavPath& rawPath) const;

    // ── Dynamic obstacles ─────────────────────────────────────────────────────

    void BlockArea(const float centre[3], float radius);
    void UnblockArea(const float centre[3], float radius);
    void SetPolyBlocked(uint32_t polyId, bool blocked);

    // ── Area costs ────────────────────────────────────────────────────────────

    void SetPolyAreaCost(uint32_t polyId, float cost);

    // ── Queries ───────────────────────────────────────────────────────────────

    bool  FindNearestPoint(const float pos[3], float result[3]) const;
    bool  IsPointOnMesh(const float pos[3]) const;

    NavMeshBuildConfig GetConfig() const;

    // ── Debug ─────────────────────────────────────────────────────────────────

    void OnPathFound(std::function<void(const NavPath&)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
