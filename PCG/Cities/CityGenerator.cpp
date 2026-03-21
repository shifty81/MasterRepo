#include "PCG/Cities/CityGenerator.h"
#include <cmath>
#include <algorithm>

namespace PCG {

// ── LCG RNG ───────────────────────────────────────────────────────────────
static uint64_t cg_rng_state = 1;
static void     cg_rng_seed(uint64_t s) { cg_rng_state = s ? s : 1; }
static uint64_t cg_rng_next() {
    cg_rng_state = cg_rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return cg_rng_state;
}
static float cg_rng_f() {
    return static_cast<float>(cg_rng_next() >> 11) / static_cast<float>(1ULL << 53);
}
static float cg_rng_range(float lo, float hi) { return lo + cg_rng_f() * (hi - lo); }

static float cvLen(CityVec2 a, CityVec2 b) {
    float dx = a.x-b.x, dy = a.y-b.y;
    return std::sqrt(dx*dx + dy*dy);
}
static float cvArea(const std::vector<CityVec2>& poly) {
    float area = 0.0f;
    size_t n = poly.size();
    for (size_t i = 0; i < n; ++i) {
        const auto& p0 = poly[i];
        const auto& p1 = poly[(i+1)%n];
        area += (p0.x * p1.y) - (p1.x * p0.y);
    }
    return std::abs(area) * 0.5f;
}

struct CityGenerator::Impl {
    CityConfig config;
    CityLayout layout;
    bool       finished{false};
    int        step{0};
    uint32_t   nextInterId{1};
    uint32_t   nextRoadId{1};
    uint32_t   nextBlockId{1};
    uint32_t   nextLotId{1};
    std::vector<std::function<void(float,const std::string&)>> progressCbs;

