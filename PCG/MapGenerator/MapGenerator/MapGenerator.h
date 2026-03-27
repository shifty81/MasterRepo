#pragma once
/**
 * @file MapGenerator.h
 * @brief Procedural 2D/3D map layout: BSP rooms/corridors, cellular automata caves, flood fill.
 *
 * Features:
 *   - Generation strategies: BSP (Binary Space Partitioning), CellularAutomata, Random Walk
 *   - 2D tile grid output: Wall, Floor, Door, Void (extendable)
 *   - BSP: min/max room size, corridor width, depth limit
 *   - Cellular automata: birth/survive rules, iterations, seed density
 *   - Random walk: step count, direction bias
 *   - Post-process: flood-fill connectivity, remove isolated regions
 *   - Room list with centre, bounds, connected neighbours
 *   - Door placement: between adjacent rooms
 *   - Seed-reproducible: uint64_t seed
 *   - Feature placement callbacks: OnRoomCreated, OnDoorPlaced
 *   - JSON export / import
 *
 * Typical usage:
 * @code
 *   MapGenerator mg;
 *   MapGenParams p;
 *   p.strategy = GenStrategy::BSP;
 *   p.width=64; p.height=64;
 *   p.seed=42; p.minRoomSize=5; p.maxRoomSize=15;
 *   auto result = mg.Generate(p);
 *   for (auto& room : result.rooms) UseRoom(room);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PCG {

enum class TileType  : uint8_t { Void=0, Wall, Floor, Door, Water, Stairs };
enum class GenStrategy: uint8_t { BSP=0, CellularAutomata, RandomWalk };

struct MapGenParams {
    GenStrategy strategy     {GenStrategy::BSP};
    uint32_t    width        {64};
    uint32_t    height       {64};
    uint64_t    seed         {0};

    // BSP
    uint32_t    minRoomSize  {5};
    uint32_t    maxRoomSize  {15};
    uint32_t    corridorWidth{1};
    uint32_t    bspDepth     {5};

    // CellularAutomata
    float       seedDensity  {0.45f};
    uint32_t    caIterations {5};
    uint32_t    birthLimit   {4};
    uint32_t    deathLimit   {3};

    // Random Walk
    uint32_t    walkSteps    {500};
    float       turnBias     {0.3f};
};

struct Room {
    uint32_t id{0};
    int32_t  x{0},  y{0};       ///< top-left tile
    int32_t  w{0},  h{0};       ///< width/height in tiles
    int32_t  cx{0}, cy{0};      ///< centre
    std::vector<uint32_t> neighbours;  ///< connected room ids
};

struct MapResult {
    uint32_t             width{0}, height{0};
    std::vector<TileType> tiles;                ///< row-major
    std::vector<Room>    rooms;
    std::vector<std::pair<int32_t,int32_t>> doors;

    TileType At(int32_t x, int32_t y) const {
        if (x<0||y<0||(uint32_t)x>=width||(uint32_t)y>=height) return TileType::Void;
        return tiles[(uint32_t)y*width+(uint32_t)x];
    }
    void Set(int32_t x, int32_t y, TileType t) {
        if (x<0||y<0||(uint32_t)x>=width||(uint32_t)y>=height) return;
        tiles[(uint32_t)y*width+(uint32_t)x] = t;
    }
};

class MapGenerator {
public:
    MapGenerator();
    ~MapGenerator();

    MapResult Generate(const MapGenParams& params);

    // Feature callbacks
    void SetOnRoomCreated(std::function<void(const Room&)> cb);
    void SetOnDoorPlaced (std::function<void(int32_t x, int32_t y)> cb);

    // Export/import
    static bool SaveJSON(const MapResult& map, const std::string& path);
    static bool LoadJSON(const std::string& path, MapResult& outMap);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
