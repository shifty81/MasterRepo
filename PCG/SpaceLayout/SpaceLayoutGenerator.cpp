#include "PCG/SpaceLayout/SpaceLayoutGenerator.h"
#include <cmath>
#include <algorithm>
#include <queue>

namespace PCG {

const char* RoomTypeName(RoomType t) {
    switch (t) {
    case RoomType::Corridor:    return "Corridor";
    case RoomType::Quarters:    return "Quarters";
    case RoomType::Engineering: return "Engineering";
    case RoomType::Bridge:      return "Bridge";
    case RoomType::Cargo:       return "Cargo";
    case RoomType::Lab:         return "Lab";
    case RoomType::MedBay:      return "MedBay";
    case RoomType::Airlock:     return "Airlock";
    default:                    return "Utility";
    }
}

// ── Connectivity check ────────────────────────────────────────────────────
bool SpaceLayout::IsConnected() const {
    if (rooms.empty()) return true;
    // BFS over room adjacency via doors
    std::unordered_map<uint32_t, std::vector<uint32_t>> adj;
    for (const auto& d : doors) {
        adj[d.roomA].push_back(d.roomB);
        adj[d.roomB].push_back(d.roomA);
    }
    std::unordered_map<uint32_t,bool> visited;
    std::queue<uint32_t> q;
    q.push(rooms[0].id);
    visited[rooms[0].id] = true;
    while (!q.empty()) {
        auto id = q.front(); q.pop();
        for (auto nb : adj[id]) if (!visited[nb]) { visited[nb]=true; q.push(nb); }
    }
    for (const auto& r : rooms) if (!visited.count(r.id)) return false;
    return true;
}

const SLRoom* SpaceLayout::GetRoom(uint32_t id) const {
    for (const auto& r : rooms) if (r.id == id) return &r;
    return nullptr;
}

std::vector<uint32_t> SpaceLayout::Neighbours(uint32_t roomId) const {
    std::vector<uint32_t> out;
    for (const auto& d : doors) {
        if (d.roomA == roomId) out.push_back(d.roomB);
        else if (d.roomB == roomId) out.push_back(d.roomA);
    }
    return out;
}

// ── LCG RNG ───────────────────────────────────────────────────────────────
static uint64_t sl_rng = 1;
static void     sl_seed(uint64_t s) { sl_rng = s ? s : 1; }
static uint64_t sl_next() {
    sl_rng = sl_rng * 6364136223846793005ULL + 1442695040888963407ULL;
    return sl_rng;
}
static float sl_f() {
    return static_cast<float>(sl_next()>>11) / static_cast<float>(1ULL<<53);
}
static float sl_range(float lo, float hi) { return lo + sl_f()*(hi-lo); }

// ── Impl ─────────────────────────────────────────────────────────────────
struct SpaceLayoutGenerator::Impl {
    SpaceLayoutConfig config;
    std::vector<ProgressCb> cbs;

