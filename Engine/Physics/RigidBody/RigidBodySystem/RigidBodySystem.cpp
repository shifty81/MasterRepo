#include "Engine/Physics/RigidBody/RigidBodySystem/RigidBodySystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct RigidBody {
    uint32_t  id{0};
    BodyDesc  desc;
    float     pos[3]{};
    float     quat[4]{0,0,0,1};
    float     linVel[3]{};
    float     angVel[3]{};
    float     force[3]{};
    float     torque[3]{};
    bool      enabled{true};
    bool      sleeping{false};

    std::function<void(uint32_t, const ContactPoint&)> onEnter;
    std::function<void(uint32_t, const ContactPoint&)> onStay;
    std::function<void(uint32_t)>                      onExit;
};

struct RigidBodySystem::Impl {
    std::vector<RigidBody> bodies;
    float gravity[3]{0,-9.81f,0};
    uint32_t substeps{2};
    float sleepThreshold{0.01f};
    uint32_t nextId{1};

    RigidBody* Find(uint32_t id) {
        for (auto& b : bodies) if (b.id==id) return &b;
        return nullptr;
    }
    const RigidBody* Find(uint32_t id) const {
        for (auto& b : bodies) if (b.id==id) return &b;
        return nullptr;
    }
};

static bool AABBOverlap(const RigidBody& a, const RigidBody& b, ContactPoint& cp) {
    float ar=0, br=0;
    if (a.desc.shape==ShapeType::AABB) {
        for (int i=0;i<3;i++) {
            float aMin=a.pos[i]-a.desc.halfExtent[i], aMax=a.pos[i]+a.desc.halfExtent[i];
            float bMin=b.pos[i]-b.desc.halfExtent[i], bMax=b.pos[i]+b.desc.halfExtent[i];
            if (aMax < bMin || bMax < aMin) return false;
        }
        // Simple contact normal = axis of minimum penetration
        float minPen=1e9f; int axis=1;
        for (int i=0;i<3;i++) {
            float pen = (a.pos[i]+a.desc.halfExtent[i]) - (b.pos[i]-b.desc.halfExtent[i]);
            float pen2= (b.pos[i]+b.desc.halfExtent[i]) - (a.pos[i]-a.desc.halfExtent[i]);
            float p = std::min(pen, pen2);
            if (p < minPen) { minPen=p; axis=i; }
        }
        cp.depth = minPen;
        cp.normal[0]=cp.normal[1]=cp.normal[2]=0.f;
        cp.normal[axis] = (a.pos[axis]<b.pos[axis]) ? -1.f : 1.f;
        for(int i=0;i<3;i++) cp.point[i]=(a.pos[i]+b.pos[i])*0.5f;
        return true;
    }
    // Sphere-Sphere
    float dx=a.pos[0]-b.pos[0], dy=a.pos[1]-b.pos[1], dz=a.pos[2]-b.pos[2];
    float dist2=dx*dx+dy*dy+dz*dz;
    ar=a.desc.halfExtent[0]; br=b.desc.halfExtent[0];
    float rsum=ar+br;
    if (dist2 >= rsum*rsum) return false;
    float dist=std::sqrt(dist2)+1e-6f;
    cp.normal[0]=dx/dist; cp.normal[1]=dy/dist; cp.normal[2]=dz/dist;
    cp.depth = rsum-dist;
    for(int i=0;i<3;i++) cp.point[i]=(a.pos[i]+b.pos[i])*0.5f;
    return true;
}

RigidBodySystem::RigidBodySystem()  : m_impl(new Impl) {}
RigidBodySystem::~RigidBodySystem() { Shutdown(); delete m_impl; }

void RigidBodySystem::Init()     {}
void RigidBodySystem::Shutdown() { m_impl->bodies.clear(); }

void RigidBodySystem::SetGravity(const float g[3]) { for(int i=0;i<3;i++) m_impl->gravity[i]=g[i]; }
void RigidBodySystem::SetSubsteps(uint32_t n)       { m_impl->substeps=std::max(1u,n); }
void RigidBodySystem::SetSleepThreshold(float v)    { m_impl->sleepThreshold=v; }

uint32_t RigidBodySystem::CreateBody(const BodyDesc& desc)
{
    RigidBody b; b.id=m_impl->nextId++; b.desc=desc;
    m_impl->bodies.push_back(b); return b.id;
}

void RigidBodySystem::DestroyBody(uint32_t id)
{
    auto& v=m_impl->bodies;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& b){ return b.id==id; }), v.end());
}

bool RigidBodySystem::HasBody(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void RigidBodySystem::SetPosition(uint32_t id, const float p[3])
{ auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) b->pos[i]=p[i]; }
void RigidBodySystem::SetOrientation(uint32_t id, const float q[4])
{ auto* b=m_impl->Find(id); if(b) for(int i=0;i<4;i++) b->quat[i]=q[i]; }
void RigidBodySystem::GetPosition(uint32_t id, float out[3]) const
{ const auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) out[i]=b->pos[i]; }
void RigidBodySystem::GetOrientation(uint32_t id, float out[4]) const
{ const auto* b=m_impl->Find(id); if(b) for(int i=0;i<4;i++) out[i]=b->quat[i]; }
void RigidBodySystem::SetLinearVelocity(uint32_t id, const float v[3])
{ auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) b->linVel[i]=v[i]; }
void RigidBodySystem::SetAngularVelocity(uint32_t id, const float v[3])
{ auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) b->angVel[i]=v[i]; }
void RigidBodySystem::GetLinearVelocity(uint32_t id, float out[3]) const
{ const auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) out[i]=b->linVel[i]; }
void RigidBodySystem::GetAngularVelocity(uint32_t id, float out[3]) const
{ const auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) out[i]=b->angVel[i]; }

