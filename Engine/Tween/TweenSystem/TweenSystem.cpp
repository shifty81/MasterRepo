#include "Engine/Tween/TweenSystem/TweenSystem.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Engine {

// ── Easing ──────────────────────────────────────────────────────────────────

static float EaseBack(float t, bool in, bool out) {
    const float s = 1.70158f;
    if (in && !out) return t*t*((s+1)*t - s);
    if (!in && out) { t-=1; return t*t*((s+1)*t+s)+1; }
    // inout
    t*=2; if (t<1) return 0.5f*(t*t*(((s*1.525f)+1)*t-s*1.525f));
    t-=2; return 0.5f*(t*t*(((s*1.525f)+1)*t+s*1.525f)+2);
}

static float BounceOut(float t) {
    if (t < 1/2.75f)      return 7.5625f*t*t;
    if (t < 2/2.75f)    { t-=1.5f/2.75f;   return 7.5625f*t*t+0.75f; }
    if (t < 2.5f/2.75f) { t-=2.25f/2.75f;  return 7.5625f*t*t+0.9375f; }
    t-=2.625f/2.75f; return 7.5625f*t*t+0.984375f;
}

float TweenSystem::Evaluate(Ease ease, float t)
{
    t = std::max(0.f, std::min(1.f, t));
    switch(ease) {
    case Ease::Linear:       return t;
    case Ease::QuadIn:       return t*t;
    case Ease::QuadOut:      return t*(2-t);
    case Ease::QuadInOut:    return t<0.5f ? 2*t*t : -1+(4-2*t)*t;
    case Ease::CubicIn:      return t*t*t;
    case Ease::CubicOut:     { float q=t-1; return q*q*q+1; }
    case Ease::CubicInOut:   return t<0.5f ? 4*t*t*t : (t-1)*(2*t-2)*(2*t-2)+1;
    case Ease::SineIn:       return 1-std::cos(t*(float)M_PI*0.5f);
    case Ease::SineOut:      return std::sin(t*(float)M_PI*0.5f);
    case Ease::SineInOut:    return -(std::cos((float)M_PI*t)-1)*0.5f;
    case Ease::ExpoIn:       return t==0?0:std::pow(2.f,10*(t-1));
    case Ease::ExpoOut:      return t==1?1:1-std::pow(2.f,-10*t);
    case Ease::ExpoInOut:    if(t==0)return 0; if(t==1)return 1;
                             return t<0.5f?std::pow(2.f,20*t-10)*0.5f:(2-std::pow(2.f,-20*t+10))*0.5f;
    case Ease::CircIn:       return 1-std::sqrt(1-t*t);
    case Ease::CircOut:      { float c=t-1; return std::sqrt(1-c*c); }
    case Ease::CircInOut:    return t<0.5f?(1-std::sqrt(1-4*t*t))*0.5f:(std::sqrt(1-((-2*t+2)*(-2*t+2)))+1)*0.5f;
    case Ease::BackIn:       return EaseBack(t,true,false);
    case Ease::BackOut:      return EaseBack(t,false,true);
    case Ease::BackInOut:    return EaseBack(t,true,true);
    case Ease::BounceIn:     return 1-BounceOut(1-t);
    case Ease::BounceOut:    return BounceOut(t);
    case Ease::BounceInOut:  return t<0.5f?(1-BounceOut(1-2*t))*0.5f:(1+BounceOut(2*t-1))*0.5f;
    case Ease::ElasticIn:
    {
        if(t==0)return 0; if(t==1)return 1;
        return -std::pow(2.f,10*t-10)*std::sin((t*10-10.75f)*(float)(2*M_PI)/3.f);
    }
    case Ease::ElasticOut:
    {
        if(t==0)return 0; if(t==1)return 1;
        return std::pow(2.f,-10*t)*std::sin((t*10-0.75f)*(float)(2*M_PI)/3.f)+1;
    }
    case Ease::ElasticInOut:
    {
        if(t==0)return 0; if(t==1)return 1;
        if(t<0.5f) return -(std::pow(2.f,20*t-10)*std::sin((20*t-11.125f)*(float)(2*M_PI)/4.5f))*0.5f;
        return std::pow(2.f,-20*t+10)*std::sin((20*t-11.125f)*(float)(2*M_PI)/4.5f)*0.5f+1;
    }
    default: return t;
    }
}

// ── Impl ────────────────────────────────────────────────────────────────────

struct TweenEntry {
    uint32_t  id{0};
    TweenDesc desc;
    float     elapsed{0.f};
    float     from[4]{};
    bool      started{false};
    bool      alive{true};
    bool      paused{false};
    int32_t   loopsRemaining{0};
    bool      yoyoForward{true};

    std::function<void()>         onComplete;
    std::function<void(float)>    onUpdate;
};

struct SequenceEntry {
    uint32_t              seqId{0};
    std::vector<uint32_t> tweenIds;
    uint32_t              currentIdx{0};
    bool                  playing{false};
};

struct TweenSystem::Impl {
    std::vector<TweenEntry>    tweens;
    std::vector<SequenceEntry> sequences;
    uint32_t nextId{1};
    uint32_t nextSeqId{0x80000000u};

    TweenEntry* Find(uint32_t id) {
        for (auto& e : tweens) if (e.id == id) return &e;
        return nullptr;
    }
};

TweenSystem::TweenSystem()  : m_impl(new Impl) {}
TweenSystem::~TweenSystem() { Shutdown(); delete m_impl; }

