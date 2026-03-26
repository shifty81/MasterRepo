#include "Engine/Lighting/LightBaker/LightBaker.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

namespace Engine {

// ── Simple ray-triangle intersection (Möller-Trumbore) ───────────────────────

static bool RayTriHit(const float orig[3], const float dir[3],
                       const float v0[3], const float v1[3], const float v2[3],
                       float& tOut){
    const float EPSILON=1e-7f;
    float e1[3]={v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2]};
    float e2[3]={v2[0]-v0[0],v2[1]-v0[1],v2[2]-v0[2]};
    float h[3]={dir[1]*e2[2]-dir[2]*e2[1],dir[2]*e2[0]-dir[0]*e2[2],dir[0]*e2[1]-dir[1]*e2[0]};
    float a=e1[0]*h[0]+e1[1]*h[1]+e1[2]*h[2];
    if(std::abs(a)<EPSILON) return false;
    float f=1.f/a;
    float s[3]={orig[0]-v0[0],orig[1]-v0[1],orig[2]-v0[2]};
    float u=f*(s[0]*h[0]+s[1]*h[1]+s[2]*h[2]);
    if(u<0||u>1) return false;
    float q[3]={s[1]*e1[2]-s[2]*e1[1],s[2]*e1[0]-s[0]*e1[2],s[0]*e1[1]-s[1]*e1[0]};
    float v=f*(dir[0]*q[0]+dir[1]*q[1]+dir[2]*q[2]);
    if(v<0||u+v>1) return false;
    tOut=f*(e2[0]*q[0]+e2[1]*q[1]+e2[2]*q[2]);
    return tOut>EPSILON;
}

static float Dot3(const float a[3],const float b[3]){ return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
static void  Normalize3(float v[3]){ float l=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); if(l>1e-6f){v[0]/=l;v[1]/=l;v[2]/=l;} }

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct LightBaker::Impl {
    BakeSettings             settings;
    std::vector<BakeMeshEntry> meshes;
    std::vector<BakeLight>   lights;
    std::vector<BakeResult>  results;
    BakeState                state{BakeState::Idle};
    float                    progress{0.f};
    std::thread              bakeThread;
    std::function<void(bool)>  onComplete;

    bool IsShadowed(const float point[3], const float dir[3], float maxDist) const {
        for(auto& mesh:meshes){
            uint32_t triCount=(uint32_t)mesh.indices.size()/3;
            for(uint32_t ti=0;ti<triCount;++ti){
                uint32_t i0=mesh.indices[ti*3],i1=mesh.indices[ti*3+1],i2=mesh.indices[ti*3+2];
                float t;
                if(RayTriHit(point,dir,
                    mesh.vertices.data()+i0*3,
                    mesh.vertices.data()+i1*3,
                    mesh.vertices.data()+i2*3,t) && t<maxDist) return true;
            }
        }
        return false;
    }

    float ComputeAO(const float point[3], const float normal[3]) const {
        // Sample hemisphere
        int occluded=0, total=settings.samples;
        for(int i=0;i<total;++i){
            float u=((float)rand()/(float)RAND_MAX)*2.f-1.f;
            float v=((float)rand()/(float)RAND_MAX)*2.f-1.f;
            float w=((float)rand()/(float)RAND_MAX);
            float dir[3]={u,v,w};
            Normalize3(dir);
            if(Dot3(dir,normal)<0){dir[0]*=-1;dir[1]*=-1;dir[2]*=-1;}
            float orig[3]={point[0]+normal[0]*0.001f,point[1]+normal[1]*0.001f,point[2]+normal[2]*0.001f};
            if(IsShadowed(orig,dir,settings.aoRadius)) ++occluded;
        }
        return 1.f-(float)occluded/(float)total;
    }

    void DoBake(std::function<void(float)> onProgress){
        state=BakeState::Baking;
        results.clear();
        uint32_t total=(uint32_t)meshes.size();
        for(uint32_t mi=0;mi<total;++mi){
            auto& mesh=meshes[mi];
            uint32_t res=settings.resolution;
            BakeResult r; r.meshName=mesh.name; r.width=res; r.height=res;
            r.rgbaPixels.resize((size_t)res*res*4,0);
            uint32_t uvCount=(uint32_t)mesh.uvs.size()/2;
            for(uint32_t pi=0;pi<uvCount&&pi<res*res;++pi){
                uint32_t px=(uint32_t)(mesh.uvs[pi*2]*(res-1));
                uint32_t py=(uint32_t)(mesh.uvs[pi*2+1]*(res-1));
                if(px>=res||py>=res) continue;
                const float* point=mesh.vertices.data()+pi*3;
                float* normal=(pi*3+2<(uint32_t)mesh.normals.size())?
                               const_cast<float*>(mesh.normals.data()+pi*3):nullptr;
                float lightVal=0.f;
                for(auto& light:lights){
                    if(light.type==BakeLightType::Directional){
                        float dir[3]={-light.direction[0],-light.direction[1],-light.direction[2]};
                        Normalize3(dir);
                        float orig[3]={point[0]+dir[0]*0.001f,point[1]+dir[1]*0.001f,point[2]+dir[2]*0.001f};
                        float shadow=(!light.castShadows||!IsShadowed(orig,dir,1e6f))?1.f:0.f;
                        float diffuse=normal?std::max(0.f,Dot3(normal,dir)):1.f;
                        lightVal+=light.intensity*light.colour[0]*diffuse*shadow;
                    }
                }
                float ao=settings.bakeAO&&normal?ComputeAO(point,normal):1.f;
                lightVal*=ao;
                lightVal=std::min(1.f,lightVal);
                size_t idx=((size_t)py*res+px)*4;
                r.rgbaPixels[idx+0]=r.rgbaPixels[idx+1]=r.rgbaPixels[idx+2]=(uint8_t)(lightVal*255.f);
                r.rgbaPixels[idx+3]=255;
            }
            r.succeeded=true; results.push_back(r);
            progress=(float)(mi+1)/(float)total;
            if(onProgress) onProgress(progress);
        }
        state=BakeState::Finished;
        if(onComplete) onComplete(true);
    }
};