void RigidBodySystem::AddForce  (uint32_t id, const float f[3])
{ auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) b->force[i]+=f[i]; }
void RigidBodySystem::AddImpulse(uint32_t id, const float j[3])
{ auto* b=m_impl->Find(id); if(b&&b->desc.mass>0.f) for(int i=0;i<3;i++) b->linVel[i]+=j[i]/b->desc.mass; }
void RigidBodySystem::AddTorque (uint32_t id, const float t[3])
{ auto* b=m_impl->Find(id); if(b) for(int i=0;i<3;i++) b->torque[i]+=t[i]; }
void RigidBodySystem::ClearForces(uint32_t id)
{ auto* b=m_impl->Find(id); if(b){for(int i=0;i<3;i++){b->force[i]=0;b->torque[i]=0;}} }

void RigidBodySystem::SetMass(uint32_t id, float m) { auto* b=m_impl->Find(id); if(b) b->desc.mass=m; }
void RigidBodySystem::SetFriction(uint32_t id, float f) { auto* b=m_impl->Find(id); if(b) b->desc.friction=f; }
void RigidBodySystem::SetRestitution(uint32_t id, float r) { auto* b=m_impl->Find(id); if(b) b->desc.restitution=r; }
void RigidBodySystem::SetKinematic(uint32_t id, bool k) { auto* b=m_impl->Find(id); if(b) b->desc.type=k?BodyType::Kinematic:BodyType::Dynamic; }
void RigidBodySystem::SetEnabled(uint32_t id, bool e) { auto* b=m_impl->Find(id); if(b) b->enabled=e; }
float RigidBodySystem::GetMass(uint32_t id) const { const auto* b=m_impl->Find(id); return b?b->desc.mass:0.f; }

void RigidBodySystem::OnCollisionEnter(uint32_t id, std::function<void(uint32_t, const ContactPoint&)> cb)
{ auto* b=m_impl->Find(id); if(b) b->onEnter=cb; }
void RigidBodySystem::OnCollisionStay(uint32_t id, std::function<void(uint32_t, const ContactPoint&)> cb)
{ auto* b=m_impl->Find(id); if(b) b->onStay=cb; }
void RigidBodySystem::OnCollisionExit(uint32_t id, std::function<void(uint32_t)> cb)
{ auto* b=m_impl->Find(id); if(b) b->onExit=cb; }

bool RigidBodySystem::Overlap(uint32_t idA, uint32_t idB, ContactPoint* outContact) const
{
    const auto* a=m_impl->Find(idA), *b=m_impl->Find(idB);
    if (!a||!b) return false;
    ContactPoint cp;
    bool hit = AABBOverlap(*a, *b, cp);
    if (hit && outContact) *outContact=cp;
    return hit;
}

uint32_t RigidBodySystem::BodyCount() const { return (uint32_t)m_impl->bodies.size(); }

void RigidBodySystem::Tick(float dt)
{
    float subDt = dt / (float)m_impl->substeps;
    for (uint32_t step=0; step<m_impl->substeps; step++) {
        // Integrate dynamics
        for (auto& b : m_impl->bodies) {
            if (!b.enabled || b.desc.type!=BodyType::Dynamic) continue;
            float invM = b.desc.mass>0.f ? 1.f/b.desc.mass : 0.f;
            // Gravity + forces
            for (int i=0;i<3;i++)
                b.linVel[i] += (m_impl->gravity[i] + b.force[i]*invM) * subDt;
            // Damping
            for (int i=0;i<3;i++) b.linVel[i] *= (1.f - b.desc.linearDamp*subDt);
            // Integrate position
            for (int i=0;i<3;i++) b.pos[i] += b.linVel[i]*subDt;
            ClearForces(b.id);
            // Sleep check
            float v2=0; for(int i=0;i<3;i++) v2+=b.linVel[i]*b.linVel[i];
            b.sleeping = (v2 < m_impl->sleepThreshold*m_impl->sleepThreshold);
        }
        // Collision
        for (uint32_t i=0;i<(uint32_t)m_impl->bodies.size();i++) {
            for (uint32_t j=i+1;j<(uint32_t)m_impl->bodies.size();j++) {
                auto& a = m_impl->bodies[i];
                auto& bdy = m_impl->bodies[j];
                if (!a.enabled||!bdy.enabled) continue;
                if ((a.desc.layer & bdy.desc.mask)==0) continue;
                ContactPoint cp;
                if (AABBOverlap(a, bdy, cp)) {
                    // Simple penalty response
                    if (a.desc.type==BodyType::Dynamic) {
                        float impulse = cp.depth * 0.5f;
                        for(int k=0;k<3;k++) a.linVel[k] -= cp.normal[k]*impulse;
                    }
                    if (bdy.desc.type==BodyType::Dynamic) {
                        float impulse = cp.depth * 0.5f;
                        for(int k=0;k<3;k++) bdy.linVel[k] += cp.normal[k]*impulse;
                    }
                    if (a.onEnter) a.onEnter(bdy.id, cp);
                    if (bdy.onEnter) bdy.onEnter(a.id, cp);
                }
            }
        }
    }
}

} // namespace Engine
