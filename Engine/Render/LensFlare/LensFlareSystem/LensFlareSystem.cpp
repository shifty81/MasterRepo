#include "Engine/Render/LensFlare/LensFlareSystem/LensFlareSystem.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace Engine {

struct FlareTemplate { uint32_t id; std::string name; std::vector<FlareElement> elements; };
struct FlareSource {
    uint32_t id;
    FlareSourceDesc desc;
    float visibility{1.f};  ///< current occlusion fade
};

struct LensFlareSystem::Impl {
    std::vector<FlareTemplate> templates;
    std::vector<FlareSource>   sources;
    uint32_t nextTemplId{1};
    uint32_t nextSrcId  {1};
    LensFlareOcclusionFn occlusionFn;

    FlareTemplate* FindTmpl(uint32_t id){
        for(auto& t:templates) if(t.id==id) return &t; return nullptr;
    }
    FlareSource* FindSrc(uint32_t id){
        for(auto& s:sources) if(s.id==id) return &s; return nullptr;
    }
    const FlareSource* FindSrc(uint32_t id) const {
        for(auto& s:sources) if(s.id==id) return &s; return nullptr;
    }
};

LensFlareSystem::LensFlareSystem()  : m_impl(new Impl){}
LensFlareSystem::~LensFlareSystem() { Shutdown(); delete m_impl; }
void LensFlareSystem::Init()     {}
void LensFlareSystem::Shutdown() { m_impl->templates.clear(); m_impl->sources.clear(); }

void LensFlareSystem::SetOcclusionFn(LensFlareOcclusionFn fn){ m_impl->occlusionFn=fn; }

uint32_t LensFlareSystem::CreateTemplate(const std::string& name){
    FlareTemplate t; t.id=m_impl->nextTemplId++; t.name=name;
    m_impl->templates.push_back(t); return m_impl->templates.back().id;
}
void LensFlareSystem::AddElement(uint32_t tid, const FlareElement& e){
    auto* t=m_impl->FindTmpl(tid); if(t) t->elements.push_back(e);
}
void LensFlareSystem::ClearElements(uint32_t tid){
    auto* t=m_impl->FindTmpl(tid); if(t) t->elements.clear();
}

uint32_t LensFlareSystem::AddSource(const FlareSourceDesc& d){
    FlareSource s; s.id=m_impl->nextSrcId++; s.desc=d; s.visibility=1.f;
    m_impl->sources.push_back(s); return m_impl->sources.back().id;
}
void LensFlareSystem::RemoveSource(uint32_t id){
    auto& v=m_impl->sources;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& s){return s.id==id;}),v.end());
}
void LensFlareSystem::SetIntensity(uint32_t id, float i){
    auto* s=m_impl->FindSrc(id); if(s) s->desc.intensity=i;
}
bool LensFlareSystem::HasSource(uint32_t id) const { return m_impl->FindSrc(id)!=nullptr; }

std::vector<ActiveFlareElement> LensFlareSystem::GetActiveElements(
    const float viewPos[3], const float viewDir[3], float fovY, float aspect) const
{
    std::vector<ActiveFlareElement> out;
    for(auto& src : m_impl->sources){
        if(src.visibility<=0.01f) continue;
        auto* tmpl=m_impl->FindTmpl(src.desc.templateId);
        if(!tmpl) continue;
        // Compute screen position of source
        float toSrc[3];
        for(int i=0;i<3;i++) toSrc[i]=src.desc.worldPos[i]-viewPos[i];
        float dot=toSrc[0]*viewDir[0]+toSrc[1]*viewDir[1]+toSrc[2]*viewDir[2];
        if(dot<=0.f) continue; // behind camera
        float tanH=std::tan(fovY*0.5f*3.14159f/180.f);
        float tanV=tanH/aspect;
        float nx=toSrc[0]/(dot*tanH);
        float ny=toSrc[1]/(dot*tanV);
        // Flare axis: source → screen centre (0,0)
        float axX=-nx, axY=-ny;
        for(auto& e:tmpl->elements){
            ActiveFlareElement ae;
            ae.type=e.type;
            ae.screenPos[0]=nx+axX*e.offset;
            ae.screenPos[1]=ny+axY*e.offset;
            ae.size=e.size*src.desc.intensity;
            for(int i=0;i<4;i++) ae.colour[i]=e.colour[i]*src.desc.colour[i<3?i:2];
            ae.rotation=e.rotation;
            ae.visibility=src.visibility;
            out.push_back(ae);
        }
    }
    return out;
}

std::vector<uint32_t> LensFlareSystem::GetAllSources() const {
    std::vector<uint32_t> out; for(auto& s:m_impl->sources) out.push_back(s.id); return out;
}

void LensFlareSystem::Tick(float dt)
{
    for(auto& src : m_impl->sources){
        float target=1.f;
        if(m_impl->occlusionFn) target=m_impl->occlusionFn(src.desc.worldPos);
        // Smooth fade
        float speed=4.f*dt;
        if(target>src.visibility) src.visibility=std::min(1.f, src.visibility+speed);
        else                       src.visibility=std::max(0.f, src.visibility-speed);
    }
}

} // namespace Engine
