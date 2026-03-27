#include "Engine/Physics/SurfaceAnalyzer/SurfaceAnalyzer.h"
#include <algorithm>
#include <unordered_map>

namespace Engine {

struct SurfaceAnalyzer::Impl {
    std::unordered_map<std::string, SurfaceMaterial> registry;
    RaycastFn   raycastFn;
    EntityTagFn entityTagFn;

    const SurfaceMaterial* Lookup(const std::string& tag) const {
        auto it = registry.find(tag);
        if (it != registry.end()) return &it->second;
        auto def = registry.find("default");
        return def != registry.end() ? &def->second : nullptr;
    }

    SurfaceResult BuildResult(const SurfaceMaterial* mat, const SurfaceHit& hit,
                               bool didHit) const {
        SurfaceResult r;
        r.hit = didHit;
        if (didHit) {
            r.hitPoint[0]=hit.point[0]; r.hitPoint[1]=hit.point[1]; r.hitPoint[2]=hit.point[2];
            r.hitNormal[0]=hit.normal[0]; r.hitNormal[1]=hit.normal[1]; r.hitNormal[2]=hit.normal[2];
            r.primaryTag = hit.materialTag;
        }
        if (mat) {
            r.friction      = mat->friction;
            r.restitution   = mat->restitution;
            r.stepSound     = mat->stepSound;
            r.impactSound   = mat->impactSound;
            r.stepParticle  = mat->stepParticle;
        }
        return r;
    }
};

SurfaceAnalyzer::SurfaceAnalyzer()  : m_impl(new Impl) {}
SurfaceAnalyzer::~SurfaceAnalyzer() { Shutdown(); delete m_impl; }

void SurfaceAnalyzer::Init()     {}
void SurfaceAnalyzer::Shutdown() {}

void SurfaceAnalyzer::RegisterSurface(const SurfaceMaterial& mat)
{
    m_impl->registry[mat.tag] = mat;
}

void SurfaceAnalyzer::UnregisterSurface(const std::string& tag)
{
    m_impl->registry.erase(tag);
}

bool SurfaceAnalyzer::HasSurface(const std::string& tag) const
{
    return m_impl->registry.count(tag) > 0;
}

const SurfaceMaterial* SurfaceAnalyzer::GetSurface(const std::string& tag) const
{
    return m_impl->Lookup(tag);
}

void SurfaceAnalyzer::SetRaycastFn(RaycastFn fn)   { m_impl->raycastFn   = fn; }
void SurfaceAnalyzer::SetEntityTagFn(EntityTagFn fn){ m_impl->entityTagFn = fn; }

SurfaceResult SurfaceAnalyzer::Analyze(const float origin[3], const float dir[3],
                                        float maxDist) const
{
    if (!m_impl->raycastFn) return {};

    Ray ray;
    ray.origin[0]=origin[0]; ray.origin[1]=origin[1]; ray.origin[2]=origin[2];
    ray.direction[0]=dir[0]; ray.direction[1]=dir[1]; ray.direction[2]=dir[2];

    SurfaceHit hit;
    bool ok = m_impl->raycastFn(ray, hit);
    if (ok && hit.distance > maxDist) ok = false;

    if (ok) {
        // Resolve tag from entity if not already set
        if (hit.materialTag.empty() && hit.entityId && m_impl->entityTagFn)
            hit.materialTag = m_impl->entityTagFn(hit.entityId);
        if (hit.materialTag.empty()) hit.materialTag = "default";
    }

    const SurfaceMaterial* mat = ok ? m_impl->Lookup(hit.materialTag) : nullptr;
    return m_impl->BuildResult(mat, hit, ok);
}

SurfaceResult SurfaceAnalyzer::AnalyzeContact(const float point[3],
                                               uint32_t entityId) const
{
    std::string tag;
    if (m_impl->entityTagFn && entityId) tag = m_impl->entityTagFn(entityId);
    if (tag.empty()) tag = "default";

    SurfaceHit hit;
    hit.point[0]=point[0]; hit.point[1]=point[1]; hit.point[2]=point[2];
    hit.entityId   = entityId;
    hit.materialTag = tag;

    const SurfaceMaterial* mat = m_impl->Lookup(tag);
    return m_impl->BuildResult(mat, hit, true);
}

std::vector<SurfaceResult> SurfaceAnalyzer::BatchAnalyze(
    const std::vector<Probe>& probes) const
{
    std::vector<SurfaceResult> out;
    out.reserve(probes.size());
    for (auto& p : probes) out.push_back(Analyze(p.origin, p.dir, p.maxDist));
    return out;
}

SurfaceResult SurfaceAnalyzer::BlendResults(
    const std::vector<SurfaceBlendWeight>& weights) const
{
    SurfaceResult r;
    float totalW = 0.f;
    for (auto& w : weights) totalW += w.weight;
    if (totalW <= 0.f) return r;

    float bestW = 0.f;
    for (auto& w : weights) {
        if (w.weight > bestW) { bestW=w.weight; r.primaryTag=w.tag; }
        const SurfaceMaterial* mat = m_impl->Lookup(w.tag);
        if (mat) {
            float frac = w.weight / totalW;
            r.friction    += mat->friction    * frac;
            r.restitution += mat->restitution * frac;
        }
        r.blends.push_back(w);
    }
    r.hit = true;

    // Primary sounds from dominant surface
    const SurfaceMaterial* dom = m_impl->Lookup(r.primaryTag);
    if (dom) {
        r.stepSound    = dom->stepSound;
        r.impactSound  = dom->impactSound;
        r.stepParticle = dom->stepParticle;
    }
    return r;
}

} // namespace Engine
