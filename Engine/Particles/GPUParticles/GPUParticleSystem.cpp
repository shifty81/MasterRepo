#include "Engine/Particles/GPUParticles/GPUParticleSystem.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static uint64_t NowMs(){
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

// ── CPU-side particle (for non-GPU platforms / unit tests) ────────────────────

struct Particle {
    float    pos[3]{};
    float    vel[3]{};
    float    age{0.f};
    float    lifetime{1.f};
    float    size{0.1f};
    uint32_t colour{0xFFFFFFFFu};
    bool     alive{false};
};

struct EmitterState {
    uint32_t        id{0};
    GPUEmitterDesc  desc;
    float           elapsed{0.f};
    float           spawnAccum{0.f};
    std::vector<Particle> particles;
    bool            active{true};
};

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct GPUParticleSystem::Impl {
    std::unordered_map<uint32_t, EmitterState>  emitters;
    std::unordered_map<uint32_t, ParticleForce> forces;
    uint32_t nextEmitterId{1}, nextForceId{1};
    uint32_t maxParticles{500000};
    bool     paused{false};

    std::function<void(uint32_t)> onFinished;

    static float Rand01() {
        return (float)rand() / (float)RAND_MAX;
    }

    static float RandRange(FloatRange r) {
        return r.min + Rand01() * (r.max - r.min);
    }

    void SpawnParticles(EmitterState& em, float dt) {
        float rate = (float)em.desc.spawnRate * dt;
        em.spawnAccum += rate;
        while (em.spawnAccum >= 1.f) {
            em.spawnAccum -= 1.f;
            // Find a dead slot or push new
            Particle* slot = nullptr;
            for (auto& p : em.particles) if (!p.alive) { slot = &p; break; }
            if (!slot) {
                if (em.particles.size() >= 10000) break; // per-emitter cap
                em.particles.push_back({});
                slot = &em.particles.back();
            }
            slot->alive    = true;
            slot->age      = 0.f;
            slot->lifetime = RandRange(em.desc.lifetime);
            float speed    = RandRange(em.desc.startSpeed);
            slot->size     = RandRange(em.desc.startSize);
            slot->colour   = em.desc.colourGradient.empty() ? 0xFFFFFFFFu
                             : em.desc.colourGradient.front().colour;
            // Spawn position = emitter position + cone spread
            float coneRad = em.desc.coneAngleDeg * 3.14159f / 180.f;
            float angle   = Rand01() * 6.28318f;
            float spread  = Rand01() * coneRad;
            slot->vel[0]  = em.desc.direction[0] + std::sin(spread)*std::cos(angle)*speed;
            slot->vel[1]  = em.desc.direction[1]*speed + std::cos(spread)*speed;
            slot->vel[2]  = em.desc.direction[2] + std::sin(spread)*std::sin(angle)*speed;
            for(int i=0;i<3;++i) slot->pos[i] = em.desc.position[i];
        }
    }

    void ApplyForces(Particle& p, float dt) {
        for (auto& [id, f] : forces) {
            if (!f.enabled) continue;
            switch (f.type) {
                case ParticleForceType::Gravity:
                    p.vel[1] -= f.strength * dt;
                    break;
                case ParticleForceType::Wind:
                    for(int i=0;i<3;++i) p.vel[i] += f.direction[i] * f.strength * dt;
                    break;
                case ParticleForceType::Attractor: {
                    float dx=f.position[0]-p.pos[0],dy=f.position[1]-p.pos[1],dz=f.position[2]-p.pos[2];
                    float d=std::sqrt(dx*dx+dy*dy+dz*dz)+0.01f;
                    if(d<f.radius){ float s=f.strength/d; p.vel[0]+=dx*s*dt; p.vel[1]+=dy*s*dt; p.vel[2]+=dz*s*dt; }
                    break;
                }
                default: break;
            }
        }
    }
};

GPUParticleSystem::GPUParticleSystem() : m_impl(new Impl()) {}
GPUParticleSystem::~GPUParticleSystem() { delete m_impl; }
void GPUParticleSystem::Init()     {}
void GPUParticleSystem::Shutdown() { ClearAll(); }

