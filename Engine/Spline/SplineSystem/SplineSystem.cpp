#include "Engine/Spline/SplineSystem/SplineSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Len3(SplineVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static SplineVec3 Norm3(SplineVec3 v){
    float l=Len3(v); return l>0?SplineVec3{v.x/l,v.y/l,v.z/l}:SplineVec3{};
}
static SplineVec3 Sub(SplineVec3 a,SplineVec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
static SplineVec3 Add(SplineVec3 a,SplineVec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static SplineVec3 Scale(SplineVec3 v,float s){return {v.x*s,v.y*s,v.z*s};}
static SplineVec3 Cross(SplineVec3 a,SplineVec3 b){
    return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

struct SplineData {
    std::vector<SplineVec3> pts;
    SplineType type{SplineType::CatmullRom};
    float tension{0.5f};
    bool  closed{false};
};

struct SplineSystem::Impl {
    std::unordered_map<uint32_t,SplineData> splines;
    SplineData* Find(uint32_t id){ auto it=splines.find(id); return it!=splines.end()?&it->second:nullptr; }

    SplineVec3 EvalCR(const SplineVec3& p0,const SplineVec3& p1,
                      const SplineVec3& p2,const SplineVec3& p3,float t,float tens) const {
        float t2=t*t, t3=t2*t;
        // Catmull-Rom with adjustable tension
        float a=(-tens)*t3+(2*tens)*t2+(-tens)*t+0;
        float b=(2-tens)*t3+(tens-3)*t2+1;
        float c=(tens-2)*t3+(3-2*tens)*t2+(tens)*t;
        float d=(tens)*t3+(-tens)*t2;
        return {a*p0.x+b*p1.x+c*p2.x+d*p3.x,
                a*p0.y+b*p1.y+c*p2.y+d*p3.y,
                a*p0.z+b*p1.z+c*p2.z+d*p3.z};
    }
    SplineVec3 EvalLinear(const SplineVec3& a,const SplineVec3& b,float t) const {
        return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};
    }
    SplineVec3 EvalAtT(const SplineData& s, float t) const {
        if(s.pts.size()<2) return s.pts.empty()?SplineVec3{}:s.pts[0];
        uint32_t segs=(uint32_t)s.pts.size()-(s.closed?0:1);
        if(segs==0) return s.pts[0];
        float ft=t*(float)segs; uint32_t seg=(uint32_t)ft;
        float lt=ft-seg;
        if(seg>=segs){seg=segs-1;lt=1;}
        uint32_t n=(uint32_t)s.pts.size();
        uint32_t i1=seg%n, i2=(seg+1)%n;
        if(s.type==SplineType::Linear) return EvalLinear(s.pts[i1],s.pts[i2],lt);
        uint32_t i0=(seg==0&&!s.closed)?i1:(seg==0?n-1:(seg-1)%n);
        uint32_t i3=(seg+2)%n;
        return EvalCR(s.pts[i0],s.pts[i1],s.pts[i2],s.pts[i3],lt,s.tension);
    }
};

SplineSystem::SplineSystem(): m_impl(new Impl){}
SplineSystem::~SplineSystem(){ Shutdown(); delete m_impl; }
void SplineSystem::Init(){}
void SplineSystem::Shutdown(){ m_impl->splines.clear(); }
void SplineSystem::Reset(){ m_impl->splines.clear(); }

void SplineSystem::CreateSpline (uint32_t id){ m_impl->splines[id]=SplineData{}; }
void SplineSystem::DestroySpline(uint32_t id){ m_impl->splines.erase(id); }

void SplineSystem::AddPoint(uint32_t id, SplineVec3 pos){
    auto* s=m_impl->Find(id); if(s) s->pts.push_back(pos);
}
void SplineSystem::InsertPoint(uint32_t id, uint32_t idx, SplineVec3 pos){
    auto* s=m_impl->Find(id); if(!s) return;
    idx=std::min(idx,(uint32_t)s->pts.size());
    s->pts.insert(s->pts.begin()+idx,pos);
}
void SplineSystem::RemovePoint(uint32_t id, uint32_t idx){
    auto* s=m_impl->Find(id); if(!s||idx>=s->pts.size()) return;
    s->pts.erase(s->pts.begin()+idx);
}
uint32_t SplineSystem::GetPointCount(uint32_t id) const {
    auto* s=m_impl->Find(id); return s?(uint32_t)s->pts.size():0;
}
void SplineSystem::SetClosed(uint32_t id,bool on){auto* s=m_impl->Find(id);if(s)s->closed=on;}
bool SplineSystem::IsClosed(uint32_t id) const{auto* s=m_impl->Find(id);return s&&s->closed;}
void SplineSystem::SetType(uint32_t id,SplineType t){auto* s=m_impl->Find(id);if(s)s->type=t;}
SplineType SplineSystem::GetType(uint32_t id) const{auto* s=m_impl->Find(id);return s?s->type:SplineType::Linear;}
void SplineSystem::SetTension(uint32_t id,float t){auto* s=m_impl->Find(id);if(s)s->tension=t;}

SplineVec3 SplineSystem::SamplePosition(uint32_t id,float t) const {
    auto* s=m_impl->Find(id); if(!s) return {}; return m_impl->EvalAtT(*s,t);
}
SplineVec3 SplineSystem::SampleTangent(uint32_t id,float t) const {
    auto* s=m_impl->Find(id); if(!s) return {};
    float eps=0.001f;
    SplineVec3 a=m_impl->EvalAtT(*s,std::max(0.f,t-eps));
    SplineVec3 b=m_impl->EvalAtT(*s,std::min(1.f,t+eps));
    return Norm3(Sub(b,a));
}
SplineVec3 SplineSystem::SampleNormal(uint32_t id,float t) const {
    SplineVec3 tan=SampleTangent(id,t);
    SplineVec3 up={0,1,0};
    if(std::abs(tan.y)>0.99f) up={1,0,0};
    return Norm3(Cross(up,tan));
}

float SplineSystem::GetLength(uint32_t id,uint32_t segs) const {
    auto* s=m_impl->Find(id); if(!s||s->pts.size()<2) return 0;
    float len=0; SplineVec3 prev=m_impl->EvalAtT(*s,0);
    for(uint32_t i=1;i<=segs;i++){
        SplineVec3 cur=m_impl->EvalAtT(*s,(float)i/segs);
        len+=Len3(Sub(cur,prev)); prev=cur;
    }
    return len;
}
float SplineSystem::GetTAtLength(uint32_t id,float length,uint32_t segs) const {
    float total=GetLength(id,segs); if(total<=0) return 0;
    auto* s=m_impl->Find(id); if(!s) return 0;
    float acc=0; SplineVec3 prev2=m_impl->EvalAtT(*s,0);
    for(uint32_t i=1;i<=segs;i++){
        float t=(float)i/segs;
        SplineVec3 cur=m_impl->EvalAtT(*s,t);
        float seg=Len3(Sub(cur,prev2)); acc+=seg;
        if(acc>=length) return t;
        prev2=cur;
    }
    return 1.f;
}
uint32_t SplineSystem::Resample(uint32_t id,uint32_t count,std::vector<SplineVec3>& out) const {
    out.resize(count);
    for(uint32_t i=0;i<count;i++) out[i]=SamplePosition(id,(float)i/(count>1?count-1:1));
    return count;
}

} // namespace Engine
