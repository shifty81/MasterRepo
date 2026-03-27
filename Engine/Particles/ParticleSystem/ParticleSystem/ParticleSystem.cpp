#include "Engine/Particles/ParticleSystem/ParticleSystem/ParticleSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Engine {

struct Particle {
    float pos[3]{};
    float vel[3]{};
    float life{1.f};
    float maxLife{1.f};
    float size{0.1f};
    float rot{0.f};
    float col[4]{1,1,1,1};
    bool  alive{false};
};

static float Lerp(float a, float b, float t) { return a+(b-a)*t; }
static float RandF(float lo, float hi) {
    return lo + (hi-lo) * (float)std::rand() / (float)RAND_MAX;
}

struct Emitter {
    uint32_t        id{0};
    EmitterDesc     desc;
    float           pos[3]{};
    float           quat[4]{0,0,0,1};
    float           gravity[3]{0,-9.81f,0};
    float           wind[3]{};
    float           spawnAccum{0.f};
    bool            emitting{false};
    std::vector<Particle> pool;
    ParticleRenderData renderData;
    std::function<void(const float[3])> onDie;

    float SampleSize(float t) const {
        if (desc.sizeKeys.empty()) return Lerp(desc.sizeMin, desc.sizeMax, t);
        const auto& keys = desc.sizeKeys;
        if (t <= keys.front().t) return keys.front().size;
        if (t >= keys.back().t)  return keys.back().size;
        for (uint32_t i=0;i+1<keys.size();i++) {
            if (t>=keys[i].t && t<=keys[i+1].t) {
                float lt=(t-keys[i].t)/(keys[i+1].t-keys[i].t);
                return Lerp(keys[i].size, keys[i+1].size, lt);
            }
        }
        return 1.f;
    }

    void SampleColour(float t, float out[4]) const {
        if (desc.colorKeys.empty()) { out[0]=out[1]=out[2]=out[3]=1.f; return; }
        const auto& keys = desc.colorKeys;
        if (t <= keys.front().t) { out[0]=keys.front().r;out[1]=keys.front().g;out[2]=keys.front().b;out[3]=keys.front().a; return; }
        if (t >= keys.back().t)  { const auto& k=keys.back(); out[0]=k.r;out[1]=k.g;out[2]=k.b;out[3]=k.a; return; }
        for (uint32_t i=0;i+1<keys.size();i++) {
            if (t>=keys[i].t && t<=keys[i+1].t) {
                float lt=(t-keys[i].t)/(keys[i+1].t-keys[i].t);
                out[0]=Lerp(keys[i].r,keys[i+1].r,lt); out[1]=Lerp(keys[i].g,keys[i+1].g,lt);
                out[2]=Lerp(keys[i].b,keys[i+1].b,lt); out[3]=Lerp(keys[i].a,keys[i+1].a,lt);
                return;
            }
        }
    }

    Particle* SpawnSlot() {
        for (auto& p : pool) if (!p.alive) return &p;
        return nullptr;
    }

    void SpawnParticle() {
        auto* p = SpawnSlot(); if (!p) return;
        float life = RandF(desc.lifeMin, desc.lifeMax);
        float spd  = RandF(desc.speedMin, desc.speedMax);

        // Direction based on shape
        float dx=0, dy=1, dz=0;
        if (desc.shape==EmitterShape::Sphere) {
            float theta=(float)(RandF(0.f,(float)(2*M_PI))), phi=RandF(0.f,(float)M_PI);
            dx=std::sin(phi)*std::cos(theta); dy=std::cos(phi); dz=std::sin(phi)*std::sin(theta);
        } else if (desc.shape==EmitterShape::Cone) {
            float angle = RandF(0.f, desc.coneAngle*(float)M_PI/180.f);
            float azi   = RandF(0.f,(float)(2*M_PI));
            dx=std::sin(angle)*std::cos(azi); dz=std::sin(angle)*std::sin(azi); dy=std::cos(angle);
        }

        *p = Particle{};
        p->alive   = true;
        p->life    = life; p->maxLife = life;
        p->pos[0]  = pos[0]; p->pos[1]=pos[1]; p->pos[2]=pos[2];
        p->vel[0]  = dx*spd; p->vel[1]=dy*spd; p->vel[2]=dz*spd;
        p->size    = SampleSize(0.f);
        SampleColour(0.f, p->col);
    }

