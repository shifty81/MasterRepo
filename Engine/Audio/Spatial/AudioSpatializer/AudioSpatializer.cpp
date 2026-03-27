#include "Engine/Audio/Spatial/AudioSpatializer/AudioSpatializer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

static float Dist3(ASVec3 a, ASVec3 b){
    float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}
static float Dot3(ASVec3 a, ASVec3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static float Len3(ASVec3 v){ return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static ASVec3 Sub3(ASVec3 a, ASVec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }

struct SourceData {
    uint32_t id;
    ASVec3   pos{0,0,0};
    ASVec3   vel{0,0,0};
    float    reverbSend{0};
    std::function<void()> onCull;
};

struct AudioSpatializer::Impl {
    ASVec3           listenerPos    {0,0,0};
    ASVec3           listenerVel    {0,0,0};
    ASVec3           listenerFwd    {0,0,1};
    ASVec3           listenerUp     {0,1,0};
    AttenuationModel attenModel     {AttenuationModel::InverseSquare};
    float            rolloffFactor  {1.f};
    float            refDist        {1.f};
    float            maxDist        {100.f};
    bool             dopplerEnabled {false};
    float            dopplerFactor  {1.f};
    bool             reverbEnabled  {false};
    std::vector<SourceData> sources;

    SourceData* Find(uint32_t id){
        for(auto& s:sources) if(s.id==id) return &s; return nullptr;
    }
    const SourceData* Find(uint32_t id) const {
        for(auto& s:sources) if(s.id==id) return &s; return nullptr;
    }

    float Attenuate(float dist) const {
        dist=std::max(dist,refDist);
        switch(attenModel){
            case AttenuationModel::None:
                return 1.f;
            case AttenuationModel::Linear:
                return 1.f-rolloffFactor*(dist-refDist)/std::max(1e-6f,maxDist-refDist);
            case AttenuationModel::Logarithmic:
                return refDist/(refDist+rolloffFactor*(dist-refDist));
            default: // InverseSquare
                return (refDist*refDist)/(dist*dist)*rolloffFactor;
        }
    }
};

AudioSpatializer::AudioSpatializer(): m_impl(new Impl){}
AudioSpatializer::~AudioSpatializer(){ Shutdown(); delete m_impl; }
void AudioSpatializer::Init(){}
void AudioSpatializer::Shutdown(){ m_impl->sources.clear(); }
void AudioSpatializer::Reset(){ *m_impl=Impl{}; }

void AudioSpatializer::SetListenerPos(ASVec3 pos, ASVec3 vel, ASVec3 fwd, ASVec3 up){
    m_impl->listenerPos=pos; m_impl->listenerVel=vel;
    float lf=Len3(fwd); if(lf>0) m_impl->listenerFwd={fwd.x/lf,fwd.y/lf,fwd.z/lf};
    float lu=Len3(up);  if(lu>0) m_impl->listenerUp ={up.x/lu,up.y/lu,up.z/lu};
}

void AudioSpatializer::RegisterSource(uint32_t id, ASVec3 pos, ASVec3 vel){
    for(auto& s:m_impl->sources) if(s.id==id){ s.pos=pos; s.vel=vel; return; }
    SourceData sd; sd.id=id; sd.pos=pos; sd.vel=vel;
    m_impl->sources.push_back(sd);
}
void AudioSpatializer::RemoveSource(uint32_t id){
    m_impl->sources.erase(std::remove_if(m_impl->sources.begin(),m_impl->sources.end(),
        [id](const SourceData& s){return s.id==id;}),m_impl->sources.end());
}
void AudioSpatializer::SetSourcePos(uint32_t id, ASVec3 pos){
    auto* s=m_impl->Find(id); if(s) s->pos=pos;
}
void AudioSpatializer::SetSourceVel(uint32_t id, ASVec3 vel){
    auto* s=m_impl->Find(id); if(s) s->vel=vel;
}

void AudioSpatializer::SetAttenuationModel(AttenuationModel m){ m_impl->attenModel=m; }
void AudioSpatializer::SetRolloffFactor   (float f){ m_impl->rolloffFactor=f; }
void AudioSpatializer::SetRefDistance     (float d){ m_impl->refDist=d; }
void AudioSpatializer::SetMaxDistance     (float d){ m_impl->maxDist=d; }
void AudioSpatializer::EnableDoppler      (bool on){ m_impl->dopplerEnabled=on; }
void AudioSpatializer::SetDopplerFactor   (float f){ m_impl->dopplerFactor=f; }
void AudioSpatializer::EnableReverb       (bool on){ m_impl->reverbEnabled=on; }
void AudioSpatializer::SetReverbSend(uint32_t id, float send){
    auto* s=m_impl->Find(id); if(s) s->reverbSend=send;
}

SpatialOutput AudioSpatializer::Evaluate(uint32_t id, float dt) const {
    auto* src=m_impl->Find(id);
    SpatialOutput out;
    if(!src){ out.leftGain=0; out.rightGain=0; return out; }

    float dist=Dist3(src->pos, m_impl->listenerPos);
    if(dist>m_impl->maxDist){
        out.leftGain=0; out.rightGain=0;
        if(src->onCull) const_cast<SourceData*>(src)->onCull();
        return out;
    }

    float gain=std::max(0.f,std::min(1.f,m_impl->Attenuate(dist)));

    // Simple stereo panning via dot with listener right vector
    // right = fwd × up
    ASVec3 fwd=m_impl->listenerFwd, up=m_impl->listenerUp;
    ASVec3 right={fwd.y*up.z-fwd.z*up.y, fwd.z*up.x-fwd.x*up.z, fwd.x*up.y-fwd.y*up.x};
    ASVec3 dir=Sub3(src->pos,m_impl->listenerPos);
    float dl=Len3(dir);
    float pan = (dl>1e-6f)?Dot3({dir.x/dl,dir.y/dl,dir.z/dl},right):0;
    // pan: -1=hard left, +1=hard right
    out.leftGain =gain*(1.f-std::max(0.f, pan));
    out.rightGain=gain*(1.f+std::min(0.f,-pan));
    // Normalise so max is gain
    float mx=std::max(out.leftGain,out.rightGain);
    if(mx>0){ out.leftGain*=gain/mx; out.rightGain*=gain/mx; }

    // Doppler
    if(m_impl->dopplerEnabled){
        static const float kSoundSpeed=343.f;
        ASVec3 relPos=Sub3(src->pos,m_impl->listenerPos);
        float d=Len3(relPos);
        if(d>1e-6f){
            ASVec3 n={relPos.x/d,relPos.y/d,relPos.z/d};
            float vSrc=-Dot3(src->vel,n);
            float vLis=-Dot3(m_impl->listenerVel,n);
            float num=kSoundSpeed+vLis;
            float den=kSoundSpeed+vSrc;
            if(std::abs(den)>1e-6f)
                out.pitchShift=std::max(0.1f,num/den)*m_impl->dopplerFactor;
        }
    }

    out.reverbSend=m_impl->reverbEnabled?src->reverbSend:0;
    (void)dt;
    return out;
}

std::vector<SpatialOutput> AudioSpatializer::EvaluateAll(float dt) const {
    std::vector<SpatialOutput> out;
    for(auto& s:m_impl->sources) out.push_back(Evaluate(s.id,dt));
    return out;
}

void AudioSpatializer::SetOnCull(uint32_t id, std::function<void()> cb){
    auto* s=m_impl->Find(id); if(s) s->onCull=cb;
}
uint32_t AudioSpatializer::GetSourceCount() const { return (uint32_t)m_impl->sources.size(); }

} // namespace Engine
