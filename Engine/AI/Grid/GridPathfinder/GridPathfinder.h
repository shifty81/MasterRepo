#pragma once
/**
 * @file GridPathfinder.h
 * @brief 2D A* on bool grid: diagonal, cost map, path smoothing, incremental re-plan.
 *
 * Features:
 *   - Grid: width x height, passable bool, per-cell cost (float)
 *   - SetPassable(x, y, passable) / SetCost(x, y, cost)
 *   - FindPath(start, goal) → PathResult: node list (x,y), total cost, found bool
 *   - Diagonal movement option (8-connected vs 4-connected)
 *   - Cost map: obstacle=∞, open=1, difficult=2..N
 *   - Path smoothing: Bresenham LOS to skip intermediate nodes
 *   - Incremental: ReplanFrom(pos, goal) — partial reuse of previous closed set
 *   - WorldToGrid(wx, wy) / GridToWorld(gx, gy) with tile-size
 *   - Multiple pathfinders on different grids
 *   - Async: PathAsync(start, goal, callback) — runs on caller's next Tick
 *
 * Typical usage:
 * @code
 *   GridPathfinder pf;
 *   pf.Init(64, 64, 1.f); // 64x64 grid, 1m tiles
 *   pf.SetPassable(10, 5, false); // wall
 *   auto res = pf.FindPath({0,0}, {63,63});
 *   for (auto& n : res.path) DoMove(n.x, n.y);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <vector>

namespace Engine {

struct GridNode { int32_t x{0}, y{0}; };

struct PathResult {
    std::vector<GridNode> path;
    float totalCost{0.f};
    bool  found{false};
};

class GridPathfinder {
public:
    GridPathfinder();
    ~GridPathfinder();

    void Init    (uint32_t width, uint32_t height, float tileSize=1.f);
    void Shutdown();
    void Tick    (float dt); ///< drives async requests

    // Grid modification
    void SetPassable(int32_t x, int32_t y, bool passable);
    void SetCost    (int32_t x, int32_t y, float cost);
    bool IsPassable (int32_t x, int32_t y) const;
    float GetCost   (int32_t x, int32_t y) const;
    void FillRect   (int32_t x, int32_t y, int32_t w, int32_t h, bool passable);
    void Reset      ();  ///< all passable, cost=1

    // Options
    void SetDiagonal(bool enabled);
    void SetSmoothing(bool enabled);

    // Synchronous
    PathResult FindPath   (GridNode start, GridNode goal) const;
    PathResult ReplanFrom (GridNode current, GridNode goal);

    // Async
    using PathCallback = std::function<void(const PathResult&)>;
    void PathAsync(GridNode start, GridNode goal, PathCallback cb);

    // World ↔ Grid
    GridNode    WorldToGrid(float wx, float wy) const;
    void        GridToWorld(int32_t gx, int32_t gy, float& wx, float& wy) const;

    uint32_t Width () const;
    uint32_t Height() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
