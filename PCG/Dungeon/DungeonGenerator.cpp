#include "PCG/Dungeon/DungeonGenerator.h"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

namespace PCG {

// ── Map helpers ───────────────────────────────────────────────────────────────
DungeonTile DungeonMap::Get(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) return DungeonTile::Wall;
    return tiles[y * width + x];
}
void DungeonMap::Set(int x, int y, DungeonTile t) {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    tiles[y * width + x] = t;
}

// ── LCG RNG (deterministic) ───────────────────────────────────────────────────
struct LCG {
    uint64_t state;
    explicit LCG(uint64_t seed) : state(seed ? seed : 1) {}
    uint64_t Next() {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return state;
    }
    int RandInt(int lo, int hi) {
        if (lo >= hi) return lo;
        return lo + static_cast<int>(Next() % static_cast<uint64_t>(hi - lo));
    }
    float RandFloat() {
        return static_cast<float>(Next() >> 11) / static_cast<float>(1ULL << 53);
    }
};

// ── BSP Node ─────────────────────────────────────────────────────────────────
struct BSPNode {
    DungeonRect area;
    BSPNode*    left{nullptr};
    BSPNode*    right{nullptr};
    DungeonRoom room;
    bool        hasRoom{false};

    bool IsLeaf() const { return !left && !right; }
};

static BSPNode* Split(BSPNode* node, int depth, int maxDepth,
                       int minSize, float splitRatio, LCG& rng,
                       std::vector<BSPNode>& pool)
{
    if (depth >= maxDepth) return node;
    bool splitH = rng.RandInt(0, 2) == 0;
    // Prefer horizontal split for wide areas, vertical for tall.
    if (node->area.w > node->area.h * 1.25f) splitH = false;
    if (node->area.h > node->area.w * 1.25f) splitH = true;

    if (splitH) {
        int lo = static_cast<int>(node->area.h * splitRatio);
        int hi = static_cast<int>(node->area.h * (1.0f - splitRatio));
        if (lo < minSize || hi < minSize) return node;
        int split = rng.RandInt(lo, hi);
        pool.push_back({{node->area.x, node->area.y, node->area.w, split}});
        pool.push_back({{node->area.x, node->area.y + split, node->area.w, node->area.h - split}});
        node->left  = &pool[pool.size()-2];
        node->right = &pool[pool.size()-1];
    } else {
        int lo = static_cast<int>(node->area.w * splitRatio);
        int hi = static_cast<int>(node->area.w * (1.0f - splitRatio));
        if (lo < minSize || hi < minSize) return node;
        int split = rng.RandInt(lo, hi);
        pool.push_back({{node->area.x, node->area.y, split, node->area.h}});
        pool.push_back({{node->area.x + split, node->area.y, node->area.w - split, node->area.h}});
        node->left  = &pool[pool.size()-2];
        node->right = &pool[pool.size()-1];
    }
    Split(node->left,  depth + 1, maxDepth, minSize, splitRatio, rng, pool);
    Split(node->right, depth + 1, maxDepth, minSize, splitRatio, rng, pool);
    return node;
}

static void CarveRoom(BSPNode* node, DungeonMap& map, int minRoomSz, int maxRoomSz,
                       LCG& rng, uint32_t& roomId,
                       std::vector<DungeonRoom>& rooms,
                       DungeonRoomCallback* cb)
{
    if (!node) return;
    if (node->IsLeaf()) {
        // Carve a randomly-sized room inside this partition.
        int rw = rng.RandInt(minRoomSz, std::min(maxRoomSz, node->area.w - 2) + 1);
        int rh = rng.RandInt(minRoomSz, std::min(maxRoomSz, node->area.h - 2) + 1);
        int rx = node->area.x + rng.RandInt(1, node->area.w - rw);
        int ry = node->area.y + rng.RandInt(1, node->area.h - rh);
        for (int y = ry; y < ry + rh; ++y)
            for (int x = rx; x < rx + rw; ++x)
                map.Set(x, y, DungeonTile::Floor);
        DungeonRoom room{{roomId++}, {rx, ry, rw, rh}, ""};
        node->room    = room;
        node->hasRoom = true;
        rooms.push_back(room);
        if (cb) (*cb)(room);
        return;
    }
    CarveRoom(node->left,  map, minRoomSz, maxRoomSz, rng, roomId, rooms, cb);
    CarveRoom(node->right, map, minRoomSz, maxRoomSz, rng, roomId, rooms, cb);
}

