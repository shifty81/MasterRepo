#pragma once
/**
 * @file CaveGenerator.h
 * @brief Cellular-automata cave system with vault placement and connectivity.
 *
 * CaveGenerator produces a 2-D tile grid representing a cave system:
 *   - Tile: Floor or Wall (boolean grid stored as uint8_t grid)
 *   - Algorithm:
 *     1. Fill map randomly with walls at configurable density.
 *     2. Run N cellular-automata passes: birth rule (wall if ≥ birthNeighbours
 *        wall neighbours) and survive rule.
 *     3. Flood-fill to remove isolated islands smaller than minIslandSize.
 *     4. Carve passages to connect disconnected regions.
 *     5. Place vaults (rectangular cleared rooms) at open areas.
 *     6. Validate connectivity: all floor tiles reachable.
 *   - CaveMap: width × height grid + vault list + entrance/exit positions.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace PCG {

// ── Tile types ─────────────────────────────────────────────────────────────
enum class CaveTile : uint8_t { Floor = 0, Wall = 1 };

// ── Vault ──────────────────────────────────────────────────────────────────
struct CaveVault {
    uint32_t x{0}, y{0};
    uint32_t w{0}, h{0};
    std::string type;     ///< e.g. "treasure", "boss", "spawn"
};

// ── Cave map ───────────────────────────────────────────────────────────────
struct CaveMap {
    uint32_t width{0};
    uint32_t height{0};
    std::vector<CaveTile> tiles;  ///< Row-major, tile[y*width+x]
    std::vector<CaveVault> vaults;
    uint32_t entranceX{0}, entranceY{0};
    uint32_t exitX{0},     exitY{0};

    CaveTile Get(uint32_t x, uint32_t y) const;
    void     Set(uint32_t x, uint32_t y, CaveTile t);
    bool     IsFloor(uint32_t x, uint32_t y) const;
    bool     IsWall(uint32_t x, uint32_t y)  const;
    bool     InBounds(int32_t x, int32_t y) const;

    /// BFS from (startX,startY); returns number of floor tiles reachable.
    uint32_t FloodCount(uint32_t startX, uint32_t startY) const;
    /// True if all floor tiles are reachable from the entrance.
    bool     IsFullyConnected() const;
    uint32_t FloorTileCount() const;
};

// ── Config ─────────────────────────────────────────────────────────────────
struct CaveConfig {
    uint32_t width{80};
    uint32_t height{50};
    float    fillDensity{0.48f};    ///< Initial wall probability
    uint32_t automataSteps{5};      ///< CA iterations
    uint32_t birthNeighbours{5};    ///< # wall neighbours to birth a wall
    uint32_t surviveNeighbours{4};  ///< # wall neighbours to survive as wall
    uint32_t minIslandSize{30};     ///< Remove isolated regions smaller than this
    uint32_t vaultCount{3};
    uint32_t vaultMinW{4}, vaultMinH{4};
    uint32_t vaultMaxW{10}, vaultMaxH{8};
    uint64_t seed{0};
};

// ── Generator ─────────────────────────────────────────────────────────────
class CaveGenerator {
public:
    CaveGenerator();
    ~CaveGenerator();

    void SetConfig(const CaveConfig& cfg);
    const CaveConfig& Config() const;

    CaveMap Generate();

    using ProgressCb = std::function<void(float, const std::string&)>;
    void OnProgress(ProgressCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
