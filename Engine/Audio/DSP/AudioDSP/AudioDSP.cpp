#include "Engine/Audio/DSP/AudioDSP/AudioDSP.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

// --- Biquad filter state ---
struct BiquadState {
    float b0{1},b1{0},b2{0},a1{0},a2{0};
    float x1{0},x2{0},y1{0},y2{0};

    void Process(float* s, uint32_t n){
        for(uint32_t i=0;i<n;i++){
            float y=b0*s[i]+b1*x1+b2*x2-a1*y1-a2*y2;
            x2=x1; x1=s[i]; y2=y1; y1=y; s[i]=y;
        }
    }
    void ConfigLPF(float cutoff, float Q, float sr){
        float w=2*3.14159265f*cutoff/sr;
        float alpha=std::sin(w)/(2*Q);
        float cosw=std::cos(w);
        float a0=1+alpha;
        b0=(1-cosw)/(2*a0); b1=(1-cosw)/a0; b2=b0;
        a1=-2*cosw/a0; a2=(1-alpha)/a0;
    }
    void ConfigHPF(float cutoff, float Q, float sr){
        float w=2*3.14159265f*cutoff/sr;
        float alpha=std::sin(w)/(2*Q);
        float cosw=std::cos(w);
        float a0=1+alpha;
        b0=(1+cosw)/(2*a0); b1=-(1+cosw)/a0; b2=b0;
        a1=-2*cosw/a0; a2=(1-alpha)/a0;
    }
    void ConfigBPF(float cutoff, float Q, float sr){
        float w=2*3.14159265f*cutoff/sr;
        float alpha=std::sin(w)/(2*Q);
        float a0=1+alpha;
        b0=alpha/a0; b1=0; b2=-alpha/a0;
        a1=-2*std::cos(w)/a0; a2=(1-alpha)/a0;
    }
    void Reset(){ x1=x2=y1=y2=0; }
};

struct DSPEffect {
    DSPEffectType type;
    DSPParams     params;
    bool          enabled{true};
    // Internal state
    BiquadState   biquad;
    std::vector<float> delayLine;
    uint32_t      delayPos{0};
    float         envLevel{0}; // compressor envelope
    float         chorusPhase{0};

    void Configure(uint32_t sr){
        biquad.Reset();
        switch(type){
            case DSPEffectType::LowPass:  biquad.ConfigLPF(params.cutoff,params.resonance,(float)sr); break;
            case DSPEffectType::HighPass: biquad.ConfigHPF(params.cutoff,params.resonance,(float)sr); break;
            case DSPEffectType::BandPass: biquad.ConfigBPF(params.cutoff,params.resonance,(float)sr); break;
            case DSPEffectType::Delay:
            case DSPEffectType::Reverb:{
                uint32_t dlLen=(uint32_t)(params.decay*(float)sr)+1;
                delayLine.assign(dlLen,0.f); delayPos=0; break;
            }
            default: break;
        }
    }

    void Apply(float* s, uint32_t n, uint32_t sr){
        if(!enabled) return;
        switch(type){
            case DSPEffectType::LowPass:
            case DSPEffectType::HighPass:
            case DSPEffectType::BandPass:
                biquad.Process(s,n); break;
            case DSPEffectType::Gain:
                for(uint32_t i=0;i<n;i++) s[i]*=params.gain;
                break;
            case DSPEffectType::Delay:{
                if(delayLine.empty()) Configure(sr);
                uint32_t dl=(uint32_t)delayLine.size();
                for(uint32_t i=0;i<n;i++){
                    float wet=delayLine[delayPos];
                    delayLine[delayPos]=s[i]+wet*params.mix;
                    s[i]=s[i]*(1-params.mix)+wet*params.mix;
                    delayPos=(delayPos+1)%dl;
                }
                break;
            }
            case DSPEffectType::Reverb:{
                if(delayLine.empty()) Configure(sr);
                uint32_t dl=(uint32_t)delayLine.size();
                for(uint32_t i=0;i<n;i++){
                    float feedback=delayLine[delayPos]*0.6f;
                    delayLine[delayPos]=s[i]+feedback;
                    s[i]=s[i]*(1-params.mix)+delayLine[delayPos]*params.mix;
                    delayPos=(delayPos+1)%dl;
                }
                break;
            }
            case DSPEffectType::Chorus:{
                float period=(float)sr/std::max(0.01f,params.rate);
                for(uint32_t i=0;i<n;i++){
                    float lfo=std::sin(chorusPhase*2*3.14159265f)*0.5f+0.5f;
                    float delaySec=params.depth*lfo;
                    (void)delaySec; // simple chorus: mix with LFO-modulated gain
                    s[i]=s[i]*(1-params.mix)+s[i]*params.mix*lfo;
                    chorusPhase+=1.f/period;
                    if(chorusPhase>=1.f) chorusPhase-=1.f;
                }
                break;
            }
            case DSPEffectType::Compressor:{
                float thresh=std::pow(10.f,params.threshold/20.f);
                float ratio=params.ratio;
                float attackCoeff=std::exp(-1.f/((float)sr*0.01f));
                float releaseCoeff=std::exp(-1.f/((float)sr*0.1f));
                for(uint32_t i=0;i<n;i++){
                    float level=std::fabs(s[i]);
                    envLevel=(level>envLevel)?(attackCoeff*envLevel+(1-attackCoeff)*level)
                                             :(releaseCoeff*envLevel);
                    float gain=1.f;
                    if(envLevel>thresh) gain=thresh+(envLevel-thresh)/ratio;
                    gain=(envLevel>1e-6f)?gain/envLevel:1.f;
                    s[i]*=gain;
                }
                break;
            }
        }
    }
};

