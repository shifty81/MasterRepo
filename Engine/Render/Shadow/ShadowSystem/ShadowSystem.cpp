#include "Engine/Render/Shadow/ShadowSystem/ShadowSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct ShadowLight {
    uint32_t       id{0};
    ShadowLightDesc desc;
    uint32_t       cascadeCount{1};
    std::vector<float> splitDist;
    std::vector<std::vector<float>> matrices; // matrices[cascade][16]
    uint32_t       pcfKernel{1};
    uint32_t       casterCount{0};
};

static void BuildOrthoMatrix(float left, float right, float bottom, float top,
                               float near_, float far_, float out[16])
{
    float rl = right - left, tb = top - bottom, fn = far_ - near_;
    for (int i = 0; i < 16; i++) out[i] = 0.f;
    out[0] = 2.f / rl;
    out[5] = 2.f / tb;
    out[10]= -2.f / fn;
    out[12]= -(right+left)/rl;
    out[13]= -(top+bottom)/tb;
    out[14]= -(far_+near_)/fn;
    out[15]= 1.f;
}

struct ShadowSystem::Impl {
    std::vector<ShadowLight> lights;
    uint32_t nextId{1};
    uint32_t maxLights{8};
    uint32_t defaultRes{1024};
    float    maxShadowDist{500.f};
    CasterCullFn cullFn;

    ShadowLight* Find(uint32_t id) {
        for (auto& l : lights) if (l.id==id) return &l; return nullptr;
    }
    const ShadowLight* Find(uint32_t id) const {
        for (auto& l : lights) if (l.id==id) return &l; return nullptr;
    }
};

ShadowSystem::ShadowSystem()  : m_impl(new Impl) {}
ShadowSystem::~ShadowSystem() { Shutdown(); delete m_impl; }

void ShadowSystem::Init(uint32_t maxLights, uint32_t defaultRes) {
    m_impl->maxLights = maxLights;
    m_impl->defaultRes= defaultRes;
}
void ShadowSystem::Shutdown() { m_impl->lights.clear(); }

uint32_t ShadowSystem::AddLight(const ShadowLightDesc& desc)
{
    if ((uint32_t)m_impl->lights.size() >= m_impl->maxLights) return 0;
    ShadowLight sl;
    sl.id = m_impl->nextId++;
    sl.desc = desc;
    sl.cascadeCount = 1;
    sl.splitDist.push_back(desc.farPlane);
    sl.matrices.push_back(std::vector<float>(16, 0.f));
    m_impl->lights.push_back(std::move(sl));
    return m_impl->lights.back().id;
}

void ShadowSystem::RemoveLight(uint32_t id) {
    auto& v = m_impl->lights;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& l){ return l.id==id; }), v.end());
}

void ShadowSystem::UpdateLight(uint32_t id, const ShadowLightDesc& desc) {
    auto* l = m_impl->Find(id); if (l) l->desc=desc;
}

bool ShadowSystem::HasLight(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

void ShadowSystem::SetCSMCascades(uint32_t id, uint32_t count,
                                    const std::vector<float>& splits)
{
    auto* l = m_impl->Find(id); if (!l) return;
    l->cascadeCount = std::min(count, 4u);
    l->splitDist    = splits;
    l->matrices.resize(l->cascadeCount, std::vector<float>(16, 0.f));
}

uint32_t ShadowSystem::GetCascadeCount(uint32_t id) const {
    const auto* l = m_impl->Find(id); return l?l->cascadeCount:0;
}

void ShadowSystem::SetPCF(uint32_t id, uint32_t kernel) {
    auto* l = m_impl->Find(id); if (l) l->pcfKernel=kernel;
}

void ShadowSystem::SetMaxShadowDistance(float d) { m_impl->maxShadowDist = d; }

void ShadowSystem::Update(const float camPos[3], const float camDir[3],
                           float camFovDeg, float camAspect, float camNear, float camFar)
{
    (void)camFovDeg; (void)camAspect;
    for (auto& sl : m_impl->lights) {
        if (!sl.desc.enabled) continue;
        for (uint32_t c = 0; c < sl.cascadeCount; c++) {
            float splitN = (c==0) ? camNear : sl.splitDist[c-1];
            float splitF = (c < (uint32_t)sl.splitDist.size()) ? sl.splitDist[c] : camFar;
            splitF = std::min(splitF, m_impl->maxShadowDist);
            // Simple orthographic from light direction
            float centre[3];
            for (int i=0;i<3;i++) centre[i] = camPos[i] + camDir[i]*(splitN+splitF)*0.5f;
            float radius = (splitF-splitN)*0.5f + 5.f;
            BuildOrthoMatrix(-radius, radius, -radius, radius,
                              sl.desc.nearPlane, sl.desc.farPlane, sl.matrices[c].data());
        }
    }
}

const float* ShadowSystem::GetLightMatrix(uint32_t id, uint32_t cascade) const {
    const auto* l = m_impl->Find(id);
    if (!l || cascade >= l->matrices.size()) return nullptr;
    return l->matrices[cascade].data();
}

std::vector<uint32_t> ShadowSystem::GetActiveLights() const {
    std::vector<uint32_t> out;
    for (auto& l : m_impl->lights) if (l.desc.enabled) out.push_back(l.id);
    return out;
}

void ShadowSystem::SetCasterCullFn(CasterCullFn fn) { m_impl->cullFn = fn; }

uint32_t ShadowSystem::ShadowCasterCount(uint32_t id) const {
    const auto* l = m_impl->Find(id); return l?l->casterCount:0;
}

} // namespace Engine
