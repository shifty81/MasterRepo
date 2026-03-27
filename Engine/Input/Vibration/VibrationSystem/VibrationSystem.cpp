#include "Engine/Input/Vibration/VibrationSystem/VibrationSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace Engine {

struct ControllerState {
    uint32_t id;
    float    intensity{1.f};
    float    leftStrength{0};
    float    rightStrength{0};
    bool     playing{false};
    // Active pattern playback
    std::vector<VibrationKeyframe> activeFrames;
    float playhead{0};
    bool  loop{false};
    // Raw wave playback
    std::vector<float> waveLeft;
    std::vector<float> waveRight;
    uint32_t wavePos{0};
    uint32_t waveRate{60};
    float    waveDt{0};
};

struct VibrationSystem::Impl {
    std::unordered_map<uint32_t,ControllerState> controllers;
    std::unordered_map<std::string,VibrationPattern> patterns;
    std::function<void(uint32_t,float,float)> onMotors;

    ControllerState* Find(uint32_t id){
        auto it=controllers.find(id); return it!=controllers.end()?&it->second:nullptr;
    }
};

VibrationSystem::VibrationSystem(): m_impl(new Impl){}
VibrationSystem::~VibrationSystem(){ Shutdown(); delete m_impl; }
void VibrationSystem::Init(){}
void VibrationSystem::Shutdown(){ m_impl->controllers.clear(); }
void VibrationSystem::Reset(){ m_impl->controllers.clear(); }

void VibrationSystem::RegisterController(uint32_t id){
    ControllerState cs; cs.id=id; m_impl->controllers[id]=cs;
}
void VibrationSystem::UnregisterController(uint32_t id){ m_impl->controllers.erase(id); }

void VibrationSystem::RegisterPattern(const VibrationPattern& p){
    m_impl->patterns[p.name]=p;
}

void VibrationSystem::PlayPattern(uint32_t controllerId, const std::string& name){
    auto* cs=m_impl->Find(controllerId); if(!cs) return;
    auto it=m_impl->patterns.find(name); if(it==m_impl->patterns.end()) return;
    cs->activeFrames=it->second.keyframes;
    cs->loop=it->second.loop;
    cs->playhead=0; cs->playing=true;
    cs->waveLeft.clear(); cs->waveRight.clear();
}

void VibrationSystem::PlayImpulse(uint32_t controllerId, float left, float right, float dur){
    auto* cs=m_impl->Find(controllerId); if(!cs) return;
    cs->activeFrames={{0.f,left,right},{dur,left,right},{dur+0.001f,0,0}};
    cs->loop=false; cs->playhead=0; cs->playing=true;
    cs->waveLeft.clear(); cs->waveRight.clear();
}

void VibrationSystem::PlayWave(uint32_t controllerId, VibrationMotor motor,
                                const float* samples, uint32_t count, uint32_t sr){
    auto* cs=m_impl->Find(controllerId); if(!cs) return;
    cs->waveRate=sr; cs->wavePos=0; cs->waveDt=0;
    cs->playing=true;
    cs->activeFrames.clear();
    if(motor&MotorLeft)  cs->waveLeft.assign(samples,samples+count);
    else cs->waveLeft.clear();
    if(motor&MotorRight) cs->waveRight.assign(samples,samples+count);
    else cs->waveRight.clear();
}

void VibrationSystem::StopAll(uint32_t controllerId){
    auto* cs=m_impl->Find(controllerId); if(!cs) return;
    cs->playing=false; cs->leftStrength=0; cs->rightStrength=0;
    cs->activeFrames.clear(); cs->waveLeft.clear(); cs->waveRight.clear();
    if(m_impl->onMotors) m_impl->onMotors(controllerId,0,0);
}

void VibrationSystem::SetBaseIntensity(uint32_t id, float intensity){
    auto* cs=m_impl->Find(id); if(cs) cs->intensity=intensity;
}

void VibrationSystem::Tick(float dt){
    for(auto& [id,cs]:m_impl->controllers){
        if(!cs.playing){ continue; }
        float left=0, right=0;

        // Wave playback
        if(!cs.waveLeft.empty()||!cs.waveRight.empty()){
            cs.waveDt+=dt;
            float period=1.f/std::max(1u,cs.waveRate);
            while(cs.waveDt>=period){ cs.waveDt-=period; cs.wavePos++; }
            bool doneL=cs.waveLeft.empty()||cs.wavePos>=(uint32_t)cs.waveLeft.size();
            bool doneR=cs.waveRight.empty()||cs.wavePos>=(uint32_t)cs.waveRight.size();
            if(!doneL) left =cs.waveLeft [cs.wavePos];
            if(!doneR) right=cs.waveRight[cs.wavePos];
            if(doneL&&doneR){ cs.playing=false; }
        } else if(!cs.activeFrames.empty()){
            // Pattern keyframe interpolation
            cs.playhead+=dt;
            auto& frames=cs.activeFrames;
            if(cs.playhead>=frames.back().timeSec){
                if(cs.loop) cs.playhead=std::fmod(cs.playhead,frames.back().timeSec);
                else{ cs.playing=false; left=0; right=0; goto apply; }
            }
            // Find bracket
            for(size_t i=0;i+1<frames.size();i++){
                if(cs.playhead>=frames[i].timeSec&&cs.playhead<=frames[i+1].timeSec){
                    float seg=frames[i+1].timeSec-frames[i].timeSec;
                    float u=(seg>0)?(cs.playhead-frames[i].timeSec)/seg:0.f;
                    left =frames[i].left +(frames[i+1].left -frames[i].left )*u;
                    right=frames[i].right+(frames[i+1].right-frames[i].right)*u;
                    break;
                }
            }
        }

        apply:
        cs.leftStrength =left *cs.intensity;
        cs.rightStrength=right*cs.intensity;
        if(m_impl->onMotors) m_impl->onMotors(id, cs.leftStrength, cs.rightStrength);
    }
}

void VibrationSystem::GetCurrentStrength(uint32_t id, float& l, float& r) const {
    auto* cs=m_impl->Find(id); l=cs?cs->leftStrength:0; r=cs?cs->rightStrength:0;
}
bool VibrationSystem::IsPlaying(uint32_t id) const {
    auto* cs=m_impl->Find(id); return cs&&cs->playing;
}
void VibrationSystem::SetOnMotors(std::function<void(uint32_t,float,float)> cb){ m_impl->onMotors=cb; }

} // namespace Engine
