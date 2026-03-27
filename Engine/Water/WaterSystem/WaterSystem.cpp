#include "Engine/Water/WaterSystem/WaterSystem.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Engine {

// ── Gerstner wave evaluation ───────────────────────────────────────────────

static float GerstnerHeight(const WaveDesc& w, float x, float z, float t)
{
    float kx = std::cos(w.dirAngle);
    float kz = std::sin(w.dirAngle);
    float k  = (float)(2.0*M_PI) / w.wavelength;
    float phase = k * (kx*x + kz*z) - w.speed * t;
    return w.amplitude * std::sin(phase);
}

static void GerstnerNormal(const WaveDesc& w, float x, float z, float t,
                            float& nx, float& nz)
{
    float kx = std::cos(w.dirAngle);
    float kz = std::sin(w.dirAngle);
    float k  = (float)(2.0*M_PI) / w.wavelength;
    float phase = k * (kx*x + kz*z) - w.speed * t;
    float c = w.amplitude * k * std::cos(phase);
    nx += -kx * c;
    nz += -kz * c;
}

// ── Impl ────────────────────────────────────────────────────────────────────

struct WaterSystem::Impl {
    std::vector<WaterBodyDesc>    bodies;
    std::vector<WaveDesc>         waves;

    struct BuoyancyEntry {
        BuoyancyBodyDesc desc;
        BuoyancyState    state;
    };
    std::unordered_map<uint32_t, BuoyancyEntry> buoyancy;

    struct ActorEntry {
        uint32_t actorId{0};
        float    position[3]{};
        ActorWaterState state;
    };
    std::unordered_map<uint32_t, ActorEntry> actors;

    float time{0.f};
    WaterEventCb onSwimChanged;

    bool InAnyBody(float x, float y, float z) const {
        for (auto& b : bodies) {
            if (std::abs(x) <= b.extentXZ[0] && std::abs(z) <= b.extentXZ[1])
                if (y <= b.baseHeight) return true;
        }
        return false;
    }

    float HeightAt(float x, float z, float t) const {
        float h = 0.f;
        for (auto& b : bodies) {
            if (std::abs(x) <= b.extentXZ[0] && std::abs(z) <= b.extentXZ[1])
                h = b.baseHeight;
        }
        for (auto& w : waves) h += GerstnerHeight(w, x, z, t);
        return h;
    }
};

// ── WaterSystem ────────────────────────────────────────────────────────────

WaterSystem::WaterSystem()  : m_impl(new Impl) {}
WaterSystem::~WaterSystem() { Shutdown(); delete m_impl; }

void WaterSystem::Init()     {}
void WaterSystem::Shutdown() {}

void WaterSystem::Tick(float dt)
{
    m_impl->time += dt;
    float t = m_impl->time;

    // Buoyancy
    for (auto& [id, entry] : m_impl->buoyancy) {
        auto& bd = entry.desc;
        float hWater = m_impl->HeightAt(bd.centreOfMass[0], bd.centreOfMass[2], t);
        float yPos   = bd.centreOfMass[1];
        float sub    = std::max(0.f, std::min(1.f, (hWater - yPos) / 1.f));
        entry.state.submersionRatio = sub;
        entry.state.inWater = sub > 0.f;

        if (sub > 0.f && bd.applyImpulse) {
            float buoyForce = 1025.f * 9.81f * bd.volume * sub;
            float netForce  = buoyForce - bd.mass * 9.81f;
            float vel[3]    = { 0.f, netForce * dt / bd.mass, 0.f };
            float torque[3] = {};
            bd.applyImpulse(vel, torque);
        }
    }

    // Swimming state
    for (auto& [id, entry] : m_impl->actors) {
        float x = entry.position[0], y = entry.position[1], z = entry.position[2];
        float h = m_impl->HeightAt(x, z, t);
        float depth = h - y;
        SwimState prev = entry.state.swimState;
        SwimState next;
        if (depth < 0.f)       next = SwimState::Dry;
        else if (depth < 0.8f) next = SwimState::Wading;
        else if (depth < 2.0f) next = SwimState::Swimming;
        else                   next = SwimState::Diving;

        entry.state.depth = depth;
        entry.state.swimState = next;
        if (depth > 0.f) entry.state.wetnessLevel = std::min(1.f, entry.state.wetnessLevel + dt*0.5f);
        else             entry.state.wetnessLevel = std::max(0.f, entry.state.wetnessLevel - dt*0.1f);

        if (next != prev && m_impl->onSwimChanged)
            m_impl->onSwimChanged(id, next);
    }
}

