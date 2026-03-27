#pragma once
/**
 * @file AnimNotifySystem.h
 * @brief Animation event notification system — fires callbacks at keyframe timestamps.
 *
 * Features:
 *   - Register named notify events within animation clips (by clip name + time offset)
 *   - Fire callbacks when playhead crosses a notify timestamp
 *   - Notify states: window-based (enter/tick/exit) for sustained effects
 *   - Multiple subscriber callbacks per event
 *   - Scrub-safe: skips or fires missed notifies after a seek
 *   - Notify groups for bulk enable/disable (e.g. "footstep", "combat", "vfx")
 *   - Per-actor playback instances (multiple actors playing same clip independently)
 *
 * Typical usage:
 * @code
 *   AnimNotifySystem ans;
 *   ans.Init();
 *   ans.RegisterNotify({"walk_cycle", "step_left",  0.25f, "footstep"});
 *   ans.RegisterNotify({"walk_cycle", "step_right", 0.75f, "footstep"});
 *   ans.RegisterStateNotify({"swing", "trail_fx", 0.1f, 0.6f, "vfx"});
 *   ans.Subscribe("step_left",  [](const NotifyEvent& e){ Footstep(e.actorId, Left); });
 *   uint32_t inst = ans.CreateInstance(actorId, "walk_cycle", totalLen);
 *   // per-frame
 *   ans.AdvanceInstance(inst, dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct NotifyDef {
    std::string clipName;
    std::string notifyName;    ///< event identifier
    float       time{0.f};     ///< normalised [0,1] or seconds (see useNormalized)
    std::string group;         ///< e.g. "footstep"
    bool        useNormalized{true};
};

struct NotifyStateDef {
    std::string clipName;
    std::string notifyName;
    float       startTime{0.f};
    float       endTime{0.5f};
    std::string group;
    bool        useNormalized{true};
};

struct NotifyEvent {
    uint32_t    actorId{0};
    uint32_t    instanceId{0};
    std::string clipName;
    std::string notifyName;
    float       clipTime{0.f};   ///< seconds into clip
    float       clipLength{0.f};
};

struct NotifyStateEvent {
    uint32_t    actorId{0};
    uint32_t    instanceId{0};
    std::string notifyName;
    float       elapsedInState{0.f};
};

enum class NotifyStatePhase : uint8_t { Enter=0, Tick, Exit };

using NotifyCb      = std::function<void(const NotifyEvent&)>;
using NotifyStateCb = std::function<void(const NotifyStateEvent&, NotifyStatePhase)>;

class AnimNotifySystem {
public:
    AnimNotifySystem();
    ~AnimNotifySystem();

    void Init();
    void Shutdown();

    // Notify registration (clip definitions)
    void RegisterNotify(const NotifyDef& def);
    void RegisterStateNotify(const NotifyStateDef& def);
    void UnregisterNotify(const std::string& clipName, const std::string& notifyName);
    void ClearClip(const std::string& clipName);

    // Group control
    void EnableGroup(const std::string& group);
    void DisableGroup(const std::string& group);

    // Subscriptions
    uint32_t Subscribe(const std::string& notifyName, NotifyCb cb);
    uint32_t SubscribeState(const std::string& notifyName, NotifyStateCb cb);
    void     Unsubscribe(uint32_t subId);

    // Instance management (per-actor playback)
    uint32_t CreateInstance(uint32_t actorId, const std::string& clipName,
                            float clipLength);
    void     DestroyInstance(uint32_t instanceId);
    void     AdvanceInstance(uint32_t instanceId, float dt);
    void     SeekInstance(uint32_t instanceId, float time);
    void     SetInstanceLoop(uint32_t instanceId, bool loop);
    void     SetInstanceSpeed(uint32_t instanceId, float speed);

    // Manual tick (if not using per-instance advance)
    void Tick(float dt);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
