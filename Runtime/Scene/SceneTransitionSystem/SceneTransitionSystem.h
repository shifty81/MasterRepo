#pragma once
/**
 * @file SceneTransitionSystem.h
 * @brief Fade-in/out scene transitions with async loading and additive/replace modes.
 *
 * Features:
 *   - Transition modes: Replace (unload current + load next) or Additive (keep current)
 *   - Configurable fade-out → hold → load → fade-in timeline
 *   - Async scene load during black-screen hold phase
 *   - Custom transition effects: Fade, Crossfade, Slide, Iris
 *   - On-progress callback (0-1) for loading screens
 *   - On-complete callback fired when new scene is fully visible
 *   - Pre-load without transition (background streaming)
 *   - Cancel in-progress transition
 *   - Scene stack for push/pop (additive overlay scenes)
 *   - Integrates with TimeManager to pause game during transition
 *
 * Typical usage:
 * @code
 *   SceneTransitionSystem sts;
 *   sts.Init();
 *   sts.SetLoadFn([](const std::string& name, ProgressFn p){ LoadScene(name,p); });
 *   sts.SetUnloadFn([](){ UnloadCurrentScene(); });
 *   sts.TransitionTo("Level2", {TransitionMode::Replace, 0.4f, 0.3f, 0.4f});
 *   sts.SetOnComplete([](){ HideLoadingScreen(); });
 *   // per-frame:
 *   sts.Tick(dt);
 *   float alpha = sts.GetFadeAlpha();  // 0=transparent, 1=black
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class TransitionMode   : uint8_t { Replace=0, Additive, PopAdditive };
enum class TransitionEffect : uint8_t { Fade=0, Crossfade, SlideLeft, SlideRight, Iris };
enum class TransitionPhase  : uint8_t { Idle=0, FadeOut, Loading, FadeIn, Complete };

using SceneProgressFn = std::function<void(float)>;

struct TransitionDesc {
    std::string      targetScene;
    TransitionMode   mode      {TransitionMode::Replace};
    TransitionEffect effect    {TransitionEffect::Fade};
    float            fadeOutDur{0.4f};
    float            holdDur   {0.1f};
    float            fadeInDur {0.4f};
};

class SceneTransitionSystem {
public:
    SceneTransitionSystem();
    ~SceneTransitionSystem();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Integration
    using LoadFn   = std::function<void(const std::string& sceneName, SceneProgressFn progress)>;
    using UnloadFn = std::function<void()>;
    void SetLoadFn  (LoadFn   fn);
    void SetUnloadFn(UnloadFn fn);

    // Trigger transitions
    void TransitionTo(const std::string& sceneName, const TransitionDesc& desc);
    void TransitionTo(const std::string& sceneName,
                      TransitionMode mode=TransitionMode::Replace,
                      float fadeOut=0.4f, float fadeIn=0.4f);

    // Scene stack (additive)
    void PushScene(const std::string& sceneName, float fadeOut=0.3f, float fadeIn=0.3f);
    void PopScene (float fadeOut=0.3f, float fadeIn=0.3f);
    void PreloadScene(const std::string& sceneName, SceneProgressFn progress={});

    // Cancel
    void CancelTransition();

    // State query
    TransitionPhase GetPhase()     const;
    float           GetFadeAlpha() const; ///< 0=transparent 1=fully covered
    float           GetProgress()  const; ///< load progress 0–1
    bool            IsTransitioning() const;
    std::string     GetCurrentScene() const;
    std::string     GetTargetScene()  const;

    // Callbacks
    void SetOnFadeOutComplete(std::function<void()>     cb);
    void SetOnLoadProgress   (SceneProgressFn           cb);
    void SetOnComplete       (std::function<void()>     cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
