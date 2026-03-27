#include "Engine/Camera/CameraShake/CameraShakeSystem.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace Engine {

// ── Tiny Perlin-like smooth noise (value noise) ─────────────────────────────
static float SmoothNoise(float t)
{
    int i = (int)t;
    float f = t - (float)i;
    float u = f*f*(3.f - 2.f*f); // smoothstep
    // Hash based on i
    auto h = [](int n)->float{
        n = (n<<13)^n;
        return 1.f - (float)((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.f;
    };
    return h(i)*(1.f-u) + h(i+1)*u;
}

struct ShakeInstance {
    uint32_t    id{0};
    std::string presetName;
    float       trauma{0.f};       ///< current trauma [0,1]
    float       elapsed{0.f};
    float       fadeInTime{0.f};
    float       fadeInElapsed{0.f};
    bool        alive{true};
};

struct CameraShakeSystem::Impl {
    std::unordered_map<std::string, ShakePreset> presets;
    std::vector<ShakeInstance>                   shakes;
    uint32_t nextId{1};
    float    globalScale{1.f};
    float    noiseOffset{0.f};   ///< advances with time for variety
    ShakeEndCb onEnd;

    const ShakePreset* Get(const std::string& name) const {
        auto it = presets.find(name);
        return it != presets.end() ? &it->second : nullptr;
    }
};

CameraShakeSystem::CameraShakeSystem()  : m_impl(new Impl) {}
CameraShakeSystem::~CameraShakeSystem() { Shutdown(); delete m_impl; }

void CameraShakeSystem::Init()     {}
void CameraShakeSystem::Shutdown() { m_impl->shakes.clear(); }

void CameraShakeSystem::Tick(float dt)
{
    m_impl->noiseOffset += dt * 17.f; // arbitrary drift

    for (auto& sh : m_impl->shakes) {
        if (!sh.alive) continue;
        sh.elapsed += dt;

        // Fade in
        if (sh.fadeInTime > 0.f) {
            sh.fadeInElapsed += dt;
            float fadeFrac = std::min(1.f, sh.fadeInElapsed / sh.fadeInTime);
            sh.trauma *= fadeFrac;
        }

        // Decay
        const auto* preset = m_impl->Get(sh.presetName);
        if (preset) sh.trauma -= preset->decayRate * dt;
        sh.trauma = std::max(0.f, sh.trauma);

        if (sh.trauma <= 0.001f) {
            sh.alive = false;
            if (m_impl->onEnd) m_impl->onEnd(sh.id);
        }
    }

    // Remove dead shakes
    auto& v = m_impl->shakes;
    v.erase(std::remove_if(v.begin(),v.end(),[](auto& s){ return !s.alive; }), v.end());
}

void CameraShakeSystem::RegisterPreset(const ShakePreset& preset)
{
    m_impl->presets[preset.name] = preset;
}

void CameraShakeSystem::UnregisterPreset(const std::string& name)
{
    m_impl->presets.erase(name);
}

uint32_t CameraShakeSystem::AddTrauma(float amount, const std::string& presetName)
{
    ShakeInstance sh;
    sh.id          = m_impl->nextId++;
    sh.presetName  = presetName;
    sh.trauma      = std::min(1.f, amount);
    const auto* pr = m_impl->Get(presetName);
    if (pr) sh.fadeInTime = pr->fadeInTime;
    sh.alive = true;
    m_impl->shakes.push_back(sh);
    return sh.id;
}

uint32_t CameraShakeSystem::AddTraumaAtPosition(float amount,
                                                  const std::string& presetName,
                                                  const float worldPos[3],
                                                  const float cameraPos[3],
                                                  float maxRadius)
{
    float dx = worldPos[0]-cameraPos[0];
    float dy = worldPos[1]-cameraPos[1];
    float dz = worldPos[2]-cameraPos[2];
    float dist = std::sqrt(dx*dx+dy*dy+dz*dz);
    float scaled = (maxRadius <= 0.f) ? amount : amount * std::max(0.f, 1.f - dist/maxRadius);
    return AddTrauma(scaled, presetName);
}

void CameraShakeSystem::StopShake(uint32_t shakeId)
{
    for (auto& sh : m_impl->shakes)
        if (sh.id == shakeId) { sh.alive = false; break; }
}

void CameraShakeSystem::StopAll()
{
    m_impl->shakes.clear();
}

void CameraShakeSystem::SetGlobalScale(float scale) { m_impl->globalScale = scale; }
float CameraShakeSystem::GetGlobalScale() const     { return m_impl->globalScale; }

ShakeOffset CameraShakeSystem::GetOffset() const
{
    ShakeOffset out;
    float totalTrauma = 0.f;
    float px=0,py=0,pz=0, rx=0,ry=0,rz=0;

    for (auto& sh : m_impl->shakes) {
        if (!sh.alive) continue;
        const auto* pr = m_impl->Get(sh.presetName);
        if (!pr) continue;

        float t2 = sh.trauma * sh.trauma; // shake = trauma²
        float seed = m_impl->noiseOffset + (float)sh.id * 123.f;
        float ns = sh.elapsed * pr->frequency;

        if (pr->affectsPosition) {
            float pa = pr->maxPositionAmplitude * t2 * m_impl->globalScale;
            px += SmoothNoise(ns       ) * pa + pr->directionBias[0] * t2 * pa;
            py += SmoothNoise(ns + 31.7f) * pa + pr->directionBias[1] * t2 * pa;
            pz += SmoothNoise(ns + 67.3f) * pa + pr->directionBias[2] * t2 * pa;
        }
        if (pr->affectsRotation) {
            float ra = pr->maxRotationAmplitude * t2 * m_impl->globalScale;
            rx += SmoothNoise(ns + seed       ) * ra;
            ry += SmoothNoise(ns + seed + 41.f) * ra;
            rz += SmoothNoise(ns + seed + 83.f) * ra;
        }
        totalTrauma = std::max(totalTrauma, sh.trauma);
    }

    out.position[0]=px; out.position[1]=py; out.position[2]=pz;
    out.rotation[0]=rx; out.rotation[1]=ry; out.rotation[2]=rz;
    out.traumaLevel = totalTrauma;
    return out;
}

float CameraShakeSystem::GetCurrentTrauma() const
{
    float t = 0.f;
    for (auto& sh : m_impl->shakes) if (sh.alive) t = std::max(t, sh.trauma);
    return t;
}

void CameraShakeSystem::SetOnShakeEnd(ShakeEndCb cb) { m_impl->onEnd = cb; }

} // namespace Engine
