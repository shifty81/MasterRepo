#pragma once
/**
 * @file StructureGenerator.h
 * @brief Procedural building and structure generation using modular room/corridor graphs.
 *
 * StructureGenerator creates multi-floor buildings and freestanding structures
 * from a set of configurable rules:
 *
 *   StructureType: Building, Tower, Ruin, Bunker, Outpost.
 *   RoomType:      Hall, Corridor, Storage, Office, Lab, Shrine, Stairwell.
 *
 *   StructureConfig:
 *     - type, seed, size (x/y/z blocks), floors, roomCountMin/Max,
 *       minRoomSize, maxRoomSize, corridorWidth,
 *       addWindows, addDoors, addStairs, ruinFactor (0=intact..1=ruin).
 *
 *   StructureRoom: id, type, boundsMin/Max (integer 3D), floor, connections.
 *   StructureDoor: id, roomA, roomB, position, width, height, isOpen.
 *   StructureMap:  rooms, doors, staircases (DoorDesc list), floorCount,
 *                  entrance/exit room ids.
 *
 *   StructureGenerator:
 *     - Generate(config): build a full StructureMap.
 *     - IsValidMap(map): validate connectivity and bounds.
 *     - GetRoomsByType(map, type): filter by room type.
 *     - GetFloorRooms(map, floor): all rooms on a given floor.
 *     - FindPath(map, fromRoom, toRoom): BFS door-graph path.
 *     - GeneratorStats: roomsPlaced, doorsPlaced, stairsPlaced,
 *                       generationTimeMs, failedAttempts.
 */

#include <cstdint>
#include <string>
#include <vector>

namespace PCG {

// ── Enums ─────────────────────────────────────────────────────────────────
enum class StructureType : uint8_t { Building, Tower, Ruin, Bunker, Outpost };
enum class RoomType      : uint8_t { Hall, Corridor, Storage, Office, Lab, Shrine, Stairwell };

// ── 3D integer coordinate ─────────────────────────────────────────────────
struct IVec3 {
    int32_t x{0}, y{0}, z{0};
};

// ── Room ──────────────────────────────────────────────────────────────────
struct StructureRoom {
    uint32_t             id{0};
    RoomType             type{RoomType::Hall};
    IVec3                boundsMin;
    IVec3                boundsMax;
    uint32_t             floor{0};
    std::vector<uint32_t> connections; ///< Connected room ids (via doors)
};

// ── Door / staircase ──────────────────────────────────────────────────────
struct StructureDoor {
    uint32_t id{0};
    uint32_t roomA{0};
    uint32_t roomB{0};
    IVec3    position;   ///< Grid position of door centre
    uint32_t width{1};
    uint32_t height{2};
    bool     isOpen{true};
    bool     isStair{false};
};

// ── Full structure map ────────────────────────────────────────────────────
struct StructureMap {
    std::vector<StructureRoom> rooms;
    std::vector<StructureDoor> doors;
    std::vector<StructureDoor> staircases;
    uint32_t                   floorCount{1};
    uint32_t                   entranceRoomId{0};
    uint32_t                   exitRoomId{0};
    StructureType              type{StructureType::Building};
    uint32_t                   seed{0};
};

// ── Config ────────────────────────────────────────────────────────────────
struct StructureConfig {
    StructureType type{StructureType::Building};
    uint32_t      seed{0};
    uint32_t      sizeX{20};       ///< Total footprint width (blocks)
    uint32_t      sizeY{20};       ///< Total footprint depth (blocks)
    uint32_t      sizeZ{12};       ///< Total height (blocks)
    uint32_t      floors{2};
    uint32_t      roomCountMin{4};
    uint32_t      roomCountMax{12};
    uint32_t      minRoomSize{3};  ///< Min side length (blocks)
    uint32_t      maxRoomSize{8};  ///< Max side length (blocks)
    uint32_t      corridorWidth{1};
    bool          addWindows{true};
    bool          addDoors{true};
    bool          addStairs{true};
    float         ruinFactor{0.0f}; ///< 0 = intact, 1 = fully ruined
};

// ── Stats ─────────────────────────────────────────────────────────────────
struct GeneratorStats {
    uint32_t roomsPlaced{0};
    uint32_t doorsPlaced{0};
    uint32_t stairsPlaced{0};
    double   generationTimeMs{0.0};
    uint32_t failedAttempts{0};
};

// ── Simple xorshift64 PRNG helper ─────────────────────────────────────────
inline uint64_t StructAdvance(uint64_t& rng) {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

// ── StructureGenerator ────────────────────────────────────────────────────
class StructureGenerator {
public:
    StructureGenerator() = default;
    ~StructureGenerator() = default;

    StructureGenerator(const StructureGenerator&) = delete;
    StructureGenerator& operator=(const StructureGenerator&) = delete;

    // ── generation ────────────────────────────────────────────
    StructureMap Generate(const StructureConfig& config);

    // ── validation ────────────────────────────────────────────
    bool IsValidMap(const StructureMap& map) const;

    // ── queries ───────────────────────────────────────────────
    std::vector<const StructureRoom*> GetRoomsByType(const StructureMap& map, RoomType type) const;
    std::vector<const StructureRoom*> GetFloorRooms(const StructureMap& map, uint32_t floor) const;
    std::vector<uint32_t>             FindPath(const StructureMap& map,
                                               uint32_t fromRoomId,
                                               uint32_t toRoomId) const;

    // ── stats ─────────────────────────────────────────────────
    GeneratorStats GetLastStats() const;
    void           ResetStats();

private:
    GeneratorStats m_stats;

    void  PlaceRooms(StructureMap& map, const StructureConfig& cfg,
                     uint64_t& rng) const;
    void  ConnectRooms(StructureMap& map, const StructureConfig& cfg,
                       uint64_t& rng) const;
    void  AddStaircases(StructureMap& map, const StructureConfig& cfg,
                        uint64_t& rng) const;
    void  ApplyRuin(StructureMap& map, float factor, uint64_t& rng) const;
    bool  Overlaps(const StructureRoom& a, const StructureRoom& b) const;
};

} // namespace PCG
