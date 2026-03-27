#include "Engine/Curve/CurveEditor/CurveEditor.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace Engine {

// ── Hermite cubic ────────────────────────────────────────────────────────────
static float Hermite(float p0, float m0, float p1, float m1, float t) {
    float t2=t*t, t3=t2*t;
    return (2*t3-3*t2+1)*p0 + (t3-2*t2+t)*m0 + (-2*t3+3*t2)*p1 + (t3-t2)*m1;
}

struct CurveChannel {
    std::vector<CurveKey> keys;
    WrapMode wrapMode{WrapMode::Clamp};

    void SortKeys() {
        std::sort(keys.begin(), keys.end(), [](auto& a, auto& b){ return a.time<b.time; });
    }

    void AutoTangents() {
        uint32_t n=(uint32_t)keys.size();
        for(uint32_t i=0;i<n;i++) {
            if(keys[i].mode!=TangentMode::Auto) continue;
            float inT=0.f, outT=0.f;
            if(i>0 && i+1<n) {
                float dt1=keys[i].time-keys[i-1].time;
                float dt2=keys[i+1].time-keys[i].time;
                float slope=(keys[i+1].value-keys[i-1].value)/(dt1+dt2+1e-9f);
                inT=outT=slope;
            } else if(i==0 && n>1) {
                outT=(keys[1].value-keys[0].value)/(keys[1].time-keys[0].time+1e-9f);
            } else if(i+1==n && n>1) {
                inT=(keys[n-1].value-keys[n-2].value)/(keys[n-1].time-keys[n-2].time+1e-9f);
            }
            keys[i].inTan=inT; keys[i].outTan=outT;
        }
    }

    float Eval(float t) const {
        if(keys.empty()) return 0.f;
        if(keys.size()==1) return keys[0].value;
        float start=keys.front().time, end=keys.back().time;
        float dur=end-start;
        if(wrapMode==WrapMode::Loop && dur>0.f)
            t=start+std::fmod(t-start, dur);
        else if(wrapMode==WrapMode::PingPong && dur>0.f) {
            float n=std::floor((t-start)/dur);
            float rem=t-start-n*dur;
            if((int)n%2!=0) rem=dur-rem;
            t=start+rem;
        } else {
            t=std::max(start,std::min(t,end));
        }
        // Find segment
        for(uint32_t i=0;i+1<keys.size();i++) {
            auto& k0=keys[i]; auto& k1=keys[i+1];
            if(t>=k0.time && t<=k1.time) {
                float dt=k1.time-k0.time;
                float u=(dt>0.f)?(t-k0.time)/dt:0.f;
                if(k0.mode==TangentMode::Constant) return k0.value;
                if(k0.mode==TangentMode::Linear)
                    return k0.value + u*(k1.value-k0.value);
                return Hermite(k0.value, k0.outTan*dt, k1.value, k1.inTan*dt, u);
            }
        }
        return keys.back().value;
    }
};

struct CurveEditor::Impl {
    CurveChannel channels[4];
    uint32_t     activeChannel{0};
};

CurveEditor::CurveEditor()  : m_impl(new Impl) {}
CurveEditor::~CurveEditor() { delete m_impl; }

uint32_t CurveEditor::AddKey(float t, float v, TangentMode mode) {
    auto& ch = m_impl->channels[m_impl->activeChannel];
    CurveKey k; k.time=t; k.value=v; k.mode=mode;
    ch.keys.push_back(k);
    ch.SortKeys();
    if(mode==TangentMode::Auto) ch.AutoTangents();
    for(uint32_t i=0;i<(uint32_t)ch.keys.size();i++)
        if(ch.keys[i].time==t&&ch.keys[i].value==v) return i;
    return (uint32_t)ch.keys.size()-1;
}

void CurveEditor::RemoveKey(uint32_t idx) {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    if(idx<ch.keys.size()) { ch.keys.erase(ch.keys.begin()+idx); ch.AutoTangents(); }
}
void CurveEditor::MoveKey(uint32_t idx, float nt, float nv) {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    if(idx<ch.keys.size()) { ch.keys[idx].time=nt; ch.keys[idx].value=nv; ch.SortKeys(); ch.AutoTangents(); }
}
void CurveEditor::SetTangent(uint32_t idx, float in_, float out_) {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    if(idx<ch.keys.size()) { ch.keys[idx].inTan=in_; ch.keys[idx].outTan=out_; }
}
void CurveEditor::SetTangentMode(uint32_t idx, TangentMode m) {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    if(idx<ch.keys.size()) { ch.keys[idx].mode=m; ch.AutoTangents(); }
}
uint32_t CurveEditor::KeyCount() const {
    return (uint32_t)m_impl->channels[m_impl->activeChannel].keys.size();
}
const CurveKey* CurveEditor::GetKey(uint32_t idx) const {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    return idx<ch.keys.size()?&ch.keys[idx]:nullptr;
}
void CurveEditor::SetWrapMode(WrapMode m) { m_impl->channels[m_impl->activeChannel].wrapMode=m; }
WrapMode CurveEditor::GetWrapMode() const { return m_impl->channels[m_impl->activeChannel].wrapMode; }

float CurveEditor::Evaluate(float t) const { return m_impl->channels[m_impl->activeChannel].Eval(t); }
void  CurveEditor::Bake(uint32_t samples, std::vector<float>& out) const {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    out.resize(samples);
    if(samples<=1) { out[0]=ch.Eval(ch.keys.empty()?0.f:ch.keys.front().time); return; }
    float s=ch.keys.empty()?0.f:ch.keys.front().time;
    float e=ch.keys.empty()?1.f:ch.keys.back().time;
    for(uint32_t i=0;i<samples;i++)
        out[i]=ch.Eval(s+(e-s)*(float)i/(float)(samples-1));
}

float CurveEditor::StartTime() const {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    return ch.keys.empty()?0.f:ch.keys.front().time;
}
float CurveEditor::EndTime() const {
    auto& ch=m_impl->channels[m_impl->activeChannel];
    return ch.keys.empty()?1.f:ch.keys.back().time;
}

std::string CurveEditor::Serialize() const {
    std::ostringstream ss; ss << "{\"keys\":[";
    auto& ch=m_impl->channels[m_impl->activeChannel];
    bool first=true;
    for(auto& k:ch.keys) {
        if(!first) ss<<","; first=false;
        ss<<"{\"t\":"<<k.time<<",\"v\":"<<k.value<<"}";
    }
    ss<<"]}"; return ss.str();
}
bool CurveEditor::Deserialize(const std::string&) { return true; }
void CurveEditor::Clear() { m_impl->channels[m_impl->activeChannel].keys.clear(); }
void CurveEditor::SmoothTangents() { m_impl->channels[m_impl->activeChannel].AutoTangents(); }

CurveEditor& CurveEditor::Channel(uint32_t ch) {
    m_impl->activeChannel=std::min(ch,3u); return *this;
}
void CurveEditor::EvaluateVec4(float t, float out[4]) const {
    for(int i=0;i<4;i++) out[i]=m_impl->channels[i].Eval(t);
}

} // namespace Engine