static DungeonRect RoomCenter(BSPNode* node) {
    if (!node) return {};
    if (node->hasRoom) {
        return {node->room.bounds.CenterX(), node->room.bounds.CenterY(), 1, 1};
    }
    if (node->left)  return RoomCenter(node->left);
    if (node->right) return RoomCenter(node->right);
    return {node->area.CenterX(), node->area.CenterY(), 1, 1};
}

static void ConnectChildren(BSPNode* node, DungeonMap& map,
                              std::vector<DungeonCorridor>& corridors)
{
    if (!node || node->IsLeaf()) return;
    ConnectChildren(node->left,  map, corridors);
    ConnectChildren(node->right, map, corridors);

    DungeonRect lc = RoomCenter(node->left);
    DungeonRect rc = RoomCenter(node->right);
    int x0 = lc.x, y0 = lc.y, x1 = rc.x, y1 = rc.y;
    // Horizontal then vertical L-shaped corridor.
    int xDir = (x1 > x0) ? 1 : -1;
    for (int x = x0; x != x1; x += xDir)
        map.Set(x, y0, DungeonTile::Corridor);
    int yDir = (y1 > y0) ? 1 : -1;
    for (int y = y0; y != y1 + yDir; y += yDir)
        map.Set(x1, y, DungeonTile::Corridor);
    corridors.push_back({x0, y0, x1, y1});
}

// ── Impl ─────────────────────────────────────────────────────────────────────
struct DungeonGenerator::Impl {
    DungeonConfig config;
};

DungeonGenerator::DungeonGenerator() : m_impl(new Impl()) {}
DungeonGenerator::~DungeonGenerator() { delete m_impl; }

void DungeonGenerator::SetConfig(const DungeonConfig& cfg) { m_impl->config = cfg; }
const DungeonConfig& DungeonGenerator::GetConfig() const   { return m_impl->config; }

DungeonMap DungeonGenerator::Generate() {
    return GenerateWithCallback(nullptr);
}

DungeonMap DungeonGenerator::GenerateWithCallback(DungeonRoomCallback cb) {
    const DungeonConfig& cfg = m_impl->config;
    uint64_t seed = cfg.seed ? cfg.seed :
        static_cast<uint64_t>(std::time(nullptr));
    LCG rng(seed);

    DungeonMap map;
    map.width  = cfg.width;
    map.height = cfg.height;
    map.tiles.assign(static_cast<size_t>(cfg.width) *
                     static_cast<size_t>(cfg.height), DungeonTile::Wall);

    // BSP — pre-allocate node pool (2^(depth+1) nodes max).
    std::vector<BSPNode> pool;
    pool.reserve(static_cast<size_t>(1) << (cfg.maxDepth + 1));
    pool.push_back({{0, 0, cfg.width, cfg.height}});
    BSPNode* root = &pool[0];

    Split(root, 0, cfg.maxDepth, cfg.minRoomSize, cfg.splitRatio, rng, pool);
    // After split pool may reallocate — fix root pointer.
    root = &pool[0];

    uint32_t roomId = 0;
    DungeonRoomCallback* cbPtr = cb ? &cb : nullptr;
    CarveRoom(root, map, cfg.minRoomSize, cfg.maxRoomSize, rng, roomId,
              map.rooms, cbPtr);

    // Tag start / boss rooms.
    if (!map.rooms.empty())          map.rooms.front().tag = "start";
    if (map.rooms.size() > 1)        map.rooms.back().tag  = "boss";

    ConnectChildren(root, map, map.corridors);

    return map;
}

} // namespace PCG
