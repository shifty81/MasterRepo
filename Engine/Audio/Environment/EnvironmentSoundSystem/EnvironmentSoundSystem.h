#pragma once
/**
 * @file EnvironmentSoundSystem.h
 * @brief Zone-based ambient/environment sounds: zones, listener, crossfade, reverb.
 *
 * Features:
 *   - ZoneShape: Sphere, Box
 *   - RegisterZone(zoneId, shape, cx, cy, cz, halfW, halfH, halfD) → bool
 *   - RemoveZone(zoneId)
 *   - SetZoneSound(zoneId, clipName, loop, volume)
 *   - SetZoneReverb(zoneId, reverbPreset)
 *   - SetZonePriority(zoneId, p): higher = more audible on overlap
 *   - SetListenerPosition(x, y, z)
 *   - Tick(dt): evaluate which zones the listener is in, crossfade, reverb
 *   - GetActiveZones(out[]) → uint32_t: zones containing listener
 *   - GetZoneWeight(zoneId) → float: 0=outside, 1=centre
 *   - SetCrossfadeDuration(secs)
 *   - SetOnZoneEnter(cb) / SetOnZoneExit(cb)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class EnvZoneShape : uint8_t { Sphere, Box };

class EnvironmentSoundSystem {
public:
    EnvironmentSoundSystem();
    ~EnvironmentSoundSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Zone management
    bool RegisterZone(uint32_t zoneId, EnvZoneShape shape,
                      float cx, float cy, float cz,
                      float halfW, float halfH, float halfD);
    void RemoveZone  (uint32_t zoneId);

    // Zone properties
    void SetZoneSound   (uint32_t zoneId, const std::string& clip,
                         bool loop, float volume = 1.f);
    void SetZoneReverb  (uint32_t zoneId, uint32_t reverbPreset);
    void SetZonePriority(uint32_t zoneId, int priority);

    // Listener
    void SetListenerPosition(float x, float y, float z);

    // Per-frame
    void Tick(float dt);

    // Query
    uint32_t GetActiveZones(std::vector<uint32_t>& out) const;
    float    GetZoneWeight (uint32_t zoneId) const;

    // Config
    void SetCrossfadeDuration(float secs);

    // Callbacks
    void SetOnZoneEnter(std::function<void(uint32_t zoneId)> cb);
    void SetOnZoneExit (std::function<void(uint32_t zoneId)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
