#include "Engine/Debug/DebugDraw/DebugDraw.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine {

struct Primitive {
    enum class Kind : uint8_t { Line=0, Sphere, Box, Text, Grid, Axes, Cross, Capsule, Arrow, OBB };
    Kind         kind{Kind::Line};
    float        a[3]{},b[3]{};
    float        r{0.5f};
    DebugColour  col{};
    float        lifetime{0.f};
    float        elapsed{0.f};
    std::string  text;
    std::string  group;
};

struct DebugDraw::Impl {
    std::vector<Primitive>                        prims;
    std::unordered_map<std::string,bool>          groupEnabled;
    std::string                                   activeGroup;
    DebugDrawSettings                             settings;

    bool GroupEnabled(const std::string& g) const {
        if(g.empty()) return true;
        auto it=groupEnabled.find(g); return it==groupEnabled.end()||it->second;
    }
    void Add(Primitive p){ if(settings.enabled && GroupEnabled(p.group)) prims.push_back(std::move(p)); }
};

DebugDraw& DebugDraw::Get() { static DebugDraw instance; return instance; }
DebugDraw::DebugDraw() : m_impl(new Impl()) {}
DebugDraw::~DebugDraw() { delete m_impl; }

void DebugDraw::Init(const DebugDrawSettings& s){ m_impl->settings=s; }
void DebugDraw::Shutdown(){ Clear(); }
void DebugDraw::SetEnabled(bool e){ m_impl->settings.enabled=e; }
bool DebugDraw::IsEnabled() const{ return m_impl->settings.enabled; }

void DebugDraw::Line(const float a[3],const float b[3],DebugColour col,float lt){
    Primitive p; p.kind=Primitive::Kind::Line; for(int i=0;i<3;++i){p.a[i]=a[i];p.b[i]=b[i];}
    p.col=col; p.lifetime=lt; p.group=m_impl->activeGroup; m_impl->Add(std::move(p));
}
void DebugDraw::Ray(const float o[3],const float d[3],float len,DebugColour col,float lt){
    float b[3]={o[0]+d[0]*len,o[1]+d[1]*len,o[2]+d[2]*len}; Line(o,b,col,lt);
}
void DebugDraw::Arrow(const float from[3],const float to[3],DebugColour col,float hs,float lt){
    Line(from,to,col,lt); // simplified: just a line
    (void)hs;
}
void DebugDraw::Sphere(const float c[3],float r,DebugColour col,float lt,int segs){
    Primitive p; p.kind=Primitive::Kind::Sphere; for(int i=0;i<3;++i)p.a[i]=c[i];
    p.r=r; p.col=col; p.lifetime=lt; p.group=m_impl->activeGroup; (void)segs; m_impl->Add(std::move(p));
}
void DebugDraw::Box(const float mn[3],const float mx[3],DebugColour col,float lt){
    Primitive p; p.kind=Primitive::Kind::Box;
    for(int i=0;i<3;++i){p.a[i]=mn[i];p.b[i]=mx[i];}
    p.col=col; p.lifetime=lt; p.group=m_impl->activeGroup; m_impl->Add(std::move(p));
}
void DebugDraw::OBB(const float c[3],const float*,const float*,DebugColour col,float lt){
    Sphere(c,0.1f,col,lt); // stub
}
void DebugDraw::Capsule(const float base[3],const float tip[3],float r,DebugColour col,float lt){
    Line(base,tip,col,lt); (void)r;
}
void DebugDraw::Cross(const float p[3],float s,DebugColour col,float lt){
    float a[3],b[3];
    for(int ax=0;ax<3;++ax){
        for(int i=0;i<3;++i){a[i]=p[i];b[i]=p[i];} a[ax]-=s; b[ax]+=s; Line(a,b,col,lt);
    }
}
void DebugDraw::Axes(const float pos[3],float len,float lt){
    float ex[3]={pos[0]+len,pos[1],pos[2]};
    float ey[3]={pos[0],pos[1]+len,pos[2]};
    float ez[3]={pos[0],pos[1],pos[2]+len};
    Line(pos,ex,{1,0,0,1},lt); Line(pos,ey,{0,1,0,1},lt); Line(pos,ez,{0,0,1,1},lt);
}
void DebugDraw::Grid(const float c[3],float cs,uint32_t cells,DebugColour col,float lt){
    float half=cs*(float)cells*0.5f;
    for(uint32_t i=0;i<=cells;++i){
        float t=-half+cs*(float)i;
        float a[3]={c[0]+t,c[1],c[2]-half};
        float b[3]={c[0]+t,c[1],c[2]+half};
        Line(a,b,col,lt);
        float a2[3]={c[0]-half,c[1],c[2]+t};
        float b2[3]={c[0]+half,c[1],c[2]+t};
        Line(a2,b2,col,lt);
    }
}
void DebugDraw::Text(const float wp[3],const std::string& txt,DebugColour col,float lt){
    Primitive p; p.kind=Primitive::Kind::Text; for(int i=0;i<3;++i)p.a[i]=wp[i];
    p.text=txt; p.col=col; p.lifetime=lt; p.group=m_impl->activeGroup; m_impl->Add(std::move(p));
}

void DebugDraw::EnableGroup(const std::string& g,bool e){ m_impl->groupEnabled[g]=e; }
void DebugDraw::SetGroup(const std::string& g){ m_impl->activeGroup=g; }
void DebugDraw::ClearGroup(){ m_impl->activeGroup.clear(); }

void DebugDraw::Update(float dt){
    auto& v=m_impl->prims;
    for(auto& p:v) if(p.lifetime>0.f) p.elapsed+=dt;
    v.erase(std::remove_if(v.begin(),v.end(),[](auto& p){ return p.lifetime>0.f&&p.elapsed>=p.lifetime; }),v.end());
}
void DebugDraw::Flush(){ /* GPU submission stub */ }
void DebugDraw::Clear(){ m_impl->prims.clear(); }
uint32_t DebugDraw::PrimitiveCount() const{ return (uint32_t)m_impl->prims.size(); }

} // namespace Engine
