#include "PCG/Structures/StructureGenerator/StructureGenerator.h"
#include <algorithm>
#include <chrono>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <cstring>

namespace PCG {

static uint32_t RandRange(uint64_t& rng, uint32_t lo, uint32_t hi) {
    if (hi <= lo) return lo;
    return lo + static_cast<uint32_t>(StructAdvance(rng) % (hi - lo + 1));
}

// ── Overlap test ──────────────────────────────────────────────────────────
bool StructureGenerator::Overlaps(const StructureRoom& a, const StructureRoom& b) const {
    // 2D footprint overlap (ignoring Y/Z floor axis for same-floor check)
    return !(a.boundsMax.x <= b.boundsMin.x || b.boundsMax.x <= a.boundsMin.x ||
             a.boundsMax.z <= b.boundsMin.z || b.boundsMax.z <= a.boundsMin.z);
}

// ── Room placement ────────────────────────────────────────────────────────
void StructureGenerator::PlaceRooms(StructureMap& map, const StructureConfig& cfg,
                                    uint64_t& rng) const {
    const uint32_t floorH = cfg.sizeZ / std::max(cfg.floors, 1u);
    uint32_t nextId = 1;
    static const RoomType kRoomTypes[] = {
        RoomType::Hall, RoomType::Storage, RoomType::Office,
        RoomType::Lab,  RoomType::Shrine,  RoomType::Corridor
    };

    for (uint32_t f = 0; f < cfg.floors; ++f) {
        uint32_t target = RandRange(rng, cfg.roomCountMin, cfg.roomCountMax);
        uint32_t placed = 0;
        uint32_t attempts = 0;
        while (placed < target && attempts < target * 20) {
            ++attempts;
            uint32_t rw = RandRange(rng, cfg.minRoomSize, cfg.maxRoomSize);
            uint32_t rz = RandRange(rng, cfg.minRoomSize, cfg.maxRoomSize);
            uint32_t rx = RandRange(rng, 0, cfg.sizeX > rw ? cfg.sizeX - rw : 0);
            uint32_t rz0= RandRange(rng, 0, cfg.sizeY > rz ? cfg.sizeY - rz : 0);
            uint32_t yBase = f * floorH;

            StructureRoom room;
            room.id         = nextId++;
            room.type       = kRoomTypes[StructAdvance(rng) % 6];
            room.boundsMin  = {static_cast<int32_t>(rx),
                               static_cast<int32_t>(yBase),
                               static_cast<int32_t>(rz0)};
            room.boundsMax  = {static_cast<int32_t>(rx + rw),
                               static_cast<int32_t>(yBase + floorH),
                               static_cast<int32_t>(rz0 + rz)};
            room.floor      = f;

            bool ok = true;
            for (const auto& existing : map.rooms) {
                if (existing.floor == f && Overlaps(existing, room)) { ok = false; break; }
            }
            if (ok) { map.rooms.push_back(room); ++placed; }
        }
    }
    if (!map.rooms.empty()) {
        map.rooms.front().type = RoomType::Hall; // entrance room
        map.entranceRoomId = map.rooms.front().id;
        map.rooms.back().type  = RoomType::Hall; // exit room
        map.exitRoomId = map.rooms.back().id;
    }
}

// ── Room connection (nearest-neighbour MST on each floor) ─────────────────
void StructureGenerator::ConnectRooms(StructureMap& map, const StructureConfig& cfg,
                                      uint64_t& rng) const {
    if (!cfg.addDoors) return;
    uint32_t nextDoorId = 1;

    for (uint32_t f = 0; f < cfg.floors; ++f) {
        // Collect rooms on this floor
        std::vector<StructureRoom*> floorRooms;
        for (auto& r : map.rooms) if (r.floor == f) floorRooms.push_back(&r);
        if (floorRooms.size() < 2) continue;

        // Simple: connect each room to its nearest neighbour (greedy MST approx)
        std::unordered_set<uint32_t> connected;
        connected.insert(floorRooms[0]->id);

        while (connected.size() < floorRooms.size()) {
            float bestDist = 1e18f;
            StructureRoom* bestFrom = nullptr;
            StructureRoom* bestTo   = nullptr;

            for (auto* fa : floorRooms) {
                if (!connected.count(fa->id)) continue;
                for (auto* fb : floorRooms) {
                    if (connected.count(fb->id)) continue;
                    float dx = static_cast<float>((fa->boundsMin.x + fa->boundsMax.x) / 2
                                                - (fb->boundsMin.x + fb->boundsMax.x) / 2);
                    float dz = static_cast<float>((fa->boundsMin.z + fa->boundsMax.z) / 2
                                                - (fb->boundsMin.z + fb->boundsMax.z) / 2);
                    float d  = dx*dx + dz*dz;
                    if (d < bestDist) { bestDist = d; bestFrom = fa; bestTo = fb; }
                }
            }
            if (!bestFrom || !bestTo) break;
            connected.insert(bestTo->id);

            // Add door between bestFrom and bestTo
            StructureDoor door;
            door.id    = nextDoorId++;
            door.roomA = bestFrom->id;
            door.roomB = bestTo->id;
            door.position = {
                (bestFrom->boundsMin.x + bestFrom->boundsMax.x) / 2,
                bestFrom->boundsMin.y,
                (bestFrom->boundsMin.z + bestFrom->boundsMax.z) / 2
            };
            door.width  = cfg.corridorWidth;
            door.height = 2;
            door.isOpen = (StructAdvance(rng) % 4 != 0); // 75% open
            map.doors.push_back(door);
            bestFrom->connections.push_back(bestTo->id);
            bestTo->connections.push_back(bestFrom->id);
        }
    }
}

// ── Staircases ────────────────────────────────────────────────────────────
void StructureGenerator::AddStaircases(StructureMap& map, const StructureConfig& cfg,
                                       uint64_t& rng) const {
    if (!cfg.addStairs || cfg.floors < 2) return;
    uint32_t nextId = 1000;

    for (uint32_t f = 0; f + 1 < cfg.floors; ++f) {
        // Find one stairwell room on this floor and one on next
        StructureRoom* lower = nullptr;
        StructureRoom* upper = nullptr;
        for (auto& r : map.rooms) {
            if (r.floor == f   && !lower) lower = &r;
            if (r.floor == f+1 && !upper) upper = &r;
        }
        if (!lower || !upper) continue;

        StructureDoor stair;
        stair.id     = nextId++;
        stair.roomA  = lower->id;
        stair.roomB  = upper->id;
        stair.position = lower->boundsMin;
        stair.width  = 1;
        stair.height = 3;
        stair.isOpen = true;
        stair.isStair= true;
        map.staircases.push_back(stair);
        lower->connections.push_back(upper->id);
        upper->connections.push_back(lower->id);
        (void)rng;
    }
}

// ── Ruin factor ───────────────────────────────────────────────────────────
void StructureGenerator::ApplyRuin(StructureMap& map, float factor, uint64_t& rng) const {
    if (factor <= 0.0f) return;
    // Close some doors based on ruin factor
    for (auto& d : map.doors) {
        if (static_cast<float>(StructAdvance(rng) % 100) / 100.0f < factor)
            d.isOpen = false;
    }
}

// ── Generate ──────────────────────────────────────────────────────────────
StructureMap StructureGenerator::Generate(const StructureConfig& config) {
    using Clock = std::chrono::steady_clock;
    auto t0 = Clock::now();

    m_stats = {};
    uint64_t rng = static_cast<uint64_t>(config.seed ? config.seed : 0xDEADBEEFu);
    // Warm up
    for (int i = 0; i < 8; ++i) StructAdvance(rng);

    StructureMap map;
    map.type       = config.type;
    map.seed       = config.seed;
    map.floorCount = config.floors;

    PlaceRooms    (map, config, rng);
    ConnectRooms  (map, config, rng);
    AddStaircases (map, config, rng);
    ApplyRuin     (map, config.ruinFactor, rng);

    // Promote one room to stairwell per inter-floor connection
    for (const auto& s : map.staircases) {
        for (auto& r : map.rooms) if (r.id == s.roomA) r.type = RoomType::Stairwell;
    }

    m_stats.roomsPlaced   = static_cast<uint32_t>(map.rooms.size());
    m_stats.doorsPlaced   = static_cast<uint32_t>(map.doors.size());
    m_stats.stairsPlaced  = static_cast<uint32_t>(map.staircases.size());
    auto t1 = Clock::now();
    m_stats.generationTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    return map;
}

// ── Validation ────────────────────────────────────────────────────────────
bool StructureGenerator::IsValidMap(const StructureMap& map) const {
    if (map.rooms.empty()) return false;
    // BFS connectivity check from entrance
    std::unordered_set<uint32_t> visited;
    std::queue<uint32_t> q;
    q.push(map.entranceRoomId);
    while (!q.empty()) {
        uint32_t cur = q.front(); q.pop();
        if (visited.count(cur)) continue;
        visited.insert(cur);
        for (const auto& r : map.rooms) {
            if (r.id == cur) {
                for (uint32_t nb : r.connections)
                    if (!visited.count(nb)) q.push(nb);
            }
        }
    }
    return visited.size() == map.rooms.size();
}

// ── Queries ───────────────────────────────────────────────────────────────
std::vector<const StructureRoom*> StructureGenerator::GetRoomsByType(
    const StructureMap& map, RoomType type) const {
    std::vector<const StructureRoom*> out;
    for (const auto& r : map.rooms) if (r.type == type) out.push_back(&r);
    return out;
}

std::vector<const StructureRoom*> StructureGenerator::GetFloorRooms(
    const StructureMap& map, uint32_t floor) const {
    std::vector<const StructureRoom*> out;
    for (const auto& r : map.rooms) if (r.floor == floor) out.push_back(&r);
    return out;
}

std::vector<uint32_t> StructureGenerator::FindPath(
    const StructureMap& map, uint32_t fromId, uint32_t toId) const {
    if (fromId == toId) return {fromId};
    std::unordered_map<uint32_t, uint32_t> parent;
    std::queue<uint32_t> q;
    q.push(fromId);
    parent[fromId] = 0;
    while (!q.empty()) {
        uint32_t cur = q.front(); q.pop();
        if (cur == toId) break;
        for (const auto& r : map.rooms) {
            if (r.id == cur) {
                for (uint32_t nb : r.connections) {
                    if (!parent.count(nb)) { parent[nb] = cur; q.push(nb); }
                }
            }
        }
    }
    if (!parent.count(toId)) return {}; // no path
    std::vector<uint32_t> path;
    uint32_t cur = toId;
    while (cur != fromId) { path.push_back(cur); cur = parent[cur]; }
    path.push_back(fromId);
    std::reverse(path.begin(), path.end());
    return path;
}

// ── Stats ─────────────────────────────────────────────────────────────────
GeneratorStats StructureGenerator::GetLastStats() const { return m_stats; }
void           StructureGenerator::ResetStats()         { m_stats = {}; }

} // namespace PCG
