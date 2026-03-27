#include "Engine/AI/EQS/EnvironmentQuery/EnvironmentQuery.h"
#include <algorithm>
#include <cmath>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Point generation ────────────────────────────────────────────────────────

static void GenerateGrid(const float centre[3], const GridGenerator& g,
                          std::vector<EQSPoint>& pts)
{
    int h = (int)g.halfExtent;
    for (int xi=-h;xi<=h;xi++)
    for (int zi=-h;zi<=h;zi++) {
        EQSPoint p;
        p.position[0] = centre[0] + xi*g.spacing;
        p.position[1] = centre[1] + g.heightOffset;
        p.position[2] = centre[2] + zi*g.spacing;
        pts.push_back(p);
    }
}

static void GenerateRing(const float centre[3], const RingGenerator& g,
                          std::vector<EQSPoint>& pts)
{
    float step = (float)(2*3.14159265f) / (float)g.numPoints;
    for (uint32_t i=0;i<g.numPoints;i++) {
        float angle = step*i;
        float r = g.minRadius + (g.maxRadius-g.minRadius)*(i%2==0?0.f:1.f);
        EQSPoint p;
        p.position[0] = centre[0] + r*std::cos(angle);
        p.position[1] = centre[1];
        p.position[2] = centre[2] + r*std::sin(angle);
        pts.push_back(p);
    }
}

static void GenerateRandom(const float centre[3], const RandomGenerator& g,
                             std::vector<EQSPoint>& pts)
{
    for (uint32_t i=0;i<g.count;i++) {
        float angle = (float)(std::rand()%360) * (3.14159265f/180.f);
        float r     = g.radius * (float)(std::rand()%1000)/1000.f;
        EQSPoint p;
        p.position[0] = centre[0] + r*std::cos(angle);
        p.position[1] = centre[1];
        p.position[2] = centre[2] + r*std::sin(angle);
        pts.push_back(p);
    }
}

// ── Scoring ─────────────────────────────────────────────────────────────────

static float Dist3(const float a[3], const float b[3]) {
    float dx=a[0]-b[0], dy=a[1]-b[1], dz=a[2]-b[2];
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}

static float Dot3(const float a[3], const float b[3]) {
    return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}

static void ApplyTest(const EQSTest& test, std::vector<EQSPoint>& pts,
                       RaycastTestFn rayFn)
{
    // Normalise scores across all points
    float minS=1e9f, maxS=-1e9f;
    std::vector<float> raw(pts.size(),0.f);

    if (std::holds_alternative<DistanceTest>(test)) {
        const auto& dt = std::get<DistanceTest>(test);
        for (size_t i=0;i<pts.size();i++) {
            float d = Dist3(pts[i].position, dt.to);
            raw[i] = dt.preferFar ? d : -d;
        }
    } else if (std::holds_alternative<LOSTest>(test)) {
        const auto& lt = std::get<LOSTest>(test);
        for (size_t i=0;i<pts.size();i++) {
            bool vis = true;
            if (rayFn) vis = !rayFn(pts[i].position, lt.target);
            raw[i] = ((vis == lt.wantVisible) ? 1.f : 0.f);
        }
    } else if (std::holds_alternative<DirectionTest>(test)) {
        const auto& dir = std::get<DirectionTest>(test);
        for (size_t i=0;i<pts.size();i++) {
            float v[3]={pts[i].position[0]-dir.querierPos[0],
                        pts[i].position[1]-dir.querierPos[1],
                        pts[i].position[2]-dir.querierPos[2]};
            float len=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
            if (len>1e-6f){v[0]/=len;v[1]/=len;v[2]/=len;}
            raw[i] = Dot3(v, dir.preferDir);
        }
    } else if (std::holds_alternative<CustomTest>(test)) {
        const auto& ct = std::get<CustomTest>(test);
        for (size_t i=0;i<pts.size();i++)
            raw[i] = ct.scoreFn ? ct.scoreFn(pts[i]) : 0.f;
    }

    for (auto v : raw) { minS=std::min(minS,v); maxS=std::max(maxS,v); }
    float rng = maxS-minS;
    float weight = 1.f;
    if (std::holds_alternative<LOSTest>(test))       weight = std::get<LOSTest>(test).weight;
    if (std::holds_alternative<DistanceTest>(test))  weight = std::get<DistanceTest>(test).weight;
    if (std::holds_alternative<DirectionTest>(test)) weight = std::get<DirectionTest>(test).weight;
    if (std::holds_alternative<CustomTest>(test))    weight = std::get<CustomTest>(test).weight;

    for (size_t i=0;i<pts.size();i++) {
        float norm = (rng>1e-6f) ? (raw[i]-minS)/rng : 0.5f;
        pts[i].score += norm * weight;
    }
}

