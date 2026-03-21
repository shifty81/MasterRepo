#pragma once
/**
 * @file SpaceLayoutGenerator.h
 * @brief Procedural interior layout for space stations / ships.
 *
 * SpaceLayoutGenerator produces room-and-corridor graphs for interior spaces:
 *   - Room: axis-aligned rectangular region with a type tag (Corridor, Quarters,
 *     Engineering, Bridge, Cargo, Lab, MedBay, Airlock, Utility)
 *   - Door: connection between two rooms with a side/offset specification
 *   - Corridor: thin rectangular Room linking two non-adjacent rooms
 *   - SpaceLayout: full graph of rooms, corridors, doors; connectivity validated
 *
 * Algorithm:
 *   1. Place seed rooms (Bridge at front, Engine at rear, others random).
 *   2. Binary-space-partition remaining area into candidate room rects.
 *   3. Connect nearest neighbours with corridors.
 *   4. Validate full graph is connected; add missing corridors if not.
 *   5. Insert door slots on shared walls.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace PCG {

// ── Room types ─────────────────────────────────────────────────────────────
enum class RoomType : uint8_t {
    Corridor, Quarters, Engineering, Bridge, Cargo,
    Lab, MedBay, Airlock, Utility
};

const char* RoomTypeName(RoomType t);

// ── Rect ──────────────────────────────────────────────────────────────────
struct SLRect { float x{0},y{0},w{0},h{0}; };

// ── Room ──────────────────────────────────────────────────────────────────
struct SLRoom {
    uint32_t  id{0};
    SLRect    bounds;
    RoomType  type{RoomType::Utility};
    std::string name;
    bool      isCorridor{false};
};

// ── Door ──────────────────────────────────────────────────────────────────
struct SLDoor {
    uint32_t id{0};
    uint32_t roomA{0};
    uint32_t roomB{0};
    float    x{0},y{0};  ///< World position of door centre
    bool     isAirlock{false};
};

// ── Layout ────────────────────────────────────────────────────────────────
struct SpaceLayout {
    std::vector<SLRoom> rooms;
    std::vector<SLDoor> doors;

    size_t RoomCount()     const { return rooms.size(); }
    size_t DoorCount()     const { return doors.size(); }
    bool   IsConnected()   const;           // BFS check

    const SLRoom* GetRoom(uint32_t id) const;
    std::vector<uint32_t> Neighbours(uint32_t roomId) const;
};

// ── Config ────────────────────────────────────────────────────────────────
struct SpaceLayoutConfig {
    float    width{100.0f};
    float    height{60.0f};
    uint32_t minRooms{6};
    uint32_t maxRooms{20};
    float    minRoomW{6.0f};
    float    minRoomH{5.0f};
    float    corridorWidth{2.0f};
    uint64_t seed{0};
};

// ── Generator ─────────────────────────────────────────────────────────────
class SpaceLayoutGenerator {
public:
    SpaceLayoutGenerator();
    ~SpaceLayoutGenerator();

    void SetConfig(const SpaceLayoutConfig& cfg);
    const SpaceLayoutConfig& Config() const;

    SpaceLayout Generate();

    using ProgressCb = std::function<void(float, const std::string&)>;
    void OnProgress(ProgressCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace PCG
