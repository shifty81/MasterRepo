#pragma once
/**
 * @file TriggerVolume.h
 * @brief AABB / sphere / capsule overlap volumes with enter / exit / stay callbacks.
 *
 * Features:
 *   - Primitive shapes: AABB, sphere, capsule, oriented box
 *   - Overlap test against registered actor positions (no full physics needed)
 *   - On-enter, on-stay (each frame), on-exit callbacks per volume
 *   - Filter by entity tag or entity layer mask
 *   - One-shot volumes: deactivate after first trigger
 *   - Re-trigger cooldown per actor
 *   - Nested trigger support (multiple overlapping volumes correctly tracked)
 *   - Enable/disable individual volumes at runtime
 *   - JSON description loading
 *
 * Typical usage:
 * @code
 *   TriggerVolume tv;
 *   tv.Init();
 *   uint32_t vid = tv.AddAABB("zone_a", {0,0,0}, {5,2,5});
 *   tv.SetOnEnter(vid, [](uint32_t actor){ StartCutscene(actor); });
 *   tv.SetOnExit (vid, [](uint32_t actor){ EndCutscene(actor); });
 *   // per-frame
 *   tv.SetActorPosition(playerId, playerPos);
 *   tv.Tick();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class TriggerShape : uint8_t { AABB=0, Sphere, Capsule, OBB };

struct TriggerVolumeDesc {
    std::string    id;
    TriggerShape   shape{TriggerShape::AABB};
    float          centre[3]{};
    float          halfExtents[3]{1,1,1}; ///< for AABB; [0]=radius, [1]=halfHeight for Capsule
    float          rotation[4]{0,0,0,1};  ///< quaternion, for OBB
    bool           oneShot{false};
    float          cooldown{0.f};         ///< per-actor retrigger cooldown (0=off)
    std::string    filterTag;             ///< only trigger for actors with this tag
    bool           enabled{true};
};

using TriggerCb = std::function<void(uint32_t actorId)>;
using TriggerStayCb = std::function<void(uint32_t actorId, float timeInVolume)>;

class TriggerVolume {
public:
    TriggerVolume();
    ~TriggerVolume();

    void Init();
    void Shutdown();
    void Tick();

    // Volume management
    uint32_t AddAABB(const std::string& id, const float centre[3],
                      const float halfExtents[3]);
    uint32_t AddSphere(const std::string& id, const float centre[3], float radius);
    uint32_t AddCapsule(const std::string& id, const float centre[3],
                         float radius, float halfHeight, const float upAxis[3]);
    uint32_t Add(const TriggerVolumeDesc& desc);
    void     Remove(uint32_t volumeId);
    void     Remove(const std::string& id);
    void     Enable(uint32_t volumeId, bool enabled);
    bool     HasVolume(const std::string& id) const;
    uint32_t GetVolumeId(const std::string& id) const;

    // Move volumes at runtime
    void     SetCentre(uint32_t volumeId, const float centre[3]);

    // Callbacks
    void SetOnEnter(uint32_t volumeId, TriggerCb cb);
    void SetOnExit (uint32_t volumeId, TriggerCb cb);
    void SetOnStay (uint32_t volumeId, TriggerStayCb cb);

    // Actor tracking
    void SetActorPosition(uint32_t actorId, const float pos[3]);
    void SetActorTag(uint32_t actorId, const std::string& tag);
    void RemoveActor(uint32_t actorId);

    // Query
    bool IsActorInVolume(uint32_t actorId, uint32_t volumeId) const;
    std::vector<uint32_t> GetActorsInVolume(uint32_t volumeId) const;
    std::vector<uint32_t> GetVolumesForActor(uint32_t actorId) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
