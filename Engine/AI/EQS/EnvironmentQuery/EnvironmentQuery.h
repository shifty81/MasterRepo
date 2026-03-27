#pragma once
/**
 * @file EnvironmentQuery.h
 * @brief AI Environment Query System (EQS) — spatial point generation & scoring.
 *
 * Features:
 *   - Query templates: Grid, Ring, Patrol Points, Cover Points, Flee Points
 *   - Point generators: grid around querier, random spread, nav-mesh samples
 *   - Scoring tests: distance to target, line-of-sight, dot-product direction,
 *                    dot-product away-from-threat, custom functor
 *   - Score normalization and weighted combination
 *   - Async evaluation (non-blocking, callback on result)
 *   - Result filtering: top-N, min-score threshold
 *   - Debug draw integration hook
 *   - Multiple simultaneous queries (per NPC)
 *
 * Typical usage:
 * @code
 *   EnvironmentQuery eq;
 *   eq.Init();
 *   eq.SetRaycastFn(MyRaycast);
 *   EQSTemplate tmpl;
 *   tmpl.AddGenerator(GridGenerator{8, 3.f});
 *   tmpl.AddTest(LOSTest{targetPos, 1.f});
 *   tmpl.AddTest(DistanceTest{querierPos, targetPos, 1.f, true});
 *   eq.RunQuery(querierPos, tmpl, [](EQSResult r){ MoveToward(r.bestPoint); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>
#include <optional>

namespace Engine {

struct EQSPoint {
    float    position[3]{};
    float    score{0.f};
    bool     valid{true};
};

// ── Generators ─────────────────────────────────────────────────────────────

struct GridGenerator {
    uint32_t halfExtent{4};       ///< samples per axis (total = (2*halfExtent+1)^2)
    float    spacing{1.5f};
    float    heightOffset{0.1f};  ///< lift above ground
};

struct RingGenerator {
    uint32_t numPoints{12};
    float    minRadius{3.f};
    float    maxRadius{8.f};
};

struct RandomGenerator {
    uint32_t count{20};
    float    radius{10.f};
};

using PointGenerator = std::variant<GridGenerator, RingGenerator, RandomGenerator>;

// ── Tests ───────────────────────────────────────────────────────────────────

using RaycastTestFn = std::function<bool(const float from[3], const float to[3])>;

struct LOSTest {
    float    target[3]{};
    float    weight{1.f};
    bool     wantVisible{true};   ///< true=prefer visible, false=prefer hidden
};

struct DistanceTest {
    float    from[3]{};
    float    to[3]{};
    float    weight{1.f};
    bool     preferFar{false};    ///< false=closer scores higher
    float    idealDist{5.f};
};

struct DirectionTest {
    float    querierPos[3]{};
    float    preferDir[3]{0,0,1};
    float    weight{1.f};
};

struct CustomTest {
    float    weight{1.f};
    std::function<float(const EQSPoint&)> scoreFn;
};

using EQSTest = std::variant<LOSTest, DistanceTest, DirectionTest, CustomTest>;

// ── Template & Result ────────────────────────────────────────────────────────

struct EQSTemplate {
    std::vector<PointGenerator> generators;
    std::vector<EQSTest>        tests;
    uint32_t                    topN{1};
    float                       minScore{0.f};
    std::string                 name;

    void AddGenerator(PointGenerator g) { generators.push_back(std::move(g)); }
    void AddTest(EQSTest t)             { tests.push_back(std::move(t)); }
};

struct EQSResult {
    std::vector<EQSPoint> points;    ///< sorted best-first, up to topN
    bool                  succeeded{false};
    float                 elapsed{0.f};  ///< query time in ms

    bool       HasResult() const { return succeeded && !points.empty(); }
    EQSPoint   Best()      const { return succeeded && !points.empty() ? points[0] : EQSPoint{}; }
};

using EQSCallback = std::function<void(EQSResult)>;

class EnvironmentQuery {
public:
    EnvironmentQuery();
    ~EnvironmentQuery();

    void Init();
    void Shutdown();

    // Integration
    void SetRaycastFn(RaycastTestFn fn);
    void SetDebugDrawFn(std::function<void(const EQSPoint&, bool isBest)> fn);

    // Synchronous query
    EQSResult RunQuerySync(const float querierPos[3],
                           const EQSTemplate& tmpl) const;

    // Async query (fires callback on next Tick, main-thread safe)
    uint32_t RunQuery(const float querierPos[3],
                      const EQSTemplate& tmpl,
                      EQSCallback cb);

    void CancelQuery(uint32_t queryId);
    void Tick();  ///< pump async results on main thread

    // Template registry (named presets)
    void RegisterTemplate(const EQSTemplate& tmpl);
    uint32_t RunQueryByName(const float querierPos[3],
                             const std::string& templateName,
                             EQSCallback cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