struct DSPChain {
    uint32_t id;
    std::string name;
    std::vector<DSPEffect> effects;
};

struct AudioDSP::Impl {
    uint32_t nextId{1};
    std::vector<DSPChain> chains;
    DSPChain* Find(uint32_t id){ for(auto& c:chains) if(c.id==id) return &c; return nullptr; }
};

AudioDSP::AudioDSP(): m_impl(new Impl){}
AudioDSP::~AudioDSP(){ Shutdown(); delete m_impl; }
void AudioDSP::Init(){}
void AudioDSP::Shutdown(){ m_impl->chains.clear(); }

uint32_t AudioDSP::CreateChain(const std::string& name){
    DSPChain c; c.id=m_impl->nextId++; c.name=name;
    m_impl->chains.push_back(c); return c.id;
}
void AudioDSP::DestroyChain(uint32_t id){
    m_impl->chains.erase(std::remove_if(m_impl->chains.begin(),m_impl->chains.end(),
        [id](const DSPChain& c){return c.id==id;}), m_impl->chains.end());
}

uint32_t AudioDSP::AddEffect(uint32_t chainId, DSPEffectType type, const DSPParams& p){
    auto* c=m_impl->Find(chainId); if(!c) return 0;
    DSPEffect e; e.type=type; e.params=p;
    c->effects.push_back(e);
    return (uint32_t)c->effects.size()-1;
}
void AudioDSP::RemoveEffect(uint32_t chainId, uint32_t idx){
    auto* c=m_impl->Find(chainId);
    if(c&&idx<c->effects.size()) c->effects.erase(c->effects.begin()+idx);
}
void AudioDSP::MoveEffect(uint32_t chainId, uint32_t from, uint32_t to){
    auto* c=m_impl->Find(chainId);
    if(!c||from>=c->effects.size()||to>=c->effects.size()) return;
    DSPEffect e=c->effects[from];
    c->effects.erase(c->effects.begin()+from);
    c->effects.insert(c->effects.begin()+to,e);
}

void AudioDSP::Process(uint32_t chainId, float* s, uint32_t n, uint32_t sr){
    auto* c=m_impl->Find(chainId); if(!c) return;
    for(auto& e:c->effects) e.Apply(s,n,sr);
}

void AudioDSP::SetEffectParam(uint32_t chainId, uint32_t idx, const std::string& param, float val){
    auto* c=m_impl->Find(chainId); if(!c||idx>=c->effects.size()) return;
    auto& p=c->effects[idx].params;
    if(param=="cutoff")    p.cutoff=val;
    else if(param=="resonance") p.resonance=val;
    else if(param=="decay")     p.decay=val;
    else if(param=="mix")       p.mix=val;
    else if(param=="threshold") p.threshold=val;
    else if(param=="ratio")     p.ratio=val;
    else if(param=="gain")      p.gain=val;
    else if(param=="rate")      p.rate=val;
    else if(param=="depth")     p.depth=val;
}
void AudioDSP::SetEffectEnabled(uint32_t chainId, uint32_t idx, bool en){
    auto* c=m_impl->Find(chainId); if(c&&idx<c->effects.size()) c->effects[idx].enabled=en;
}
uint32_t AudioDSP::GetEffectCount(uint32_t chainId) const {
    auto* c=m_impl->Find(chainId); return c?(uint32_t)c->effects.size():0;
}
uint32_t AudioDSP::GetChainLatency(uint32_t /*chainId*/) const { return 0; }
void AudioDSP::Reset(uint32_t chainId){
    auto* c=m_impl->Find(chainId); if(!c) return;
    for(auto& e:c->effects){
        e.biquad.Reset();
        std::fill(e.delayLine.begin(),e.delayLine.end(),0.f);
        e.envLevel=0; e.chorusPhase=0;
    }
}
DSPParams AudioDSP::GetEffectParams(uint32_t chainId, uint32_t idx) const {
    auto* c=m_impl->Find(chainId);
    if(c&&idx<c->effects.size()) return c->effects[idx].params;
    return DSPParams{};
}

} // namespace Engine
