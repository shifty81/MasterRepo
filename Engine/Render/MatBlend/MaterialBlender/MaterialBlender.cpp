#include "Engine/Render/MatBlend/MaterialBlender/MaterialBlender.h"
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace Engine {

static MatRGBA BlendTwo(MatRGBA base, MatRGBA layer, MatBlendMode mode, float mask) {
    auto Clamp=[](float v){ return std::max(0.f,std::min(1.f,v)); };
    float w=Clamp(mask);
    MatRGBA out;
    switch(mode){
        case MatBlendMode::Normal:
            out.r=base.r*(1-w)+layer.r*w;
            out.g=base.g*(1-w)+layer.g*w;
            out.b=base.b*(1-w)+layer.b*w;
            out.a=base.a*(1-w)+layer.a*w;
            break;
        case MatBlendMode::Multiply:
            out.r=base.r*layer.r*w+(base.r*(1-w));
            out.g=base.g*layer.g*w+(base.g*(1-w));
            out.b=base.b*layer.b*w+(base.b*(1-w));
            out.a=base.a; break;
        case MatBlendMode::Screen:
            out.r=1-(1-base.r)*(1-layer.r)*w+(base.r*(1-w));
            out.g=1-(1-base.g)*(1-layer.g)*w+(base.g*(1-w));
            out.b=1-(1-base.b)*(1-layer.b)*w+(base.b*(1-w));
            out.a=base.a; break;
        case MatBlendMode::Overlay:{
            auto ov=[&](float b,float l)->float{
                return b<0.5f?2*b*l:1-2*(1-b)*(1-l);
            };
            out.r=base.r*(1-w)+ov(base.r,layer.r)*w;
            out.g=base.g*(1-w)+ov(base.g,layer.g)*w;
            out.b=base.b*(1-w)+ov(base.b,layer.b)*w;
            out.a=base.a; break;
        }
        case MatBlendMode::Add:
            out.r=Clamp(base.r+layer.r*w);
            out.g=Clamp(base.g+layer.g*w);
            out.b=Clamp(base.b+layer.b*w);
            out.a=base.a; break;
        default: // Lerp
            out.r=base.r+(layer.r-base.r)*w;
            out.g=base.g+(layer.g-base.g)*w;
            out.b=base.b+(layer.b-base.b)*w;
            out.a=base.a+(layer.a-base.a)*w;
            break;
    }
    return out;
}

struct MatLayer {
    std::string   materialKey;
    MatRGBA       colour{1,1,1,1};
    float         weight{1.f};
    MatBlendMode  mode{MatBlendMode::Lerp};
    bool          enabled{true};
    std::function<float(float,float)> maskSampler;
};

struct MatBlendSet {
    uint32_t            id;
    std::string         name;
    std::vector<MatLayer> layers;
};

struct MaterialBlender::Impl {
    uint32_t nextSetId{1};
    std::vector<MatBlendSet> sets;
    MatBlendSet* Find(uint32_t id){ for(auto& s:sets) if(s.id==id) return &s; return nullptr; }
    const MatBlendSet* Find(uint32_t id) const { for(auto& s:sets) if(s.id==id) return &s; return nullptr; }
};

MaterialBlender::MaterialBlender(): m_impl(new Impl){}
MaterialBlender::~MaterialBlender(){ Shutdown(); delete m_impl; }
void MaterialBlender::Init(){}
void MaterialBlender::Shutdown(){ m_impl->sets.clear(); }
void MaterialBlender::Reset(){ m_impl->sets.clear(); m_impl->nextSetId=1; }

uint32_t MaterialBlender::CreateBlendSet(const std::string& name){
    MatBlendSet s; s.id=m_impl->nextSetId++; s.name=name;
    m_impl->sets.push_back(s); return s.id;
}
void MaterialBlender::DestroyBlendSet(uint32_t id){
    m_impl->sets.erase(std::remove_if(m_impl->sets.begin(),m_impl->sets.end(),
        [id](const MatBlendSet& s){return s.id==id;}),m_impl->sets.end());
}

uint32_t MaterialBlender::AddLayer(uint32_t setId, const std::string& key, float weight, MatBlendMode mode){
    auto* s=m_impl->Find(setId); if(!s) return 0;
    MatLayer l; l.materialKey=key; l.weight=weight; l.mode=mode;
    s->layers.push_back(l);
    return (uint32_t)s->layers.size()-1;
}
void MaterialBlender::RemoveLayer(uint32_t setId, uint32_t idx){
    auto* s=m_impl->Find(setId);
    if(s&&idx<s->layers.size()) s->layers.erase(s->layers.begin()+idx);
}
void MaterialBlender::SetLayerWeight(uint32_t setId, uint32_t idx, float w){
    auto* s=m_impl->Find(setId); if(s&&idx<s->layers.size()) s->layers[idx].weight=w;
}
void MaterialBlender::SetLayerEnabled(uint32_t setId, uint32_t idx, bool en){
    auto* s=m_impl->Find(setId); if(s&&idx<s->layers.size()) s->layers[idx].enabled=en;
}
void MaterialBlender::SetBlendMode(uint32_t setId, uint32_t idx, MatBlendMode mode){
    auto* s=m_impl->Find(setId); if(s&&idx<s->layers.size()) s->layers[idx].mode=mode;
}
void MaterialBlender::SetMaterialColour(uint32_t setId, uint32_t idx, MatRGBA c){
    auto* s=m_impl->Find(setId); if(s&&idx<s->layers.size()) s->layers[idx].colour=c;
}
void MaterialBlender::SetMaskSampler(uint32_t setId, uint32_t idx,
                                      std::function<float(float,float)> fn){
    auto* s=m_impl->Find(setId); if(s&&idx<s->layers.size()) s->layers[idx].maskSampler=fn;
}

MatRGBA MaterialBlender::Sample(uint32_t setId, float u, float v) const {
    auto* s=m_impl->Find(setId); if(!s||s->layers.empty()) return {0,0,0,1};
    MatRGBA result=s->layers[0].colour;
    for(size_t i=1;i<s->layers.size();i++){
        auto& l=s->layers[i]; if(!l.enabled) continue;
        float mask=l.weight*(l.maskSampler?l.maskSampler(u,v):1.f);
        result=BlendTwo(result,l.colour,l.mode,mask);
    }
    return result;
}

uint32_t MaterialBlender::GetDominantLayer(uint32_t setId, float u, float v) const {
    auto* s=m_impl->Find(setId); if(!s||s->layers.empty()) return 0;
    uint32_t best=0; float bestW=0;
    for(uint32_t i=0;i<(uint32_t)s->layers.size();i++){
        auto& l=s->layers[i]; if(!l.enabled) continue;
        float w=l.weight*(l.maskSampler?l.maskSampler(u,v):1.f);
        if(w>bestW){ bestW=w; best=i; }
    }
    return best;
}

void MaterialBlender::NormalizeWeights(uint32_t setId){
    auto* s=m_impl->Find(setId); if(!s) return;
    float sum=0; for(auto& l:s->layers) if(l.enabled) sum+=l.weight;
    if(sum>0) for(auto& l:s->layers) l.weight/=sum;
}

void MaterialBlender::ExportWeightMap(uint32_t setId, uint32_t w, uint32_t h, float* out) const {
    if(!out) return;
    for(uint32_t y=0;y<h;y++) for(uint32_t x=0;x<w;x++){
        float u=(float)x/std::max(1u,w-1), v=(float)y/std::max(1u,h-1);
        out[y*w+x]=(float)GetDominantLayer(setId,u,v);
    }
}

uint32_t MaterialBlender::GetLayerCount(uint32_t setId) const {
    auto* s=m_impl->Find(setId); return s?(uint32_t)s->layers.size():0;
}

} // namespace Engine