void WaterSystem::AddWaterBody(const WaterBodyDesc& desc)
{
    m_impl->bodies.push_back(desc);
}

void WaterSystem::RemoveWaterBody(const std::string& id)
{
    auto& v = m_impl->bodies;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& b){ return b.id==id; }), v.end());
}

bool WaterSystem::IsInWater(float x, float y, float z) const
{
    return m_impl->InAnyBody(x, y, z);
}

void WaterSystem::AddWave(const WaveDesc& wave)  { m_impl->waves.push_back(wave); }
void WaterSystem::ClearWaves()                    { m_impl->waves.clear(); }

float WaterSystem::GetWaterHeight(float x, float z, float time) const
{
    float t = (time < 0.f) ? m_impl->time : time;
    return m_impl->HeightAt(x, z, t);
}

void WaterSystem::GetWaterNormal(float x, float z, float outN[3], float time) const
{
    float t = (time < 0.f) ? m_impl->time : time;
    float nx=0.f, nz=0.f;
    for (auto& w : m_impl->waves) GerstnerNormal(w, x, z, t, nx, nz);
    float ny = 1.f;
    float len = std::sqrt(nx*nx + ny*ny + nz*nz);
    if (len > 1e-6f) { nx/=len; ny/=len; nz/=len; }
    outN[0]=nx; outN[1]=ny; outN[2]=nz;
}

WaterSample WaterSystem::Sample(float x, float y, float z) const
{
    WaterSample s;
    s.height = GetWaterHeight(x, z);
    GetWaterNormal(x, z, s.normal);
    s.inWater = (y <= s.height);
    // flow from first matching body
    for (auto& b : m_impl->bodies) {
        if (std::abs(x) <= b.extentXZ[0] && std::abs(z) <= b.extentXZ[1]) {
            s.flowVelocity[0] = b.flowVelocity[0];
            s.flowVelocity[1] = b.flowVelocity[1];
            s.flowVelocity[2] = b.flowVelocity[2];
            break;
        }
    }
    return s;
}

void WaterSystem::RegisterBuoyancyBody(const BuoyancyBodyDesc& desc)
{
    Impl::BuoyancyEntry e;
    e.desc = desc;
    e.state.entityId = desc.entityId;
    m_impl->buoyancy[desc.entityId] = e;
}

void WaterSystem::UnregisterBuoyancyBody(uint32_t entityId)
{
    m_impl->buoyancy.erase(entityId);
}

BuoyancyState WaterSystem::GetBuoyancyState(uint32_t entityId) const
{
    auto it = m_impl->buoyancy.find(entityId);
    return it != m_impl->buoyancy.end() ? it->second.state : BuoyancyState{};
}

void WaterSystem::SetActorPosition(uint32_t actorId, const float pos[3])
{
    auto& e = m_impl->actors[actorId];
    e.actorId = actorId;
    e.position[0]=pos[0]; e.position[1]=pos[1]; e.position[2]=pos[2];
    e.state.actorId = actorId;
}

SwimState WaterSystem::GetSwimState(uint32_t actorId) const
{
    auto it = m_impl->actors.find(actorId);
    return it != m_impl->actors.end() ? it->second.state.swimState : SwimState::Dry;
}

float WaterSystem::GetWetness(uint32_t actorId) const
{
    auto it = m_impl->actors.find(actorId);
    return it != m_impl->actors.end() ? it->second.state.wetnessLevel : 0.f;
}

void WaterSystem::SetOnSwimStateChanged(WaterEventCb cb) { m_impl->onSwimChanged = cb; }

bool WaterSystem::SaveToJSON(const std::string& path) const
{
    std::ofstream f(path);
    if (!f) return false;
    f << "{\"waterSystem\":true,\"bodies\":" << m_impl->bodies.size()
      << ",\"waves\":" << m_impl->waves.size() << "}\n";
    return true;
}

bool WaterSystem::LoadFromJSON(const std::string& /*path*/) { return true; }

} // namespace Engine
