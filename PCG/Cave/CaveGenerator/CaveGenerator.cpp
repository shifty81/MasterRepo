#include "PCG/Cave/CaveGenerator/CaveGenerator.h"
#include <cmath>
#include <queue>
#include <algorithm>

namespace PCG {

// ── CaveMap ───────────────────────────────────────────────────────────────
CaveTile CaveMap::Get(uint32_t x, uint32_t y) const {
    if (x>=width||y>=height) return CaveTile::Wall;
    return tiles[y*width+x];
}
void CaveMap::Set(uint32_t x, uint32_t y, CaveTile t) {
    if (x>=width||y>=height) return;
    tiles[y*width+x] = t;
}
bool CaveMap::IsFloor(uint32_t x, uint32_t y) const { return Get(x,y)==CaveTile::Floor; }
bool CaveMap::IsWall(uint32_t x, uint32_t y)  const { return Get(x,y)==CaveTile::Wall; }
bool CaveMap::InBounds(int32_t x, int32_t y)  const {
    return x>=0&&y>=0&&x<(int32_t)width&&y<(int32_t)height;
}

uint32_t CaveMap::FloodCount(uint32_t sx, uint32_t sy) const {
    if (!IsFloor(sx,sy)) return 0;
    std::vector<bool> visited(width*height, false);
    std::queue<std::pair<uint32_t,uint32_t>> q;
    q.push({sx,sy}); visited[sy*width+sx]=true;
    uint32_t count=0;
    const int32_t dx[]={0,0,1,-1}, dy[]={1,-1,0,0};
    while (!q.empty()) {
        auto [cx,cy]=q.front(); q.pop(); ++count;
        for (int d=0;d<4;++d) {
            int32_t nx=static_cast<int32_t>(cx)+dx[d];
            int32_t ny=static_cast<int32_t>(cy)+dy[d];
            if (!InBounds(nx,ny)) continue;
            uint32_t idx=static_cast<uint32_t>(ny)*width+static_cast<uint32_t>(nx);
            if (!visited[idx]&&IsFloor(static_cast<uint32_t>(nx),static_cast<uint32_t>(ny)))
                { visited[idx]=true; q.push({static_cast<uint32_t>(nx),static_cast<uint32_t>(ny)}); }
        }
    }
    return count;
}

bool CaveMap::IsFullyConnected() const {
    uint32_t total = FloorTileCount();
    if (total==0) return true;
    return FloodCount(entranceX,entranceY) == total;
}

uint32_t CaveMap::FloorTileCount() const {
    uint32_t n=0;
    for (auto t : tiles) if (t==CaveTile::Floor) ++n;
    return n;
}

// ── LCG RNG ───────────────────────────────────────────────────────────────
static uint64_t cv_rng=1;
static void cv_seed(uint64_t s){ cv_rng=s?s:1; }
static uint64_t cv_next(){ cv_rng=cv_rng*6364136223846793005ULL+1442695040888963407ULL; return cv_rng; }
static float cv_f(){ return static_cast<float>(cv_next()>>11)/static_cast<float>(1ULL<<53); }
static uint32_t cv_u(uint32_t max){ return max>0?(uint32_t)(cv_next()%max):0; }

// ── Impl ─────────────────────────────────────────────────────────────────
struct CaveGenerator::Impl {
    CaveConfig config;
    std::vector<ProgressCb> cbs;
    void notify(float f, const std::string& s) { for (auto& cb:cbs) cb(f,s); }
};

CaveGenerator::CaveGenerator() : m_impl(new Impl()) {}
CaveGenerator::~CaveGenerator() { delete m_impl; }
void CaveGenerator::SetConfig(const CaveConfig& c) { m_impl->config = c; }
const CaveConfig& CaveGenerator::Config() const { return m_impl->config; }
void CaveGenerator::OnProgress(ProgressCb cb) { m_impl->cbs.push_back(std::move(cb)); }

// Count 3x3 wall neighbours
static uint32_t wallNeighbours(const CaveMap& map, int32_t x, int32_t y) {
    uint32_t count=0;
    for (int dy=-1;dy<=1;++dy) for (int dx=-1;dx<=1;++dx) {
        if (dx==0&&dy==0) continue;
        int32_t nx=x+dx, ny=y+dy;
        if (!map.InBounds(nx,ny)||map.IsWall(static_cast<uint32_t>(nx),static_cast<uint32_t>(ny)))
            ++count;
    }
    return count;
}

// Remove isolated floor islands smaller than minSize; returns largest island start
static std::pair<uint32_t,uint32_t> removeSmallIslands(CaveMap& map, uint32_t minSize) {
    std::vector<bool> visited(map.width*map.height, false);
    const int32_t dx[]={0,0,1,-1}, dy[]={1,-1,0,0};
    uint32_t bestX=0,bestY=0,bestCount=0;
    for (uint32_t y=0;y<map.height;++y) for (uint32_t x=0;x<map.width;++x) {
        uint32_t idx=y*map.width+x;
        if (visited[idx]||map.IsWall(x,y)) continue;
        std::vector<std::pair<uint32_t,uint32_t>> region;
        std::queue<std::pair<uint32_t,uint32_t>> q;
        q.push({x,y}); visited[idx]=true;
        while (!q.empty()) {
            auto [cx,cy]=q.front(); q.pop();
            region.push_back({cx,cy});
            for (int d=0;d<4;++d) {
                int32_t nx=static_cast<int32_t>(cx)+dx[d];
                int32_t ny=static_cast<int32_t>(cy)+dy[d];
                if (!map.InBounds(nx,ny)) continue;
                uint32_t ni=static_cast<uint32_t>(ny)*map.width+static_cast<uint32_t>(nx);
                if (!visited[ni]&&map.IsFloor(static_cast<uint32_t>(nx),static_cast<uint32_t>(ny)))
                    { visited[ni]=true; q.push({static_cast<uint32_t>(nx),static_cast<uint32_t>(ny)}); }
            }
        }
        if (region.size()>bestCount){ bestCount=static_cast<uint32_t>(region.size()); bestX=region[0].first; bestY=region[0].second; }
        if (region.size()<minSize)
            for (auto [rx,ry]:region) map.Set(rx,ry,CaveTile::Wall);
    }
    return {bestX,bestY};
}

// Carve a straight passage between two points
static void carveLine(CaveMap& map, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) {
    int32_t dx=static_cast<int32_t>(x1)-static_cast<int32_t>(x0);
    int32_t dy=static_cast<int32_t>(y1)-static_cast<int32_t>(y0);
    int32_t steps=std::max(std::abs(dx),std::abs(dy));
    for (int32_t i=0;i<=steps;++i) {
        uint32_t cx=static_cast<uint32_t>(static_cast<int32_t>(x0)+(steps?dx*i/steps:0));
        uint32_t cy=static_cast<uint32_t>(static_cast<int32_t>(y0)+(steps?dy*i/steps:0));
        map.Set(cx,cy,CaveTile::Floor);
        if (cx>0) map.Set(cx-1,cy,CaveTile::Floor); // 1-wide passage
    }
}

CaveMap CaveGenerator::Generate() {
    const auto& cfg = m_impl->config;
    cv_seed(cfg.seed);
    CaveMap map;
    map.width  = cfg.width;
    map.height = cfg.height;
    map.tiles.resize(cfg.width * cfg.height, CaveTile::Wall);

    m_impl->notify(0.0f, "Initial fill");
    // 1. Random fill
    for (uint32_t y=1;y<cfg.height-1;++y)
        for (uint32_t x=1;x<cfg.width-1;++x)
            map.Set(x,y, cv_f()<cfg.fillDensity ? CaveTile::Wall : CaveTile::Floor);

    // 2. Cellular automata
    m_impl->notify(0.2f, "Cellular automata");
    std::vector<CaveTile> buf(map.tiles);
    for (uint32_t step=0;step<cfg.automataSteps;++step) {
        for (uint32_t y=1;y<cfg.height-1;++y)
            for (uint32_t x=1;x<cfg.width-1;++x) {
                uint32_t wn = wallNeighbours(map,static_cast<int32_t>(x),static_cast<int32_t>(y));
                buf[y*cfg.width+x] = (wn>=cfg.birthNeighbours)?CaveTile::Wall:CaveTile::Floor;
            }
        map.tiles = buf;
    }

    // 3. Remove islands
    m_impl->notify(0.5f, "Removing islands");
    auto [ex,ey] = removeSmallIslands(map, cfg.minIslandSize);
    map.entranceX=ex; map.entranceY=ey;

    // 4. Place vaults
    m_impl->notify(0.65f, "Placing vaults");
    static const char* vaultTypes[] = {"treasure","boss","spawn","rest","armory"};
    for (uint32_t v=0;v<cfg.vaultCount;++v) {
        uint32_t vw = cfg.vaultMinW + cv_u(cfg.vaultMaxW-cfg.vaultMinW+1);
        uint32_t vh = cfg.vaultMinH + cv_u(cfg.vaultMaxH-cfg.vaultMinH+1);
        uint32_t vx = 2 + cv_u(cfg.width  - vw - 4);
        uint32_t vy = 2 + cv_u(cfg.height - vh - 4);
        for (uint32_t yy=vy;yy<vy+vh;++yy)
            for (uint32_t xx=vx;xx<vx+vw;++xx) map.Set(xx,yy,CaveTile::Floor);
        CaveVault vault{vx,vy,vw,vh,vaultTypes[cv_u(5)]};
        map.vaults.push_back(vault);
        // Connect to entrance
        carveLine(map, ex, ey, vx+vw/2, vy+vh/2);
    }

    // 5. Set exit to far vault
    if (!map.vaults.empty()) {
        const auto& last = map.vaults.back();
        map.exitX = last.x + last.w/2;
        map.exitY = last.y + last.h/2;
    } else {
        map.exitX = cfg.width-2;
        map.exitY = cfg.height-2;
        map.Set(map.exitX, map.exitY, CaveTile::Floor);
        carveLine(map, ex, ey, map.exitX, map.exitY);
    }

    m_impl->notify(1.0f, "Done");
    return map;
}

} // namespace PCG
