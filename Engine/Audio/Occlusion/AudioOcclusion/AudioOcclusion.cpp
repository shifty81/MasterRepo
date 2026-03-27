#include "Engine/Audio/Occlusion/AudioOcclusion/AudioOcclusion.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct OcclusionSourceData {
    uint32_t id;
    OcclusionSourceDesc desc;
    OcclusionResult current{};
    OcclusionResult target {};
};

struct AudioOcclusion::Impl {
    std::vector<OcclusionSourceData> sources;
    std::unordered_map<uint8_t,OcclusionMaterial> materials;
    float listenerPos[3]{};
    OcclusionRayFn rayFn;
    float baseLpf{20000.f};
    float attackSec {0.1f};
    float releaseSec{0.3f};
    uint32_t nextId{1};

    OcclusionSourceData* Find(uint32_t id){
        for(auto& s:sources) if(s.id==id) return &s; return nullptr;
    }
    const OcclusionSourceData* Find(uint32_t id) const {
        for(auto& s:sources) if(s.id==id) return &s; return nullptr;
    }
};

AudioOcclusion::AudioOcclusion()  : m_impl(new Impl){}
AudioOcclusion::~AudioOcclusion() { Shutdown(); delete m_impl; }
void AudioOcclusion::Init()     {}
void AudioOcclusion::Shutdown() { m_impl->sources.clear(); }

void AudioOcclusion::SetRayFn(OcclusionRayFn fn)      { m_impl->rayFn=fn; }
void AudioOcclusion::SetBaseLpf(float hz)              { m_impl->baseLpf=hz; }
void AudioOcclusion::SetSmoothTime(float a, float r)   { m_impl->attackSec=a; m_impl->releaseSec=r; }
void AudioOcclusion::SetListenerPos(const float p[3], uint32_t){
    for(int i=0;i<3;i++) m_impl->listenerPos[i]=p[i];
}

void AudioOcclusion::RegisterMaterial(uint8_t id, const OcclusionMaterial& mat){
    m_impl->materials[id]=mat;
}

uint32_t AudioOcclusion::AddSource(const OcclusionSourceDesc& d){
    OcclusionSourceData s; s.id=m_impl->nextId++; s.desc=d;
    m_impl->sources.push_back(s); return m_impl->sources.back().id;
}
void AudioOcclusion::RemoveSource(uint32_t id){
    auto& v=m_impl->sources;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& s){return s.id==id;}),v.end());
}
void AudioOcclusion::UpdatePos(uint32_t id, const float p[3]){
    auto* s=m_impl->Find(id); if(s) for(int i=0;i<3;i++) s->desc.worldPos[i]=p[i];
}
bool AudioOcclusion::HasSource(uint32_t id) const { return m_impl->Find(id)!=nullptr; }

OcclusionResult AudioOcclusion::GetResult(uint32_t id) const {
    const auto* s=m_impl->Find(id); return s?s->current:OcclusionResult{};
}

std::vector<uint32_t> AudioOcclusion::GetAll() const {
    std::vector<uint32_t> out; for(auto& s:m_impl->sources) out.push_back(s.id); return out;
}

void AudioOcclusion::Tick(float dt)
{
    for(auto& src : m_impl->sources){
        OcclusionResult& tgt=src.target;
        tgt.occlusionFactor=0.f; tgt.lpfCutoff=m_impl->baseLpf; tgt.reverbSend=0.f;
        if(m_impl->rayFn){
            auto hit=m_impl->rayFn(m_impl->listenerPos, src.desc.worldPos);
            if(hit.hit){
                auto it=m_impl->materials.find(hit.materialId);
                float dbLoss=-6.f; float lpfScale=0.5f;
                if(it!=m_impl->materials.end()){dbLoss=it->second.dbLoss; lpfScale=it->second.lpfCutoffScale;}
                // Convert dB loss to linear occlusion factor
                tgt.occlusionFactor=std::min(1.f, std::pow(10.f, -std::abs(dbLoss)/20.f));
                tgt.occlusionFactor=1.f-tgt.occlusionFactor;
                tgt.lpfCutoff=m_impl->baseLpf*lpfScale;
                tgt.reverbSend=std::min(1.f, hit.distance/src.desc.maxRange*0.7f);
            }
        }
        // Smooth
        float& cf=src.current.occlusionFactor;
        float speed=(tgt.occlusionFactor>cf)?1.f/std::max(0.001f,m_impl->attackSec)
                                            :1.f/std::max(0.001f,m_impl->releaseSec);
        cf=cf+(tgt.occlusionFactor-cf)*std::min(1.f,speed*dt);
        src.current.lpfCutoff  =m_impl->baseLpf*(1.f-cf)+tgt.lpfCutoff*cf;
        src.current.reverbSend =tgt.reverbSend*cf;
    }
}

} // namespace Engine
