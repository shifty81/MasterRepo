#pragma once
/**
 * @file ScreenEffectSystem.h
 * @brief Fullscreen post-process effect pipeline: register, order, enable, params.
 *
 * Features:
 *   - EffectDef: id, name, priority (lower = earlier pass)
 *   - RegisterEffect(def) → bool
 *   - RemoveEffect(id)
 *   - SetEffectEnabled(id, on)
 *   - IsEffectEnabled(id) → bool
 *   - SetFloatParam(id, name, v) / GetFloatParam(id, name) → float
 *   - SetVec4Param(id, name, r,g,b,a) / GetVec4Param(id, name, r&,g&,b&,a&)
 *   - SetPriority(id, priority): re-order in pipeline
 *   - GetEffectOrder(out[]) → uint32_t: sorted effect IDs
 *   - Execute(backend): iterate enabled effects in priority order, call backend per effect
 *   - GetEffectCount() → uint32_t (total registered)
 *   - GetActiveCount() → uint32_t (enabled only)
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct ScreenEffectDef {
    uint32_t    id;
    std::string name;
    int         priority{0};
    bool        enabled{true};
};

class ScreenEffectSystem {
public:
    ScreenEffectSystem();
    ~ScreenEffectSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Effect registration
    bool RegisterEffect(const ScreenEffectDef& def);
    void RemoveEffect  (uint32_t id);

    // Enable/disable
    void SetEffectEnabled(uint32_t id, bool on);
    bool IsEffectEnabled (uint32_t id) const;

    // Parameters
    void  SetFloatParam(uint32_t id, const std::string& name, float v);
    float GetFloatParam(uint32_t id, const std::string& name,
                        float def = 0.f) const;
    void SetVec4Param  (uint32_t id, const std::string& name,
                        float r, float g, float b, float a);
    void GetVec4Param  (uint32_t id, const std::string& name,
                        float& r, float& g, float& b, float& a) const;

    // Ordering
    void     SetPriority   (uint32_t id, int priority);
    uint32_t GetEffectOrder(std::vector<uint32_t>& out) const;

    // Execute
    void Execute(std::function<void(uint32_t effectId)> backend);

    // Query
    uint32_t GetEffectCount() const;
    uint32_t GetActiveCount() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
