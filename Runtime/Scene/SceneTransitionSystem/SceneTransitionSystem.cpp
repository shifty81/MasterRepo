#include "Runtime/Scene/SceneTransitionSystem/SceneTransitionSystem.h"
#include <algorithm>
#include <cmath>

namespace Runtime {

struct SceneTransitionSystem::Impl {
    LoadFn   loadFn;
    UnloadFn unloadFn;

    TransitionPhase phase{TransitionPhase::Idle};
    TransitionDesc  activeDesc;
    float           phaseTimer{0.f};
    float           phaseDur{0.f};
    float           fadeAlpha{0.f};
    float           loadProgress{0.f};
    std::string     currentScene;
    std::vector<std::string> sceneStack;

    std::function<void()>   onFadeOutCb;
    SceneProgressFn         onProgressCb;
    std::function<void()>   onCompleteCb;

    void StartPhase(TransitionPhase p, float dur) {
        phase = p; phaseTimer=0.f; phaseDur=dur;
    }
};

SceneTransitionSystem::SceneTransitionSystem()  : m_impl(new Impl) {}
SceneTransitionSystem::~SceneTransitionSystem() { Shutdown(); delete m_impl; }

void SceneTransitionSystem::Init()     {}
void SceneTransitionSystem::Shutdown() {}

void SceneTransitionSystem::SetLoadFn  (LoadFn fn)   { m_impl->loadFn   = fn; }
void SceneTransitionSystem::SetUnloadFn(UnloadFn fn) { m_impl->unloadFn = fn; }

void SceneTransitionSystem::TransitionTo(const std::string& sceneName,
                                          const TransitionDesc& desc)
{
    m_impl->activeDesc        = desc;
    m_impl->activeDesc.targetScene = sceneName;
    m_impl->loadProgress      = 0.f;
    m_impl->StartPhase(TransitionPhase::FadeOut, desc.fadeOutDur);
}

void SceneTransitionSystem::TransitionTo(const std::string& sceneName,
                                          TransitionMode mode,
                                          float fadeOut, float fadeIn)
{
    TransitionDesc d; d.targetScene=sceneName; d.mode=mode;
    d.fadeOutDur=fadeOut; d.holdDur=0.1f; d.fadeInDur=fadeIn;
    TransitionTo(sceneName, d);
}

void SceneTransitionSystem::PushScene(const std::string& sceneName, float fadeOut, float fadeIn)
{
    if (!m_impl->currentScene.empty()) m_impl->sceneStack.push_back(m_impl->currentScene);
    TransitionTo(sceneName, TransitionMode::Additive, fadeOut, fadeIn);
}

void SceneTransitionSystem::PopScene(float fadeOut, float fadeIn)
{
    if (m_impl->sceneStack.empty()) return;
    std::string prev = m_impl->sceneStack.back(); m_impl->sceneStack.pop_back();
    TransitionTo(prev, TransitionMode::Replace, fadeOut, fadeIn);
}

void SceneTransitionSystem::PreloadScene(const std::string& sceneName, SceneProgressFn progress)
{
    if (m_impl->loadFn) m_impl->loadFn(sceneName, progress);
}

void SceneTransitionSystem::CancelTransition()
{
    m_impl->phase     = TransitionPhase::Idle;
    m_impl->fadeAlpha = 0.f;
}

void SceneTransitionSystem::Tick(float dt)
{
    if (m_impl->phase == TransitionPhase::Idle) return;

    m_impl->phaseTimer += dt;
    float t = m_impl->phaseDur>0.f ? std::min(1.f, m_impl->phaseTimer/m_impl->phaseDur) : 1.f;

    switch (m_impl->phase) {
    case TransitionPhase::FadeOut:
        m_impl->fadeAlpha = t;
        if (t >= 1.f) {
            if (m_impl->onFadeOutCb) m_impl->onFadeOutCb();
            // Unload
            if (m_impl->activeDesc.mode==TransitionMode::Replace && m_impl->unloadFn)
                m_impl->unloadFn();
            // Start loading
            if (m_impl->loadFn) {
                m_impl->loadFn(m_impl->activeDesc.targetScene, [this](float p){
                    m_impl->loadProgress = p;
                    if (m_impl->onProgressCb) m_impl->onProgressCb(p);
                });
            }
            m_impl->loadProgress = 1.f;
            m_impl->StartPhase(TransitionPhase::Loading, m_impl->activeDesc.holdDur);
        }
        break;
    case TransitionPhase::Loading:
        if (t >= 1.f) {
            m_impl->currentScene = m_impl->activeDesc.targetScene;
            m_impl->StartPhase(TransitionPhase::FadeIn, m_impl->activeDesc.fadeInDur);
        }
        break;
    case TransitionPhase::FadeIn:
        m_impl->fadeAlpha = 1.f - t;
        if (t >= 1.f) {
            m_impl->fadeAlpha = 0.f;
            m_impl->phase     = TransitionPhase::Idle;
            if (m_impl->onCompleteCb) m_impl->onCompleteCb();
        }
        break;
    default: break;
    }
}

TransitionPhase SceneTransitionSystem::GetPhase()     const { return m_impl->phase; }
float           SceneTransitionSystem::GetFadeAlpha() const { return m_impl->fadeAlpha; }
float           SceneTransitionSystem::GetProgress()  const { return m_impl->loadProgress; }
bool            SceneTransitionSystem::IsTransitioning() const { return m_impl->phase!=TransitionPhase::Idle; }
std::string     SceneTransitionSystem::GetCurrentScene() const { return m_impl->currentScene; }
std::string     SceneTransitionSystem::GetTargetScene()  const { return m_impl->activeDesc.targetScene; }

void SceneTransitionSystem::SetOnFadeOutComplete(std::function<void()> cb) { m_impl->onFadeOutCb=cb; }
void SceneTransitionSystem::SetOnLoadProgress   (SceneProgressFn cb)       { m_impl->onProgressCb=cb; }
void SceneTransitionSystem::SetOnComplete       (std::function<void()> cb) { m_impl->onCompleteCb=cb; }

} // namespace Runtime
