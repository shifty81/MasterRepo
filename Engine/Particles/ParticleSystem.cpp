#include "Engine/Particles/ParticleSystem.h"
#include <cmath>
#include <algorithm>

namespace Engine {

// ── ColorGradient ─────────────────────────────────────────────────────────
ParticleColor ColorGradient::Lerp(float t) const {
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return {
        start.r + (end.r - start.r) * t,
        start.g + (end.g - start.g) * t,
        start.b + (end.b - start.b) * t,
        start.a + (end.a - start.a) * t,
    };
}

// ── Helpers ───────────────────────────────────────────────────────────────
static float degToRad(float d) { return d * 3.14159265f / 180.0f; }

float ParticleEmitter::randRange(float lo, float hi, uint32_t& rng) {
    rng = rng * 1664525u + 1013904223u; // LCG
    float t = static_cast<float>(rng >> 9) / static_cast<float>(1u << 23);
    return lo + t * (hi - lo);
}

// ── ParticleEmitter ───────────────────────────────────────────────────────
ParticleEmitter::ParticleEmitter(const EmitterConfig& cfg) : m_cfg(cfg) {
    m_pool.resize(cfg.maxParticles);
}

void ParticleEmitter::Start() {
    m_active    = true;
    m_elapsed   = 0.0f;
    m_accumulator = 0.0f;
    // Burst
    for (uint32_t i = 0; i < m_cfg.burstCount; ++i) spawnParticle();
}

void ParticleEmitter::Stop()  { m_active = false; }

void ParticleEmitter::Reset() {
    for (auto& p : m_pool) p.active = false;
    m_active = false;
    m_elapsed = m_accumulator = 0.0f;
}

size_t ParticleEmitter::AliveCount() const {
    size_t n = 0;
    for (const auto& p : m_pool) if (p.active) ++n;
    return n;
}

void ParticleEmitter::spawnParticle() {
    // Find an inactive slot
    for (auto& p : m_pool) {
        if (p.active) continue;
        float life = randRange(m_cfg.lifetimeMin, m_cfg.lifetimeMax, m_rng);
        float speed = randRange(m_cfg.speedMin, m_cfg.speedMax, m_rng);
        // Cone spread direction
        float theta = degToRad(randRange(-m_cfg.spreadAngle,  m_cfg.spreadAngle, m_rng));
        float phi   = degToRad(randRange(0.0f, 360.0f, m_rng));
        float sinT  = std::sin(theta);
        p.vx = std::cos(phi) * sinT * speed;
        p.vy = std::cos(theta)       * speed;
        p.vz = std::sin(phi) * sinT  * speed;
        p.x  = m_cfg.originX;
        p.y  = m_cfg.originY;
        p.z  = m_cfg.originZ;
        p.life = 1.0f;
        p.totalLife = life;
        p.size  = m_cfg.sizeStart;
        p.color = m_cfg.colorGradient.start;
        p.active = true;
        return;
    }
}

void ParticleEmitter::Update(float dt) {
    // Update existing particles
    for (auto& p : m_pool) {
        if (!p.active) continue;
        p.totalLife -= dt;
        if (p.totalLife <= 0.0f) { p.active = false; continue; }
        // life ratio [0,1]: 1=fresh 0=dying
        p.life = p.totalLife > 0.0f ? p.totalLife / (p.totalLife + dt) : 0.0f;
        // Re-derive life fraction from original lifetime — approximate
        // (simple: decrement then ratio)
        float ageRatio = 1.0f - (p.totalLife / (p.totalLife + dt));
        p.vx += 0.0f;
        p.vy += m_cfg.gravityY * dt;
        p.vz += 0.0f;
        p.vx *= (1.0f - m_cfg.drag * dt);
        p.vy *= (1.0f - m_cfg.drag * dt);
        p.vz *= (1.0f - m_cfg.drag * dt);
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.z  += p.vz * dt;
        p.size  = m_cfg.sizeStart + (m_cfg.sizeEnd - m_cfg.sizeStart) * ageRatio;
        p.color = m_cfg.colorGradient.Lerp(ageRatio);
    }

    // Emit new particles
    if (!m_active) return;
    if (m_cfg.duration > 0.0f && m_elapsed >= m_cfg.duration) {
        if (!m_cfg.loop) { m_active = false; return; }
        m_elapsed = 0.0f;
    }
    m_elapsed += dt;

    if (m_cfg.emitRate > 0.0f) {
        m_accumulator += m_cfg.emitRate * dt;
        while (m_accumulator >= 1.0f) {
            spawnParticle();
            m_accumulator -= 1.0f;
        }
    }
}

// ── ParticleSystem ────────────────────────────────────────────────────────
ParticleSystem::ParticleSystem() = default;
ParticleSystem::~ParticleSystem() = default;

uint32_t ParticleSystem::AddEmitter(const EmitterConfig& cfg,
                                     const std::string& name)
{
    uint32_t id = m_nextId++;
    EmitterSlot slot;
    slot.id      = id;
    slot.emitter = ParticleEmitter(cfg);
    slot.emitter.name = name.empty() ? ("emitter_" + std::to_string(id)) : name;
    m_emitters.push_back(std::move(slot));
    return id;
}

void ParticleSystem::RemoveEmitter(uint32_t id) {
    m_emitters.erase(
        std::remove_if(m_emitters.begin(), m_emitters.end(),
                       [id](const EmitterSlot& s){ return s.id == id; }),
        m_emitters.end());
}

ParticleEmitter* ParticleSystem::GetEmitter(uint32_t id) {
    for (auto& s : m_emitters) if (s.id == id) return &s.emitter;
    return nullptr;
}

size_t ParticleSystem::EmitterCount() const { return m_emitters.size(); }

void ParticleSystem::StartAll() {
    for (auto& s : m_emitters) s.emitter.Start();
}
void ParticleSystem::StopAll() {
    for (auto& s : m_emitters) s.emitter.Stop();
}

void ParticleSystem::Update(float dt) {
    for (auto& s : m_emitters) s.emitter.Update(dt);
    if (m_renderCb) m_renderCb(CollectDrawData());
}

std::vector<ParticleDrawData> ParticleSystem::CollectDrawData() const {
    std::vector<ParticleDrawData> out;
    for (const auto& s : m_emitters) {
        for (const auto& p : s.emitter.Particles()) {
            if (!p.active) continue;
            out.push_back({p.x, p.y, p.z, p.size, p.color});
        }
    }
    return out;
}

void ParticleSystem::SetRenderCallback(ParticleRenderCallback cb) {
    m_renderCb = std::move(cb);
}

} // namespace Engine
