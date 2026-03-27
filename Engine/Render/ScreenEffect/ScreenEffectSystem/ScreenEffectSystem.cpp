#include "Engine/Render/ScreenEffect/ScreenEffectSystem/ScreenEffectSystem.h"
#include <algorithm>
#include <array>
#include <unordered_map>

namespace Engine {

struct EffectState {
    ScreenEffectDef def;
    std::unordered_map<std::string,float>                  floatParams;
    std::unordered_map<std::string,std::array<float,4>>    vec4Params;
};

struct ScreenEffectSystem::Impl {
    std::unordered_map<uint32_t,EffectState> effects;
};

ScreenEffectSystem::ScreenEffectSystem()  { m_impl = new Impl; }
ScreenEffectSystem::~ScreenEffectSystem() { delete m_impl; }

void ScreenEffectSystem::Init    () {}
void ScreenEffectSystem::Shutdown() { Reset(); }
void ScreenEffectSystem::Reset   () { m_impl->effects.clear(); }

bool ScreenEffectSystem::RegisterEffect(const ScreenEffectDef& def) {
    if (m_impl->effects.count(def.id)) return false;
    EffectState s; s.def = def;
    m_impl->effects[def.id] = std::move(s);
    return true;
}
void ScreenEffectSystem::RemoveEffect(uint32_t id) { m_impl->effects.erase(id); }

void ScreenEffectSystem::SetEffectEnabled(uint32_t id, bool on) {
    auto it = m_impl->effects.find(id); if (it != m_impl->effects.end()) it->second.def.enabled = on;
}
bool ScreenEffectSystem::IsEffectEnabled(uint32_t id) const {
    auto it = m_impl->effects.find(id); return it != m_impl->effects.end() && it->second.def.enabled;
}

void  ScreenEffectSystem::SetFloatParam(uint32_t id, const std::string& n, float v) {
    auto it = m_impl->effects.find(id); if (it != m_impl->effects.end()) it->second.floatParams[n] = v;
}
float ScreenEffectSystem::GetFloatParam(uint32_t id, const std::string& n, float def) const {
    auto it = m_impl->effects.find(id); if (it == m_impl->effects.end()) return def;
    auto it2 = it->second.floatParams.find(n); return it2 != it->second.floatParams.end() ? it2->second : def;
}
void ScreenEffectSystem::SetVec4Param(uint32_t id, const std::string& n,
                                       float r, float g, float b, float a) {
    auto it = m_impl->effects.find(id);
    if (it != m_impl->effects.end()) it->second.vec4Params[n] = {r,g,b,a};
}
void ScreenEffectSystem::GetVec4Param(uint32_t id, const std::string& n,
                                       float& r, float& g, float& b, float& a) const {
    auto it = m_impl->effects.find(id);
    if (it != m_impl->effects.end()) {
        auto it2 = it->second.vec4Params.find(n);
        if (it2 != it->second.vec4Params.end()) {
            r = it2->second[0]; g = it2->second[1];
            b = it2->second[2]; a = it2->second[3]; return;
        }
    }
    r = g = b = a = 0;
}

void ScreenEffectSystem::SetPriority(uint32_t id, int p) {
    auto it = m_impl->effects.find(id); if (it != m_impl->effects.end()) it->second.def.priority = p;
}
uint32_t ScreenEffectSystem::GetEffectOrder(std::vector<uint32_t>& out) const {
    out.clear();
    std::vector<const EffectState*> sorted;
    for (auto& [id, s] : m_impl->effects) sorted.push_back(&s);
    std::sort(sorted.begin(), sorted.end(),
        [](const EffectState* a, const EffectState* b){ return a->def.priority < b->def.priority; });
    for (auto* s : sorted) out.push_back(s->def.id);
    return (uint32_t)out.size();
}
void ScreenEffectSystem::Execute(std::function<void(uint32_t)> backend) {
    if (!backend) return;
    std::vector<uint32_t> order; GetEffectOrder(order);
    for (uint32_t id : order)
        if (IsEffectEnabled(id)) backend(id);
}
uint32_t ScreenEffectSystem::GetEffectCount() const { return (uint32_t)m_impl->effects.size(); }
uint32_t ScreenEffectSystem::GetActiveCount() const {
    uint32_t n = 0;
    for (auto& [id, s] : m_impl->effects) if (s.def.enabled) ++n;
    return n;
}

} // namespace Engine