uint32_t GPUParticleSystem::CreateEmitter(const GPUEmitterDesc& desc) {
    uint32_t id=m_impl->nextEmitterId++;
    auto& em=m_impl->emitters[id];
    em.id=id; em.desc=desc;
    return id;
}

void GPUParticleSystem::DestroyEmitter(uint32_t id) { m_impl->emitters.erase(id); }

void GPUParticleSystem::SetEmitterPosition(uint32_t id, const float p[3]) {
    auto it=m_impl->emitters.find(id); if(it!=m_impl->emitters.end()) for(int i=0;i<3;++i) it->second.desc.position[i]=p[i];
}
void GPUParticleSystem::SetEmitterDirection(uint32_t id, const float d[3]) {
    auto it=m_impl->emitters.find(id); if(it!=m_impl->emitters.end()) for(int i=0;i<3;++i) it->second.desc.direction[i]=d[i];
}
void GPUParticleSystem::SetEmitterActive(uint32_t id, bool a) {
    auto it=m_impl->emitters.find(id); if(it!=m_impl->emitters.end()) it->second.active=a;
}

GPUEmitterState GPUParticleSystem::GetEmitterState(uint32_t id) const {
    auto it=m_impl->emitters.find(id); if(it==m_impl->emitters.end()) return {};
    GPUEmitterState s; s.id=id; s.desc=it->second.desc; s.elapsedSeconds=it->second.elapsed;
    s.liveParticleCount=0; for(auto& p:it->second.particles) if(p.alive) ++s.liveParticleCount;
    s.active=it->second.active;
    return s;
}

std::vector<uint32_t> GPUParticleSystem::ActiveEmitterIds() const {
    std::vector<uint32_t> out; for(auto&[id,e]:m_impl->emitters) if(e.active) out.push_back(id); return out;
}

uint32_t GPUParticleSystem::AddForce(const ParticleForce& f) {
    uint32_t id=m_impl->nextForceId++;
    ParticleForce pf=f; pf.id=id;
    m_impl->forces[id]=pf; return id;
}
void GPUParticleSystem::RemoveForce(uint32_t id) { m_impl->forces.erase(id); }
void GPUParticleSystem::SetForceEnabled(uint32_t id, bool e) {
    auto it=m_impl->forces.find(id); if(it!=m_impl->forces.end()) it->second.enabled=e;
}

void GPUParticleSystem::Update(float dt) {
    if(m_impl->paused) return;
    for(auto&[eid,em]:m_impl->emitters){
        if(!em.active) continue;
        em.elapsed+=dt;
        if(em.desc.duration>=0.f&&em.elapsed>em.desc.duration){
            if(!em.desc.loop){ em.active=false; if(m_impl->onFinished) m_impl->onFinished(eid); continue; }
            em.elapsed=std::fmod(em.elapsed,em.desc.duration);
        }
        m_impl->SpawnParticles(em,dt);
        for(auto& p:em.particles){
            if(!p.alive) continue;
            p.age+=dt;
            if(p.age>=p.lifetime){ p.alive=false; continue; }
            m_impl->ApplyForces(p,dt);
            for(int i=0;i<3;++i) p.pos[i]+=p.vel[i]*dt;
        }
    }
}

void GPUParticleSystem::Render() { /* Atlas/OpenGL draw calls go here */ }

uint32_t GPUParticleSystem::TotalLiveParticles() const {
    uint32_t n=0;
    for(auto&[id,em]:m_impl->emitters) for(auto& p:em.particles) if(p.alive) ++n;
    return n;
}
uint32_t GPUParticleSystem::EmitterCount() const { return (uint32_t)m_impl->emitters.size(); }

void GPUParticleSystem::PauseAll()  { m_impl->paused=true; }
void GPUParticleSystem::ResumeAll() { m_impl->paused=false; }
void GPUParticleSystem::ClearAll()  { m_impl->emitters.clear(); }
void GPUParticleSystem::SetMaxParticles(uint32_t m) { m_impl->maxParticles=m; }

void GPUParticleSystem::OnEmitterFinished(std::function<void(uint32_t)> cb) {
    m_impl->onFinished=std::move(cb);
}

} // namespace Engine