// ── Impl ────────────────────────────────────────────────────────────────────

struct PendingQuery {
    uint32_t    id{0};
    float       querierPos[3]{};
    EQSTemplate tmpl;
    EQSCallback cb;
};

struct EnvironmentQuery::Impl {
    RaycastTestFn rayFn;
    std::function<void(const EQSPoint&,bool)> debugFn;
    std::unordered_map<std::string, EQSTemplate> templates;

    std::mutex               mu;
    std::vector<PendingQuery> pending;
    std::vector<std::pair<uint32_t,EQSResult>> results;
    std::vector<std::thread> workers;
    uint32_t nextId{1};

    EQSResult Evaluate(const float qPos[3], const EQSTemplate& tmpl) const {
        std::vector<EQSPoint> pts;
        for (auto& g : tmpl.generators) {
            if (std::holds_alternative<GridGenerator>(g))   GenerateGrid(qPos,std::get<GridGenerator>(g),pts);
            else if (std::holds_alternative<RingGenerator>(g))  GenerateRing(qPos,std::get<RingGenerator>(g),pts);
            else if (std::holds_alternative<RandomGenerator>(g)) GenerateRandom(qPos,std::get<RandomGenerator>(g),pts);
        }
        for (auto& t : tmpl.tests) ApplyTest(t, pts, rayFn);

        // Sort descending
        std::sort(pts.begin(),pts.end(),[](auto& a,auto& b){ return a.score>b.score; });

        EQSResult r;
        r.succeeded = true;
        for (auto& p : pts) {
            if (p.score < tmpl.minScore) break;
            r.points.push_back(p);
            if (r.points.size() >= tmpl.topN) break;
        }
        return r;
    }
};

EnvironmentQuery::EnvironmentQuery()  : m_impl(new Impl) {}
EnvironmentQuery::~EnvironmentQuery() { Shutdown(); delete m_impl; }

void EnvironmentQuery::Init()     {}
void EnvironmentQuery::Shutdown() {}

void EnvironmentQuery::SetRaycastFn(RaycastTestFn fn)  { m_impl->rayFn   = fn; }
void EnvironmentQuery::SetDebugDrawFn(std::function<void(const EQSPoint&,bool)> fn)
{ m_impl->debugFn = fn; }

EQSResult EnvironmentQuery::RunQuerySync(const float qPos[3],
                                          const EQSTemplate& tmpl) const
{
    return m_impl->Evaluate(qPos, tmpl);
}

uint32_t EnvironmentQuery::RunQuery(const float qPos[3],
                                     const EQSTemplate& tmpl,
                                     EQSCallback cb)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    uint32_t id = m_impl->nextId++;
    PendingQuery pq;
    pq.id = id;
    pq.tmpl = tmpl;
    pq.cb   = cb;
    for (int i=0;i<3;i++) pq.querierPos[i] = qPos[i];
    m_impl->pending.push_back(pq);
    return id;
}

void EnvironmentQuery::CancelQuery(uint32_t queryId)
{
    std::lock_guard<std::mutex> lk(m_impl->mu);
    auto& v = m_impl->pending;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& p){ return p.id==queryId; }), v.end());
}

void EnvironmentQuery::Tick()
{
    std::vector<PendingQuery> work;
    {
        std::lock_guard<std::mutex> lk(m_impl->mu);
        work.swap(m_impl->pending);
    }
    for (auto& pq : work) {
        auto res = m_impl->Evaluate(pq.querierPos, pq.tmpl);
        if (pq.cb) pq.cb(res);
    }
}

void EnvironmentQuery::RegisterTemplate(const EQSTemplate& tmpl)
{
    m_impl->templates[tmpl.name] = tmpl;
}

uint32_t EnvironmentQuery::RunQueryByName(const float qPos[3],
                                           const std::string& name,
                                           EQSCallback cb)
{
    auto it = m_impl->templates.find(name);
    if (it == m_impl->templates.end()) return 0;
    return RunQuery(qPos, it->second, cb);
}

} // namespace Engine
