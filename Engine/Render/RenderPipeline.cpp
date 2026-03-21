#include "Engine/Render/RenderPipeline.h"
#include <algorithm>
#include <cmath>

namespace Engine::Render {

// ─────────────────────────────────────────────────────────────────────────────
// LODSystem
// ─────────────────────────────────────────────────────────────────────────────

void LODSystem::RegisterGroup(const LODGroup& group)
{
    auto it = std::find_if(m_groups.begin(), m_groups.end(),
                           [&](const LODGroup& g){ return g.id == group.id; });
    if (it != m_groups.end())
        *it = group;
    else
        m_groups.push_back(group);
}

void LODSystem::UnregisterGroup(const std::string& id)
{
    m_groups.erase(std::remove_if(m_groups.begin(), m_groups.end(),
                                  [&](const LODGroup& g){ return g.id == id; }),
                   m_groups.end());
}

int LODSystem::SelectLevel(const std::string& groupId,
                            float cameraDistanceSq) const
{
    auto it = std::find_if(m_groups.begin(), m_groups.end(),
                           [&](const LODGroup& g){ return g.id == groupId; });
    if (it == m_groups.end() || it->levels.empty()) return 0;

    float dist = std::sqrt(cameraDistanceSq) * m_bias;
    int best = static_cast<int>(it->levels.size()) - 1;
    for (int i = 0; i < (int)it->levels.size(); ++i) {
        if (dist <= it->levels[i].maxDistance) { best = i; break; }
    }
    return best;
}

void LODSystem::SetBias(float bias) { m_bias = (bias > 0.f) ? bias : 1.0f; }
float LODSystem::GetBias() const    { return m_bias; }
size_t LODSystem::GroupCount() const { return m_groups.size(); }

// ─────────────────────────────────────────────────────────────────────────────
// GISystem
// ─────────────────────────────────────────────────────────────────────────────

GISystem::GISystem(const GIConfig& cfg) : m_cfg(cfg) {}

void GISystem::Init()       { m_initialised = true; }
void GISystem::Shutdown()   { m_probes.clear(); m_initialised = false; }
void GISystem::BakeProbes() { /* GPU bake stub */ }
void GISystem::UpdateDynamic()
{
    for (auto& p : m_probes)
        if (p.dynamic) { /* re-capture stub */ }
}

void GISystem::AddProbe(const GIProbe& probe) { m_probes.push_back(probe); }
void GISystem::RemoveProbe(uint32_t texId)
{
    m_probes.erase(std::remove_if(m_probes.begin(), m_probes.end(),
                                  [&](const GIProbe& p){ return p.texId == texId; }),
                   m_probes.end());
}

const std::vector<GIProbe>& GISystem::GetProbes() const { return m_probes; }
void GISystem::SetConfig(const GIConfig& cfg) { m_cfg = cfg; }
const GIConfig& GISystem::GetConfig() const { return m_cfg; }
bool GISystem::IsEnabled() const { return m_cfg.enabled; }
void GISystem::SetEnabled(bool v) { m_cfg.enabled = v; }

// ─────────────────────────────────────────────────────────────────────────────
// ReflectionSystem
// ─────────────────────────────────────────────────────────────────────────────

ReflectionSystem::ReflectionSystem(const ReflectionConfig& cfg) : m_cfg(cfg) {}
void ReflectionSystem::Init()   {}
void ReflectionSystem::Shutdown() {}
void ReflectionSystem::Render(uint32_t, uint32_t, uint32_t) { /* GPU stub */ }
void ReflectionSystem::SetConfig(const ReflectionConfig& cfg) { m_cfg = cfg; }
const ReflectionConfig& ReflectionSystem::GetConfig() const { return m_cfg; }
bool ReflectionSystem::SupportsRayTracing() const
{
    // In a real build: query DXR / VK_KHR_ray_tracing extension availability
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// VolumetricSystem
// ─────────────────────────────────────────────────────────────────────────────

VolumetricSystem::VolumetricSystem(const VolumetricConfig& cfg) : m_cfg(cfg) {}
void VolumetricSystem::Init()
{
    // Allocate froxel texture (cfg.froxelW × froxelH × numSlices × RGBA16F)
    m_froxelTex = 0; // GPU handle stub
}
void VolumetricSystem::Shutdown() { m_froxelTex = 0; }
void VolumetricSystem::Render()   { /* inject scattering into light pass — GPU stub */ }
void VolumetricSystem::SetConfig(const VolumetricConfig& cfg) { m_cfg = cfg; }
const VolumetricConfig& VolumetricSystem::GetConfig() const { return m_cfg; }

// ─────────────────────────────────────────────────────────────────────────────
// ParticleSystem
// ─────────────────────────────────────────────────────────────────────────────

ParticleSystem::ParticleSystem()  {}
ParticleSystem::~ParticleSystem() {}

uint32_t ParticleSystem::CreateEmitter(const ParticleEmitterDesc& desc)
{
    EmitterState state;
    state.desc = desc;
    state.id   = m_nextId++;
    state.particles.resize(desc.maxParticles);
    m_emitters.push_back(std::move(state));
    return m_emitters.back().id;
}

void ParticleSystem::DestroyEmitter(uint32_t id)
{
    m_emitters.erase(std::remove_if(m_emitters.begin(), m_emitters.end(),
                                    [id](const EmitterState& s){ return s.id == id; }),
                     m_emitters.end());
}

ParticleEmitterDesc* ParticleSystem::GetEmitter(uint32_t id)
{
    for (auto& s : m_emitters)
        if (s.id == id) return &s.desc;
    return nullptr;
}

void ParticleSystem::Update(float dt)
{
    for (auto& emitter : m_emitters) {
        const auto& d = emitter.desc;

        // Emission
        emitter.emitAccum += d.emitRate * dt;
        int toEmit = (int)emitter.emitAccum;
        emitter.emitAccum -= toEmit;

        for (auto& p : emitter.particles) {
            if (!p.alive && toEmit > 0) {
                // Spawn
                p.alive    = true;
                p.age      = 0.f;
                p.life     = d.lifetime;
                p.position = d.position;
                p.velocity = {d.velocity[0], d.velocity[1], d.velocity[2]};
                p.color    = d.startColor;
                p.size     = d.startSize;
                p.alpha    = d.startAlpha;
                --toEmit;
                continue;
            }
            if (!p.alive) continue;

            // Integrate
            p.age += dt;
            if (p.age >= p.life) { p.alive = false; continue; }

            float t = p.age / p.life; // 0..1

            p.velocity[0] += m_globalGravity[0] * dt;
            p.velocity[1] += m_globalGravity[1] * dt;
            p.velocity[2] += m_globalGravity[2] * dt;

            p.position[0] += p.velocity[0] * dt;
            p.position[1] += p.velocity[1] * dt;
            p.position[2] += p.velocity[2] * dt;

            // Interpolate size / alpha / colour
            p.size  = d.startSize  + (d.endSize  - d.startSize)  * t;
            p.alpha = d.startAlpha + (d.endAlpha - d.startAlpha) * t;
            for (int i = 0; i < 4; ++i)
                p.color[i] = d.startColor[i] + (d.endColor[i] - d.startColor[i]) * t;
        }
    }
}

void ParticleSystem::Render()
{
    // Submit sorted billboard quads to the renderer (GPU stub).
    // In a real implementation: sort back-to-front, fill a vertex buffer,
    // bind the emitter material, call Renderer::Submit().
}

size_t ParticleSystem::TotalAlive() const
{
    size_t n = 0;
    for (const auto& e : m_emitters)
        for (const auto& p : e.particles)
            if (p.alive) ++n;
    return n;
}

void ParticleSystem::SetGlobalGravity(const std::array<float,3>& g)
{
    m_globalGravity = g;
}

// ─────────────────────────────────────────────────────────────────────────────
// RenderPipeline
// ─────────────────────────────────────────────────────────────────────────────

RenderPipeline::RenderPipeline(const RenderPipelineConfig& cfg)
    : m_cfg(cfg)
    , m_gi(cfg.gi)
    , m_reflections(cfg.reflections)
    , m_volumetrics(cfg.volumetrics)
{}

RenderPipeline::~RenderPipeline() { Shutdown(); }

void RenderPipeline::Init()
{
    if (m_initialised) return;
    m_gi.Init();
    m_reflections.Init();
    m_volumetrics.Init();
    m_initialised = true;
}

void RenderPipeline::Shutdown()
{
    if (!m_initialised) return;
    m_particles    = ParticleSystem{};
    m_volumetrics.Shutdown();
    m_reflections.Shutdown();
    m_gi.Shutdown();
    m_initialised = false;
}

void RenderPipeline::Update(float dt)
{
    if (m_cfg.enableParticles)
        m_particles.Update(dt);

    if (m_gi.IsEnabled())
        m_gi.UpdateDynamic();
}

void RenderPipeline::BeginFrame() {}

void RenderPipeline::Render()
{
    if (m_cfg.gi.enabled)
        ; // GI binding — GPU stub

    if (m_cfg.reflections.mode != ReflectionMode::None) {
        // ReflectionSystem::Render called by caller with depth/normal textures
    }

    if (m_cfg.volumetrics.enabled)
        m_volumetrics.Render();

    if (m_cfg.enableParticles)
        m_particles.Render();
}

void RenderPipeline::EndFrame() {}

LODSystem&        RenderPipeline::GetLOD()         { return m_lod; }
GISystem&         RenderPipeline::GetGI()           { return m_gi; }
ReflectionSystem& RenderPipeline::GetReflections()  { return m_reflections; }
VolumetricSystem& RenderPipeline::GetVolumetrics()  { return m_volumetrics; }
ParticleSystem&   RenderPipeline::GetParticles()    { return m_particles; }

const RenderPipelineConfig& RenderPipeline::GetConfig() const { return m_cfg; }
void RenderPipeline::SetConfig(const RenderPipelineConfig& cfg)
{
    m_cfg = cfg;
    m_gi.SetConfig(cfg.gi);
    m_reflections.SetConfig(cfg.reflections);
    m_volumetrics.SetConfig(cfg.volumetrics);
}

} // namespace Engine::Render