LightBaker::LightBaker() : m_impl(new Impl()) {}
LightBaker::~LightBaker() { if(m_impl->bakeThread.joinable()) m_impl->bakeThread.join(); delete m_impl; }
void LightBaker::Init(const BakeSettings& s){ m_impl->settings=s; }
void LightBaker::Shutdown(){ Cancel(); }

void LightBaker::AddStaticMesh(const BakeMeshEntry& mesh){ m_impl->meshes.push_back(mesh); }
void LightBaker::RemoveStaticMesh(const std::string& name){
    auto& v=m_impl->meshes; v.erase(std::remove_if(v.begin(),v.end(),[&](auto& m){ return m.name==name; }),v.end());
}
void LightBaker::AddLight(const BakeLight& l){ m_impl->lights.push_back(l); }
void LightBaker::ClearLights(){ m_impl->lights.clear(); }
void LightBaker::ClearMeshes(){ m_impl->meshes.clear(); }

void LightBaker::Bake(){ m_impl->DoBake(nullptr); }
void LightBaker::BakeAsync(std::function<void(float)> onProgress){
    if(m_impl->bakeThread.joinable()) m_impl->bakeThread.join();
    m_impl->bakeThread=std::thread([this,onProgress]{ m_impl->DoBake(onProgress); });
}
void LightBaker::WaitForCompletion(){ if(m_impl->bakeThread.joinable()) m_impl->bakeThread.join(); }
void LightBaker::Cancel(){
    // signal + wait – simplified: just wait
    if(m_impl->bakeThread.joinable()) m_impl->bakeThread.join();
    m_impl->state=BakeState::Idle;
}

BakeState LightBaker::GetState() const{ return m_impl->state; }
float     LightBaker::GetProgress() const{ return m_impl->progress; }
const std::vector<BakeResult>& LightBaker::GetResults() const{ return m_impl->results; }
BakeResult LightBaker::GetResult(const std::string& name) const{
    for(auto& r:m_impl->results) if(r.meshName==name) return r;
    return {};
}
bool LightBaker::ExportLightmaps(const std::string& dir) const{
    bool ok=true;
    for(auto& r:m_impl->results){
        std::string path=dir+"/"+r.meshName+"_lightmap.pgm";
        std::ofstream f(path,std::ios::binary); if(!f.is_open()){ ok=false; continue; }
        f<<"P5\n"<<r.width<<" "<<r.height<<"\n255\n";
        for(size_t i=0;i<(size_t)r.width*r.height;++i) f.write((char*)&r.rgbaPixels[i*4],1);
    }
    return ok;
}
void LightBaker::OnComplete(std::function<void(bool)> cb){ m_impl->onComplete=std::move(cb); }

} // namespace Engine
