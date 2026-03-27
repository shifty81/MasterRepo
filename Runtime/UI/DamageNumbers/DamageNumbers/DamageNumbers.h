#pragma once
/**
 * @file DamageNumbers.h
 * @brief Floating damage/heal numbers: spawn, animate, pool, colour rules.
 *
 * Features:
 *   - Spawn(pos, value, type): add a floating number at world position
 *   - Tick(dt): advance animations, expire entries
 *   - Render(cb): iterate active entries, calls cb(entry) for external draw
 *   - SetLifetime(seconds): default entry lifetime
 *   - SetFloatSpeed(v): upward drift speed (world units/s)
 *   - SetFadeStartTime(t): fraction of lifetime at which alpha fades
 *   - SetScalePeak(scale): peak scale factor at animation midpoint
 *   - SetColourRule(type, colour): assign colour to damage type enum
 *   - SetCriticalScale(multiplier): extra scale for critical hits
 *   - GetActiveCount() → uint32_t
 *   - Clear()
 *   - SetPoolSize(n): pre-allocate entry pool
 *   - DamageType: Normal, Critical, Heal, Miss, Block, Absorb
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Runtime {

enum class DamageType : uint8_t { Normal, Critical, Heal, Miss, Block, Absorb };

struct DNVec3  { float x,y,z; };
struct DNColour { float r,g,b,a; };

struct DamageNumberEntry {
    uint32_t    id;
    DNVec3      pos;
    float       value;
    DamageType  type;
    float       age;
    float       lifetime;
    float       alpha;
    float       scale;
    DNColour    colour;
};

class DamageNumbers {
public:
    DamageNumbers();
    ~DamageNumbers();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Clear   ();

    // Spawn
    uint32_t Spawn(DNVec3 pos, float value, DamageType type=DamageType::Normal);

    // Per-frame
    void Tick  (float dt);
    void Render(std::function<void(const DamageNumberEntry&)> drawCb) const;

    // Config
    void SetLifetime    (float seconds);
    void SetFloatSpeed  (float v);
    void SetFadeStartTime(float t);
    void SetScalePeak   (float scale);
    void SetCriticalScale(float mult);
    void SetColourRule  (DamageType type, DNColour col);
    void SetPoolSize    (uint32_t n);

    // Query
    uint32_t GetActiveCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