    void notify(float f, const std::string& s) {
        for (auto& cb : progressCbs) cb(f, s);
    }
};

CityGenerator::CityGenerator() : m_impl(new Impl()) {}
CityGenerator::~CityGenerator() { delete m_impl; }

void CityGenerator::SetConfig(const CityConfig& cfg) { m_impl->config = cfg; }
const CityConfig& CityGenerator::Config() const { return m_impl->config; }

void CityGenerator::OnProgress(ProgressCb cb) {
    m_impl->progressCbs.push_back(std::move(cb));
}

CityLayout CityGenerator::Generate() {
    m_impl->layout = {};
    m_impl->finished = false;
    m_impl->step = 0;
    while (!m_impl->finished) GenerateStep();
    return m_impl->layout;
}

bool CityGenerator::GenerateStep() {
    if (m_impl->finished) return true;
    const auto& cfg = m_impl->config;
    auto& L = m_impl->layout;
    cg_rng_seed(cfg.seed + static_cast<uint64_t>(m_impl->step));

    switch (m_impl->step) {
    case 0: {
        // Step 1: build grid of intersections
        m_impl->notify(0.0f, "Building road grid");
        float cellW = cfg.worldSize / cfg.gridColumns;
        float cellH = cfg.worldSize / cfg.gridRows;
        for (uint32_t r = 0; r <= cfg.gridRows; ++r) {
            for (uint32_t c = 0; c <= cfg.gridColumns; ++c) {
                Intersection inter;
                inter.id  = m_impl->nextInterId++;
                inter.pos = {c * cellW, r * cellH};
                L.intersections.push_back(inter);
            }
        }
        break;
    }
    case 1: {
        // Step 2: connect intersections with road segments
        m_impl->notify(0.2f, "Connecting intersections");
        uint32_t cols = cfg.gridColumns + 1;
        auto interAt = [&](uint32_t r, uint32_t c) -> Intersection& {
            return L.intersections[r * cols + c];
        };
        for (uint32_t r = 0; r <= cfg.gridRows; ++r) {
            for (uint32_t c = 0; c <= cfg.gridColumns; ++c) {
                auto& a = interAt(r, c);
                if (c + 1 <= cfg.gridColumns) {
                    auto& b = interAt(r, c+1);
                    RoadSegment seg;
                    seg.id     = m_impl->nextRoadId++;
                    seg.fromId = a.id; seg.toId = b.id;
                    seg.type   = (r == 0 || r == cfg.gridRows) ? RoadType::Arterial : RoadType::Secondary;
                    seg.width  = seg.type == RoadType::Arterial ? cfg.arterialWidth : cfg.secondaryWidth;
                    seg.length = cvLen(a.pos, b.pos);
                    L.roads.push_back(seg);
                    ++a.valence; ++b.valence;
                }
                if (r + 1 <= cfg.gridRows) {
                    auto& b = interAt(r+1, c);
                    RoadSegment seg;
                    seg.id     = m_impl->nextRoadId++;
                    seg.fromId = a.id; seg.toId = b.id;
                    seg.type   = (c == 0 || c == cfg.gridColumns) ? RoadType::Arterial : RoadType::Secondary;
                    seg.width  = seg.type == RoadType::Arterial ? cfg.arterialWidth : cfg.secondaryWidth;
                    seg.length = cvLen(a.pos, b.pos);
                    L.roads.push_back(seg);
                    ++a.valence; ++b.valence;
                }
            }
        }
        break;
    }
    case 2: {
        // Step 3: extract blocks (each grid cell = one block)
        m_impl->notify(0.5f, "Extracting city blocks");
        uint32_t cols = cfg.gridColumns + 1;
        float cellW = cfg.worldSize / cfg.gridColumns;
        float cellH = cfg.worldSize / cfg.gridRows;
        for (uint32_t r = 0; r < cfg.gridRows; ++r) {
            for (uint32_t c = 0; c < cfg.gridColumns; ++c) {
                CityBlock blk;
                blk.id = m_impl->nextBlockId++;
                blk.outline = {
                    {c * cellW,       r * cellH},
                    {(c+1) * cellW,   r * cellH},
                    {(c+1) * cellW,   (r+1) * cellH},
                    {c * cellW,       (r+1) * cellH},
                };
                blk.area = cellW * cellH;
                L.blocks.push_back(blk);
            }
        }
        (void)cols;
        break;
    }
    case 3: {
        // Step 4: subdivide blocks into lots
        m_impl->notify(0.7f, "Subdividing lots");
        float cx = cfg.worldSize * 0.5f;
        float cy = cfg.worldSize * 0.5f;
        for (auto& blk : L.blocks) {
            if (blk.outline.size() < 4) continue;
            float bx = blk.outline[0].x;
            float by = blk.outline[0].y;
            float bw = blk.outline[1].x - bx;
            float bh = blk.outline[3].y - by;
            float lotW = cg_rng_range(cfg.minLotWidth, cfg.maxLotWidth);
            float x = bx;
            while (x + lotW <= bx + bw + 0.01f) {
                float actualW = std::min(lotW, bx + bw - x);
                if (actualW < cfg.minLotWidth * 0.5f) break;
                Lot lot;
                lot.id      = m_impl->nextLotId++;
                lot.blockId = blk.id;
                lot.corners = {{x, by},{x+actualW, by},{x+actualW, by+bh},{x, by+bh}};
                lot.area    = actualW * bh;
                // Land use: commercial near centre, residential further out
                float dist  = std::sqrt((x+actualW*0.5f-cx)*(x+actualW*0.5f-cx) +
                                        (by+bh*0.5f-cy)*(by+bh*0.5f-cy));
                float normalised = dist / (cfg.worldSize * 0.7f);
                float r = cg_rng_f();
                if (normalised < 0.2f && r < 0.6f + cfg.commercialCoreBias * 0.3f)
                    lot.use = LandUse::Commercial;
                else if (normalised < 0.4f && r < 0.3f)
                    lot.use = LandUse::Commercial;
                else if (r < 0.05f) lot.use = LandUse::Park;
                else if (r < 0.10f) lot.use = LandUse::Civic;
                else if (r < 0.15f) lot.use = LandUse::Industrial;
                else                lot.use = LandUse::Residential;
                blk.lotIds.push_back(lot.id);
                L.lots.push_back(std::move(lot));
                x += lotW;
                lotW = cg_rng_range(cfg.minLotWidth, cfg.maxLotWidth);
            }
        }
        break;
    }
    default:
        m_impl->notify(1.0f, "Done");
        m_impl->finished = true;
        break;
    }
    ++m_impl->step;
    return m_impl->finished;
}

bool CityGenerator::IsFinished() const { return m_impl->finished; }
const CityLayout& CityGenerator::CurrentLayout() const { return m_impl->layout; }

} // namespace PCG