    void notify(float f, const std::string& s) { for (auto& cb:cbs) cb(f,s); }
};

SpaceLayoutGenerator::SpaceLayoutGenerator() : m_impl(new Impl()) {}
SpaceLayoutGenerator::~SpaceLayoutGenerator() { delete m_impl; }

void SpaceLayoutGenerator::SetConfig(const SpaceLayoutConfig& c) { m_impl->config = c; }
const SpaceLayoutConfig& SpaceLayoutGenerator::Config() const { return m_impl->config; }
void SpaceLayoutGenerator::OnProgress(ProgressCb cb) { m_impl->cbs.push_back(std::move(cb)); }

SpaceLayout SpaceLayoutGenerator::Generate() {
    const auto& cfg = m_impl->config;
    sl_seed(cfg.seed);
    SpaceLayout layout;
    uint32_t nextRoomId = 1;
    uint32_t nextDoorId = 1;

    m_impl->notify(0.0f, "Placing seed rooms");

    // ── 1. Place seed rooms ────────────────────────────────────
    auto addRoom = [&](float x, float y, float w, float h, RoomType type) {
        SLRoom r;
        r.id     = nextRoomId++;
        r.bounds = {x,y,w,h};
        r.type   = type;
        r.name   = RoomTypeName(type);
        r.isCorridor = (type == RoomType::Corridor);
        layout.rooms.push_back(r);
        return r.id;
    };

    float W = cfg.width, H = cfg.height;
    float roomW = std::max(cfg.minRoomW, W * 0.18f);
    float roomH = std::max(cfg.minRoomH, H * 0.22f);

    uint32_t bridgeId = addRoom(W*0.05f, H*0.4f, roomW, roomH, RoomType::Bridge);
    uint32_t engId    = addRoom(W*0.78f, H*0.4f, roomW, roomH, RoomType::Engineering);

    // ── 2. BSP-style room placement ────────────────────────────
    m_impl->notify(0.3f, "BSP room placement");

    uint32_t targetRooms = cfg.minRooms +
        static_cast<uint32_t>(sl_f() * (cfg.maxRooms - cfg.minRooms));

    static const RoomType roomTypes[] = {
        RoomType::Quarters, RoomType::Cargo, RoomType::Lab,
        RoomType::MedBay, RoomType::Airlock, RoomType::Utility
    };

    float gridW = (W - roomW*2.0f) / 3.0f;
    float gridH = (H - roomH*2.0f) / 2.0f;

    for (uint32_t row = 0; row < 2; ++row) {
        for (uint32_t col = 0; col < 3; ++col) {
            if (layout.rooms.size() >= targetRooms) break;
            float x = roomW + col * gridW + sl_range(0.0f, gridW*0.2f);
            float y = roomH + row * gridH + sl_range(0.0f, gridH*0.2f);
            float w = cfg.minRoomW + sl_range(0.0f, cfg.minRoomW * 1.5f);
            float h = cfg.minRoomH + sl_range(0.0f, cfg.minRoomH * 1.5f);
            x = std::min(x, W - w - 1.0f);
            y = std::min(y, H - h - 1.0f);
            RoomType rt = roomTypes[sl_next() % 6];
            addRoom(x, y, w, h, rt);
        }
    }

    // ── 3. Connect rooms with corridors ────────────────────────
    m_impl->notify(0.6f, "Connecting with corridors");

    auto addDoor = [&](uint32_t a, uint32_t b, float x, float y) {
        SLDoor d;
        d.id = nextDoorId++;
        d.roomA = a; d.roomB = b;
        d.x = x; d.y = y;
        const SLRoom* rb = layout.GetRoom(b);
        d.isAirlock = rb && rb->type == RoomType::Airlock;
        layout.doors.push_back(d);
    };

    auto roomCentre = [](const SLRoom& r) -> std::pair<float,float> {
        return {r.bounds.x + r.bounds.w*0.5f, r.bounds.y + r.bounds.h*0.5f};
    };

    // Chain rooms together by nearest unconnected pair
    std::vector<uint32_t> connected = {bridgeId};
    std::vector<uint32_t> unconnected;
    for (const auto& r : layout.rooms)
        if (r.id != bridgeId) unconnected.push_back(r.id);

    while (!unconnected.empty()) {
        float bestDist = 1e30f;
        uint32_t bestA = 0, bestB = 0;
        for (uint32_t a : connected) {
            const SLRoom* ra = layout.GetRoom(a);
            if (!ra) continue;
            auto [ax,ay] = roomCentre(*ra);
            for (uint32_t b : unconnected) {
                const SLRoom* rb = layout.GetRoom(b);
                if (!rb) continue;
                auto [bx,by] = roomCentre(*rb);
                float d = (ax-bx)*(ax-bx)+(ay-by)*(ay-by);
                if (d < bestDist) { bestDist=d; bestA=a; bestB=b; }
            }
        }
        if (!bestA) break;
        const SLRoom* ra = layout.GetRoom(bestA);
        const SLRoom* rb = layout.GetRoom(bestB);
        auto [ax,ay] = roomCentre(*ra);
        auto [bx,by] = roomCentre(*rb);
        // Add an L-shaped corridor: first horizontal, then vertical
        float cw  = cfg.corridorWidth;
        uint32_t hId = addRoom(std::min(ax,bx), ay-cw*0.5f,
                               std::abs(bx-ax), cw, RoomType::Corridor);
        uint32_t vId = addRoom(bx-cw*0.5f, std::min(ay,by),
                               cw, std::abs(by-ay)+cw, RoomType::Corridor);
        addDoor(bestA, hId, ax, ay);
        addDoor(hId,   vId, bx, ay);
        addDoor(vId,   bestB, bx, by);
        connected.push_back(bestB);
        unconnected.erase(std::find(unconnected.begin(), unconnected.end(), bestB));
    }

    // Ensure engineering is reachable
    if (std::find(connected.begin(), connected.end(), engId) == connected.end()) {
        const SLRoom* ra = layout.GetRoom(bridgeId);
        const SLRoom* rb = layout.GetRoom(engId);
        auto [ax,ay] = roomCentre(*ra);
        auto [bx,by] = roomCentre(*rb);
        float cw = cfg.corridorWidth;
        uint32_t cId = addRoom(std::min(ax,bx), ay-cw*0.5f,
                               std::abs(bx-ax), cw, RoomType::Corridor);
        addDoor(bridgeId, cId, ax, ay);
        addDoor(cId, engId, bx, by);
    }

    m_impl->notify(1.0f, "Done");
    return layout;
}

} // namespace PCG