    void Tick(float dt) {
        // Spawn
        if (emitting && desc.emitRate > 0.f) {
            spawnAccum += desc.emitRate * dt;
            while (spawnAccum >= 1.f) { SpawnParticle(); spawnAccum -= 1.f; }
        }
        // Update
        for (auto& p : pool) {
            if (!p.alive) continue;
            p.life -= dt;
            if (p.life <= 0.f) {
                if (onDie) onDie(p.pos);
                p.alive=false; continue;
            }
            float t = 1.f - p.life/p.maxLife;
            // Physics
            for(int i=0;i<3;i++) p.vel[i] += (gravity[i]+wind[i])*dt;
            for(int i=0;i<3;i++) p.pos[i] += p.vel[i]*dt;
            // Ground collision
            if (desc.collide && p.pos[1] < desc.groundY) {
                p.pos[1]=desc.groundY; p.vel[1]=std::abs(p.vel[1])*desc.bounce;
            }
            // Visual
            p.size = SampleSize(t);
            SampleColour(t, p.col);
        }
        // Build render data
        renderData.positions.clear(); renderData.colours.clear();
        renderData.sizes.clear(); renderData.rotations.clear(); renderData.count=0;
        for (auto& p : pool) {
            if (!p.alive) continue;
            renderData.positions.push_back(p.pos[0]); renderData.positions.push_back(p.pos[1]); renderData.positions.push_back(p.pos[2]);
            for(int i=0;i<4;i++) renderData.colours.push_back(p.col[i]);
            renderData.sizes.push_back(p.size);
            renderData.rotations.push_back(p.rot);
            renderData.count++;
        }
    }
};

struct ParticleSystem::Impl {
    std::vector<Emitter> emitters;
    uint32_t nextId{1};
    Emitter* Find(uint32_t id){ for(auto& e:emitters) if(e.id==id) return &e; return nullptr; }
};

static const ParticleRenderData s_emptyRenderData{};

ParticleSystem::ParticleSystem()  : m_impl(new Impl) {}
ParticleSystem::~ParticleSystem() { Shutdown(); delete m_impl; }
void ParticleSystem::Init()     {}
void ParticleSystem::Shutdown() { m_impl->emitters.clear(); }

uint32_t ParticleSystem::CreateEmitter(const EmitterDesc& desc)
{
    Emitter e; e.id=m_impl->nextId++; e.desc=desc;
    e.pool.resize(desc.capacity);
    m_impl->emitters.push_back(std::move(e));
    return m_impl->emitters.back().id;
}

void ParticleSystem::DestroyEmitter(uint32_t id) {
    auto& v=m_impl->emitters;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){return e.id==id;}),v.end());
}

bool ParticleSystem::HasEmitter(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void ParticleSystem::SetEmitting(uint32_t id, bool e) { auto* em=m_impl->Find(id); if(em) em->emitting=e; }
void ParticleSystem::Burst(uint32_t id, uint32_t n) { auto* em=m_impl->Find(id); if(em) for(uint32_t i=0;i<n;i++) em->SpawnParticle(); }
void ParticleSystem::StopAndClear(uint32_t id) { auto* em=m_impl->Find(id); if(em){em->emitting=false;for(auto& p:em->pool) p.alive=false;} }
bool ParticleSystem::IsEmitting(uint32_t id) const { const auto* e=m_impl->Find(id); return e&&e->emitting; }

void ParticleSystem::SetPosition(uint32_t id, const float p[3]) { auto* e=m_impl->Find(id); if(e) for(int i=0;i<3;i++) e->pos[i]=p[i]; }
void ParticleSystem::SetOrientation(uint32_t id, const float q[4]) { auto* e=m_impl->Find(id); if(e) for(int i=0;i<4;i++) e->quat[i]=q[i]; }
void ParticleSystem::SetGravity(uint32_t id, const float g[3]) { auto* e=m_impl->Find(id); if(e) for(int i=0;i<3;i++) e->gravity[i]=g[i]; }
void ParticleSystem::SetWindForce(uint32_t id, const float w[3]) { auto* e=m_impl->Find(id); if(e) for(int i=0;i<3;i++) e->wind[i]=w[i]; }
void ParticleSystem::UpdateDesc(uint32_t id, const EmitterDesc& d) { auto* e=m_impl->Find(id); if(e){e->desc=d;e->pool.resize(d.capacity);} }

const ParticleRenderData& ParticleSystem::GetRenderData(uint32_t id) const {
    const auto* e=m_impl->Find(id); return e?e->renderData:s_emptyRenderData;
}

uint32_t ParticleSystem::ActiveCount(uint32_t id) const {
    const auto* e=m_impl->Find(id); return e?e->renderData.count:0;
}
uint32_t ParticleSystem::EmitterCount() const { return (uint32_t)m_impl->emitters.size(); }

void ParticleSystem::SetOnParticleDie(uint32_t id, std::function<void(const float[3])> cb) {
    auto* e=m_impl->Find(id); if(e) e->onDie=cb;
}

void ParticleSystem::Tick(float dt) {
    for (auto& e : m_impl->emitters) e.Tick(dt);
}

} // namespace Engine
