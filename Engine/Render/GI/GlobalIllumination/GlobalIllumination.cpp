#include "Engine/Render/GI/GlobalIllumination/GlobalIllumination.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace Engine {

// SH basis Y_l_m evaluated at direction (x,y,z) normalised — L0 and L1 only (4 basis)
// For full L2 we encode 9 coefficients using the standard real SH basis.
static void EncodeSH(const GIRgb* sky6, SHCoeffs9& sh) {
    // 6 face cube samples: +X,-X,+Y,-Y,+Z,-Z
    static const float dirs[6][3]={
        {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
    };
    static const float kPi   = 3.14159265f;
    static const float w = 4.f*kPi/6.f; // solid angle per face (approx)
    float Y[9];
    for(int f=0;f<6;f++){
        float x=dirs[f][0],y=dirs[f][1],z=dirs[f][2];
        // Evaluate 9 real SH basis functions
        Y[0]=0.282095f;
        Y[1]=0.488603f*y;
        Y[2]=0.488603f*z;
        Y[3]=0.488603f*x;
        Y[4]=1.092548f*x*y;
        Y[5]=1.092548f*y*z;
        Y[6]=0.315392f*(3*z*z-1);
        Y[7]=1.092548f*x*z;
        Y[8]=0.546274f*(x*x-y*y);
        for(int i=0;i<9;i++){
            sh.r[i]+=sky6[f].r*Y[i]*w;
            sh.g[i]+=sky6[f].g*Y[i]*w;
            sh.b[i]+=sky6[f].b*Y[i]*w;
        }
    }
}

static GIRgb EvalSH(const SHCoeffs9& sh, GIVec3 n) {
    float x=n.x,y=n.y,z=n.z;
    float Y[9];
    Y[0]=0.282095f;
    Y[1]=0.488603f*y; Y[2]=0.488603f*z; Y[3]=0.488603f*x;
    Y[4]=1.092548f*x*y; Y[5]=1.092548f*y*z;
    Y[6]=0.315392f*(3*z*z-1); Y[7]=1.092548f*x*z;
    Y[8]=0.546274f*(x*x-y*y);
    GIRgb out{0,0,0};
    for(int i=0;i<9;i++){ out.r+=sh.r[i]*Y[i]; out.g+=sh.g[i]*Y[i]; out.b+=sh.b[i]*Y[i]; }
    out.r=std::max(0.f,out.r); out.g=std::max(0.f,out.g); out.b=std::max(0.f,out.b);
    return out;
}

static inline float Dist2(GIVec3 a, GIVec3 b){
    float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z; return dx*dx+dy*dy+dz*dz;
}

struct GlobalIllumination::Impl {
    std::vector<IrradianceProbe> probes;
    uint32_t nextId{1};
    uint32_t blendK{8};
    GIVec3 volMin{-10,-10,-10}, volMax{10,10,10};

    IrradianceProbe* Find(uint32_t id){
        for(auto& p:probes) if(p.id==id) return &p;
        return nullptr;
    }
};

GlobalIllumination::GlobalIllumination(): m_impl(new Impl){}
GlobalIllumination::~GlobalIllumination(){ Shutdown(); delete m_impl; }
void GlobalIllumination::Init(){}
void GlobalIllumination::Shutdown(){ m_impl->probes.clear(); }
void GlobalIllumination::Reset(){ m_impl->probes.clear(); m_impl->nextId=1; }

void GlobalIllumination::SetVolume(GIVec3 mn, GIVec3 mx){ m_impl->volMin=mn; m_impl->volMax=mx; }

uint32_t GlobalIllumination::AddProbe(GIVec3 pos){
    IrradianceProbe p; p.id=m_impl->nextId++; p.pos=pos; p.dirty=true;
    m_impl->probes.push_back(p); return p.id;
}
void GlobalIllumination::RemoveProbe(uint32_t id){
    m_impl->probes.erase(std::remove_if(m_impl->probes.begin(),m_impl->probes.end(),
        [id](const IrradianceProbe& p){return p.id==id;}), m_impl->probes.end());
}

void GlobalIllumination::AutoPlaceProbes(uint32_t cx, uint32_t cy, uint32_t cz){
    m_impl->probes.clear(); m_impl->nextId=1;
    GIVec3 mn=m_impl->volMin, mx=m_impl->volMax;
    float sx=(cx>1)?(mx.x-mn.x)/(cx-1):0;
    float sy=(cy>1)?(mx.y-mn.y)/(cy-1):0;
    float sz=(cz>1)?(mx.z-mn.z)/(cz-1):0;
    for(uint32_t iz=0;iz<cz;iz++) for(uint32_t iy=0;iy<cy;iy++) for(uint32_t ix=0;ix<cx;ix++){
        GIVec3 p{mn.x+ix*sx, mn.y+iy*sy, mn.z+iz*sz};
        AddProbe(p);
    }
}

void GlobalIllumination::BakeProbe(uint32_t id, GIRgb sky, GIRgb bounce){
    auto* p=m_impl->Find(id); if(!p) return;
    // Build 6 face samples: sky hemisphere (+Y) vs bounce (other faces)
    GIRgb sky6[6]={sky, bounce, sky, bounce, bounce, bounce};
    p->sh = SHCoeffs9{};
    EncodeSH(sky6, p->sh);
    p->dirty=false;
}

void GlobalIllumination::BakeAll(GIRgb sky, GIRgb bounce){
    for(auto& p:m_impl->probes) if(p.dirty) BakeProbe(p.id,sky,bounce);
}

GIRgb GlobalIllumination::SampleIrradiance(GIVec3 wp, GIVec3 normal) const {
    if(m_impl->probes.empty()) return {0,0,0};
    // Find k nearest probes
    struct KEntry { float d2; const IrradianceProbe* p; };
    std::vector<KEntry> knn;
    for(auto& p:m_impl->probes){
        if(p.dirty) continue;
        knn.push_back({Dist2(wp,p.pos), &p});
    }
    if(knn.empty()) return {0,0,0};
    std::sort(knn.begin(),knn.end(),[](const KEntry& a,const KEntry& b){return a.d2<b.d2;});
    uint32_t k=std::min((uint32_t)knn.size(), m_impl->blendK);
    // Inverse-distance weighted blend
    GIRgb result{0,0,0}; float wSum=0;
    for(uint32_t i=0;i<k;i++){
        float w=1.f/(knn[i].d2+1e-4f);
        GIRgb c=EvalSH(knn[i].p->sh, normal);
        result.r+=c.r*w; result.g+=c.g*w; result.b+=c.b*w; wSum+=w;
    }
    if(wSum>0){ result.r/=wSum; result.g/=wSum; result.b/=wSum; }
    return result;
}

void GlobalIllumination::SetMaxBlendProbes(uint32_t k){ m_impl->blendK=k; }
void GlobalIllumination::InvalidateProbe(uint32_t id){ auto* p=m_impl->Find(id); if(p) p->dirty=true; }
void GlobalIllumination::InvalidateAll(){ for(auto& p:m_impl->probes) p.dirty=true; }

uint32_t GlobalIllumination::GetProbeCount()         const { return (uint32_t)m_impl->probes.size(); }
GIVec3   GlobalIllumination::GetProbePosition(uint32_t id) const {
    auto* p=m_impl->Find(id); return p?p->pos:GIVec3{0,0,0};
}
bool GlobalIllumination::IsProbeValid(uint32_t id) const {
    auto* p=m_impl->Find(id); return p&&!p->dirty;
}

bool GlobalIllumination::SaveToJSON(const std::string& path) const {
    FILE* f=fopen(path.c_str(),"w"); if(!f) return false;
    fprintf(f,"{\n\"probes\":[\n");
    for(size_t i=0;i<m_impl->probes.size();i++){
        auto& p=m_impl->probes[i];
        fprintf(f,"{\"id\":%u,\"pos\":[%.4f,%.4f,%.4f]}%s\n",
            p.id,p.pos.x,p.pos.y,p.pos.z,i+1<m_impl->probes.size()?",":"");
    }
    fprintf(f,"]\n}\n");
    fclose(f); return true;
}
bool GlobalIllumination::LoadFromJSON(const std::string& /*path*/){ return true; /* stub */ }

} // namespace Engine
