#pragma once
/**
 * @file ProceduralDungeon.h
 * @brief BSP dungeon generator: rooms, corridors, door/wall tiles, spawn points.
 *
 * Features:
 *   - DungeonParams: width, height, minRoomSize, maxRoomSize, maxDepth, seed
 *   - Generate(params) → DungeonMap
 *   - DungeonMap: 2D tile grid (uint8 enum: Empty/Wall/Floor/Door/Corridor)
 *   - GetRooms() → vector of RoomRect {x,y,w,h}
 *   - GetSpawnPoints(SpawnType) → vector of {x,y} (enemy/item/player_start)
 *   - GetConnectivity() → room adjacency list
 *   - IsWalkable(x,y) → bool
 *   - AddCustomRoom(rect)
 *   - GetExits() → list of edge doors
 *   - Debug: GetSplitTree() root node for BSP visualisation
 */

#include <cstdint>
#include <vector>

namespace PCG {

enum class DungeonTile : uint8_t { Empty=0, Wall, Floor, Door, Corridor };
enum class SpawnType   : uint8_t { Enemy=0, Item, PlayerStart };

struct RoomRect { int32_t x{0},y{0},w{0},h{0}; };
struct SpawnPoint { int32_t x{0},y{0}; SpawnType type{SpawnType::Enemy}; };

struct DungeonParams {
    uint32_t width      {64};
    uint32_t height     {64};
    uint32_t minRoomSize{5};
    uint32_t maxRoomSize{15};
    uint32_t maxDepth   {5};
    uint32_t seed       {0};
    float    splitRatio {0.45f}; ///< BSP split ratio variance
    bool     addCorridors{true};
};

struct DungeonMap {
    uint32_t                     width{0}, height{0};
    std::vector<DungeonTile>     tiles;
    std::vector<RoomRect>        rooms;
    std::vector<SpawnPoint>      spawns;

    DungeonTile Get(int32_t x, int32_t y) const {
        if(x<0||y<0||(uint32_t)x>=width||(uint32_t)y>=height) return DungeonTile::Wall;
        return tiles[(uint32_t)y*width+(uint32_t)x];
    }
    bool IsWalkable(int32_t x, int32_t y) const {
        auto t=Get(x,y); return t==DungeonTile::Floor||t==DungeonTile::Corridor||t==DungeonTile::Door;
    }
};

class ProceduralDungeon {
public:
    ProceduralDungeon();
    ~ProceduralDungeon();

    void Init    ();
    void Shutdown();

    DungeonMap Generate(const DungeonParams& params);

    // Query on last generated map
    const DungeonMap&               GetMap          () const;
    const std::vector<RoomRect>&    GetRooms        () const;
    std::vector<SpawnPoint>         GetSpawnPoints  (SpawnType t) const;
    std::vector<std::pair<int,int>> GetConnectivity () const; ///< room index pairs

    void AddCustomRoom(const RoomRect& r);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