void TweenSystem::Init()     {}
void TweenSystem::Shutdown() { m_impl->tweens.clear(); }

uint32_t TweenSystem::To(float* target, float toVal, float duration, Ease ease)
{
    TweenDesc d; d.target=target; d.numComponents=1;
    d.to[0]=toVal; d.duration=duration; d.ease=ease;
    return Play(d);
}

uint32_t TweenSystem::From(float* target, float fromVal, float duration, Ease ease)
{
    TweenDesc d; d.target=target; d.numComponents=1;
    d.from[0]=fromVal; d.useFrom=true; d.to[0]=*target;
    d.duration=duration; d.ease=ease;
    return Play(d);
}

uint32_t TweenSystem::ToVec(float* target, const float* toVals, uint32_t n,
                              float duration, Ease ease)
{
    TweenDesc d; d.target=target; d.numComponents=std::min(n,4u);
    for (uint32_t i=0;i<d.numComponents;i++) d.to[i]=toVals[i];
    d.duration=duration; d.ease=ease;
    return Play(d);
}

uint32_t TweenSystem::Play(const TweenDesc& desc)
{
    TweenEntry e;
    e.id   = m_impl->nextId++;
    e.desc = desc;
    e.alive= true;
    e.loopsRemaining = desc.loopCount;
    if (desc.useFrom) {
        for (uint32_t i=0;i<desc.numComponents;i++) e.from[i]=desc.from[i];
    }
    m_impl->tweens.push_back(e);
    return e.id;
}

TweenSystem& TweenSystem::Delay(uint32_t id, float delay)
{
    auto* e = m_impl->Find(id);
    if (e) e->desc.delay=delay;
    return *this;
}

TweenSystem& TweenSystem::Loop(uint32_t id, LoopMode mode, int32_t count)
{
    auto* e = m_impl->Find(id);
    if (e) { e->desc.loop=mode; e->desc.loopCount=count; e->loopsRemaining=count; }
    return *this;
}

TweenSystem& TweenSystem::OnComplete(uint32_t id, std::function<void()> cb)
{
    auto* e = m_impl->Find(id);
    if (e) e->onComplete = cb;
    return *this;
}

TweenSystem& TweenSystem::OnUpdate(uint32_t id, std::function<void(float)> cb)
{
    auto* e = m_impl->Find(id);
    if (e) e->onUpdate = cb;
    return *this;
}

void TweenSystem::Tick(float dt)
{
    for (auto& e : m_impl->tweens) {
        if (!e.alive || e.paused) continue;

        e.elapsed += dt;
        if (e.elapsed < e.desc.delay) continue;

        float t = e.elapsed - e.desc.delay;

        // Capture start value
        if (!e.started) {
            e.started = true;
            if (!e.desc.useFrom && e.desc.target) {
                for (uint32_t i=0;i<e.desc.numComponents;i++) e.from[i]=e.desc.target[i];
            }
        }

        float prog = (e.desc.duration > 0.f) ? std::min(1.f, t / e.desc.duration) : 1.f;
        float effProg = e.yoyoForward ? prog : 1.f-prog;
        float ep = Evaluate(e.desc.ease, effProg);

        if (e.desc.target) {
            for (uint32_t i=0;i<e.desc.numComponents;i++)
                e.desc.target[i] = e.from[i] + (e.desc.to[i]-e.from[i])*ep;
        }
        if (e.onUpdate) e.onUpdate(effProg);

        if (prog >= 1.f) {
            if (e.desc.loop != LoopMode::None &&
                (e.loopsRemaining > 0 || e.desc.loopCount == 0)) {
                if (e.desc.loopCount > 0) e.loopsRemaining--;
                e.elapsed = e.desc.delay; // restart
                if (e.desc.loop == LoopMode::Yoyo) e.yoyoForward = !e.yoyoForward;
            } else {
                if (e.onComplete) e.onComplete();
                e.alive = false;
            }
        }
    }

    auto& v = m_impl->tweens;
    v.erase(std::remove_if(v.begin(),v.end(),[](auto& e){ return !e.alive; }), v.end());
}

void TweenSystem::Pause(uint32_t id)  { auto* e=m_impl->Find(id); if(e) e->paused=true; }
void TweenSystem::Resume(uint32_t id) { auto* e=m_impl->Find(id); if(e) e->paused=false; }
void TweenSystem::Kill(uint32_t id)   { auto* e=m_impl->Find(id); if(e) e->alive=false; }

void TweenSystem::KillAll(float* target)
{
    for (auto& e : m_impl->tweens)
        if (!target || e.desc.target==target) e.alive=false;
}

bool TweenSystem::IsAlive(uint32_t id) const
{
    for (auto& e : m_impl->tweens) if (e.id==id) return e.alive;
    return false;
}

uint32_t TweenSystem::CreateSequence()
{
    SequenceEntry se; se.seqId = m_impl->nextSeqId++;
    m_impl->sequences.push_back(se);
    return se.seqId;
}

void TweenSystem::SequenceAppend(uint32_t seqId, uint32_t tweenId)
{
    for (auto& s : m_impl->sequences)
        if (s.seqId==seqId) { s.tweenIds.push_back(tweenId); return; }
}

void TweenSystem::SequencePlay(uint32_t /*seqId*/) {}

uint32_t TweenSystem::ActiveCount() const
{
    uint32_t n=0;
    for (auto& e : m_impl->tweens) if (e.alive) n++;
    return n;
}

} // namespace Engine
