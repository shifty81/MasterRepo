#pragma once
/**
 * @file DungeonGenerator.h
 * @brief BSP-based dungeon / level generator.
 *
 * Partitions a rectangular canvas using Binary Space Partitioning, then
 * carves rooms in each leaf partition and connects them with corridors.
 * Produces a flat grid of DungeonTile values and a list of Room/Corridor
 * structs for post-processing (placing props, enemies, loot, etc.).
 */

#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace PCG {

enum class DungeonTile : uint8_t {
    Wall    = 0,
    Floor   = 1,
    Door    = 2,
    Stairs  = 3,
    Corridor = 4,
};

struct DungeonRect {
    int x{0}, y{0}, w{0}, h{0};
    int CenterX() const { return x + w / 2; }
    int CenterY() const { return y + h / 2; }
};

struct DungeonRoom {
    uint32_t    id{0};
    DungeonRect bounds;
    std::string tag;         ///< Semantic tag: "start", "boss", "treasure", etc.
};

struct DungeonCorridor {
    int x0{0}, y0{0}, x1{0}, y1{0};
};

struct DungeonMap {
    int                           width{0};
    int                           height{0};
    std::vector<DungeonTile>      tiles;    ///< Row-major: tiles[y*width + x]
    std::vector<DungeonRoom>      rooms;
    std::vector<DungeonCorridor>  corridors;

    DungeonTile Get(int x, int y) const;
    void        Set(int x, int y, DungeonTile t);
};

struct DungeonConfig {
    int      width{80};          ///< Map width in tiles
    int      height{50};         ///< Map height in tiles
    int      minRoomSize{5};     ///< Minimum room dimension
    int      maxRoomSize{15};    ///< Maximum room dimension
    int      maxDepth{5};        ///< BSP recursion depth
    float    splitRatio{0.45f};  ///< How close to centre a split can be [0.3,0.5]
    uint64_t seed{0};            ///< 0 = use time
};

using DungeonRoomCallback = std::function<void(const DungeonRoom&)>;

/// DungeonGenerator — BSP-based dungeon generator.
class DungeonGenerator {
public:
    DungeonGenerator();
    ~DungeonGenerator();

    void SetConfig(const DungeonConfig& cfg);
    const DungeonConfig& GetConfig() const;

    /// Generate a complete DungeonMap.
    DungeonMap Generate();

    /// Generate and call cb for each room as it is created (streaming).
    DungeonMap GenerateWithCallback(DungeonRoomCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
