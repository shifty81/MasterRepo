#include "Runtime/Physics/PhysicsWorld/PhysicsWorld.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Runtime {
using Vec3 = Engine::Math::Vec3;

// ── Helpers ───────────────────────────────────────────────────────────────
static Vec3 vAdd(const Vec3& a, const Vec3& b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static Vec3 vSub(const Vec3& a, const Vec3& b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static Vec3 vMul(const Vec3& v, float s) { return {v.x*s,v.y*s,v.z*s}; }
static float vDot(const Vec3& a, const Vec3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static float vLen(const Vec3& v) { return std::sqrt(vDot(v,v)); }
static Vec3 vNorm(const Vec3& v) { float l=vLen(v); return l>1e-7f?vMul(v,1.0f/l):Vec3{}; }

// ── AABB overlap + penetration ────────────────────────────────────────────
struct Overlap { bool hit{false}; Vec3 normal{}; float depth{0}; };
static Overlap aabbOverlap(const RigidBody& a, const RigidBody& b) {
    Vec3 diff = vSub(a.position, b.position);
    float ox = (a.halfExtents.x + b.halfExtents.x) - std::abs(diff.x);
    float oy = (a.halfExtents.y + b.halfExtents.y) - std::abs(diff.y);
    float oz = (a.halfExtents.z + b.halfExtents.z) - std::abs(diff.z);
    if (ox<=0||oy<=0||oz<=0) return {};
    Overlap r; r.hit = true;
    if (ox<oy && ox<oz) { r.depth=ox; r.normal={diff.x>0?1.0f:-1.0f,0,0}; }
    else if (oy<oz)     { r.depth=oy; r.normal={0,diff.y>0?1.0f:-1.0f,0}; }
    else                { r.depth=oz; r.normal={0,0,diff.z>0?1.0f:-1.0f}; }
    return r;
}

// ── Grid cell hash ────────────────────────────────────────────────────────
struct GridCell { int32_t x,y,z;
    bool operator==(const GridCell& o) const { return x==o.x&&y==o.y&&z==o.z; }
};
struct GridCellHash {
    size_t operator()(const GridCell& c) const {
        size_t h = std::hash<int32_t>()(c.x);
        h ^= std::hash<int32_t>()(c.y) + 0x9e3779b9 + (h<<6) + (h>>2);
        h ^= std::hash<int32_t>()(c.z) + 0x9e3779b9 + (h<<6) + (h>>2);
        return h;
    }
};

// ── Impl ─────────────────────────────────────────────────────────────────
struct PhysicsWorld::Impl {
    std::vector<RigidBody>         bodies;
    PhysicsConfig                  config;
    std::vector<CollisionCb>       cbs;
    uint64_t                       frame{0};
    uint32_t                       lastCollisions{0};
    uint32_t                       nextId{1};
};

PhysicsWorld::PhysicsWorld() : m_impl(new Impl()) {}
PhysicsWorld::PhysicsWorld(const PhysicsConfig& cfg) : m_impl(new Impl()) {
    m_impl->config = cfg;
}
PhysicsWorld::~PhysicsWorld() { delete m_impl; }

void PhysicsWorld::SetConfig(const PhysicsConfig& c) { m_impl->config = c; }
const PhysicsConfig& PhysicsWorld::Config() const { return m_impl->config; }

uint32_t PhysicsWorld::AddBody(const RigidBody& proto) {
    RigidBody b = proto;
    b.id = m_impl->nextId++;
    m_impl->bodies.push_back(b);
    return b.id;
}

bool PhysicsWorld::RemoveBody(uint32_t id) {
    auto it = std::find_if(m_impl->bodies.begin(), m_impl->bodies.end(),
        [id](const RigidBody& b){ return b.id==id; });
    if (it==m_impl->bodies.end()) return false;
    m_impl->bodies.erase(it);
    return true;
}

RigidBody* PhysicsWorld::GetBody(uint32_t id) {
    for (auto& b : m_impl->bodies) if (b.id==id) return &b;
    return nullptr;
}
const RigidBody* PhysicsWorld::GetBody(uint32_t id) const {
    for (const auto& b : m_impl->bodies) if (b.id==id) return &b;
    return nullptr;
}
std::vector<uint32_t> PhysicsWorld::BodyIDs() const {
    std::vector<uint32_t> ids;
    for (const auto& b : m_impl->bodies) ids.push_back(b.id);
    return ids;
}
void PhysicsWorld::Clear() { m_impl->bodies.clear(); }

void PhysicsWorld::ApplyForce(uint32_t id, const Vec3& f) {
    if (auto* b = GetBody(id)) { b->force=vAdd(b->force,f); b->sleeping=false; }
}
void PhysicsWorld::ApplyImpulse(uint32_t id, const Vec3& imp) {
    if (auto* b = GetBody(id); b && b->type==BodyType::Dynamic)
        b->velocity = vAdd(b->velocity, imp);
}
void PhysicsWorld::SetVelocity(uint32_t id, const Vec3& vel) {
    if (auto* b = GetBody(id)) b->velocity=vel;
}

void PhysicsWorld::OnCollision(CollisionCb cb) { m_impl->cbs.push_back(std::move(cb)); }
uint64_t PhysicsWorld::FrameCount() const { return m_impl->frame; }
uint32_t PhysicsWorld::CollisionCount() const { return m_impl->lastCollisions; }

// ── Step ─────────────────────────────────────────────────────────────────
void PhysicsWorld::Step(float dt) {
    const auto& cfg = m_impl->config;
    float subDt = dt / static_cast<float>(std::max(1u, cfg.subSteps));
    m_impl->lastCollisions = 0;

    for (uint32_t sub = 0; sub < cfg.subSteps; ++sub) {
        // 1. Integrate dynamics
        for (auto& b : m_impl->bodies) {
            if (b.type != BodyType::Dynamic) continue;
            if (b.sleeping) continue;
            // Gravity
            b.force = vAdd(b.force, vMul(cfg.gravity, b.mass));
            // Integrate velocity
            b.velocity = vAdd(b.velocity, vMul(b.force, subDt / b.mass));
            b.force = {};
            // Integrate position
            b.position = vAdd(b.position, vMul(b.velocity, subDt));
            // Sleep check
            if (vLen(b.velocity) < cfg.sleepThreshold) b.sleeping = true;
        }

        // 2. Broad-phase: spatial grid
        std::unordered_map<GridCell, std::vector<uint32_t>, GridCellHash> grid;
        float cellSize = cfg.cellSize;
        for (size_t i = 0; i < m_impl->bodies.size(); ++i) {
            const auto& b = m_impl->bodies[i];
            Vec3 mn = b.minBound(), mx = b.maxBound();
            int32_t x0=static_cast<int32_t>(std::floor(mn.x/cellSize));
            int32_t x1=static_cast<int32_t>(std::floor(mx.x/cellSize));
            int32_t y0=static_cast<int32_t>(std::floor(mn.y/cellSize));
            int32_t y1=static_cast<int32_t>(std::floor(mx.y/cellSize));
            int32_t z0=static_cast<int32_t>(std::floor(mn.z/cellSize));
            int32_t z1=static_cast<int32_t>(std::floor(mx.z/cellSize));
            for (int32_t x=x0;x<=x1;++x)
                for (int32_t y=y0;y<=y1;++y)
                    for (int32_t z=z0;z<=z1;++z)
                        grid[{x,y,z}].push_back(static_cast<uint32_t>(i));
        }

        // 3. Narrow-phase + resolution
        std::unordered_set<uint64_t> tested;
        for (auto& [cell, indices] : grid) {
            for (size_t ii = 0; ii < indices.size(); ++ii) {
                for (size_t jj = ii+1; jj < indices.size(); ++jj) {
                    uint32_t ai = indices[ii], bi = indices[jj];
                    uint64_t key = (static_cast<uint64_t>(std::min(ai,bi))<<32)|std::max(ai,bi);
                    if (!tested.insert(key).second) continue;
                    auto& a = m_impl->bodies[ai];
                    auto& b = m_impl->bodies[bi];
                    if (a.type==BodyType::Static && b.type==BodyType::Static) continue;
                    auto ov = aabbOverlap(a, b);
                    if (!ov.hit) continue;
                    ++m_impl->lastCollisions;
                    CollisionEvent evt;
                    evt.bodyA = a.id; evt.bodyB = b.id;
                    evt.normal = ov.normal; evt.penetration = ov.depth;
                    for (auto& cb : m_impl->cbs) cb(evt);
                    // Positional correction
                    float totalMass = ((a.type==BodyType::Dynamic)?a.mass:0.0f)
                                    + ((b.type==BodyType::Dynamic)?b.mass:0.0f);
                    if (totalMass < 1e-7f) continue;
                    float correction = ov.depth / totalMass * 0.8f;
                    if (a.type==BodyType::Dynamic)
                        a.position=vSub(a.position, vMul(ov.normal,correction*a.mass));
                    if (b.type==BodyType::Dynamic)
                        b.position=vAdd(b.position, vMul(ov.normal,correction*b.mass));
                    // Impulse
                    float e = std::min(a.restitution, b.restitution);
                    Vec3 relVel = vSub(a.velocity, b.velocity);
                    float vn = vDot(relVel, ov.normal);
                    if (vn >= 0) continue;
                    float j = -(1+e)*vn / totalMass;
                    Vec3 imp = vMul(ov.normal, j);
                    if (a.type==BodyType::Dynamic)
                        a.velocity=vAdd(a.velocity, vMul(imp, a.mass));
                    if (b.type==BodyType::Dynamic)
                        b.velocity=vSub(b.velocity, vMul(imp, b.mass));
                }
            }
        }
    }
    ++m_impl->frame;
}

// ── Query ─────────────────────────────────────────────────────────────────
std::vector<uint32_t> PhysicsWorld::QueryAABB(const Vec3& mn, const Vec3& mx) const {
    std::vector<uint32_t> out;
    for (const auto& b : m_impl->bodies) {
        Vec3 bmn=b.minBound(), bmx=b.maxBound();
        if (bmx.x<mn.x||bmn.x>mx.x||bmx.y<mn.y||bmn.y>mx.y||bmx.z<mn.z||bmn.z>mx.z) continue;
        out.push_back(b.id);
    }
    return out;
}

uint32_t PhysicsWorld::RayCast(const Vec3& origin, const Vec3& dir,
                                 float maxDist, float& hitDist) const {
    float len = vLen(dir); if (len<1e-7f) return 0;
    Vec3 d = vMul(dir,1.0f/len);
    float best = maxDist; uint32_t bestId = 0;
    for (const auto& b : m_impl->bodies) {
        Vec3 mn=b.minBound(), mx=b.maxBound();
        float tmin=0, tmax=best;
        bool miss=false;
        for (int i=0;i<3;++i) {
            float o=(i==0?origin.x:i==1?origin.y:origin.z);
            float di=(i==0?d.x:i==1?d.y:d.z);
            float blo=(i==0?mn.x:i==1?mn.y:mn.z);
            float bhi=(i==0?mx.x:i==1?mx.y:mx.z);
            if (std::abs(di)<1e-7f) { if (o<blo||o>bhi){miss=true;break;} continue; }
            float t1=(blo-o)/di, t2=(bhi-o)/di;
            if (t1>t2) std::swap(t1,t2);
            tmin=std::max(tmin,t1); tmax=std::min(tmax,t2);
            if (tmin>tmax){miss=true;break;}
        }
        if (!miss && tmin<best) { best=tmin; bestId=b.id; }
    }
    hitDist=best; return bestId;
}

} // namespace Runtime
