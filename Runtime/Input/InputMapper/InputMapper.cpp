#include "Runtime/Input/InputMapper/InputMapper.h"
#include <algorithm>

namespace Runtime::Input {

// ── Impl ─────────────────────────────────────────────────────────────────
struct InputMapper::Impl {
    std::unordered_map<std::string, ActionDef>   actions;
    std::unordered_map<std::string, ActionState> states;
    std::vector<ActionCallback>                  callbacks;
};

InputMapper::InputMapper() : m_impl(new Impl()) {}
InputMapper::~InputMapper() { delete m_impl; }

// ── Registration ─────────────────────────────────────────────────────────
void InputMapper::RegisterAction(const ActionDef& def) {
    m_impl->actions[def.name] = def;
    m_impl->states[def.name]  = {};
}

void InputMapper::RemoveAction(const std::string& name) {
    m_impl->actions.erase(name);
    m_impl->states.erase(name);
}

bool InputMapper::HasAction(const std::string& name) const {
    return m_impl->actions.count(name) > 0;
}

const ActionDef* InputMapper::GetAction(const std::string& name) const {
    auto it = m_impl->actions.find(name);
    return it != m_impl->actions.end() ? &it->second : nullptr;
}

std::vector<std::string> InputMapper::ActionNames() const {
    std::vector<std::string> out;
    for (const auto& [k,_] : m_impl->actions) out.push_back(k);
    return out;
}

// ── Rebinding ─────────────────────────────────────────────────────────────
bool InputMapper::RebindAction(const std::string& name, std::vector<Binding> bindings) {
    auto it = m_impl->actions.find(name);
    if (it == m_impl->actions.end()) return false;
    it->second.bindings = std::move(bindings);
    return true;
}

bool InputMapper::AddBinding(const std::string& name, const Binding& binding) {
    auto it = m_impl->actions.find(name);
    if (it == m_impl->actions.end()) return false;
    it->second.bindings.push_back(binding);
    return true;
}

bool InputMapper::RemoveBinding(const std::string& name, InputCode code) {
    auto it = m_impl->actions.find(name);
    if (it == m_impl->actions.end()) return false;
    auto& bindings = it->second.bindings;
    bindings.erase(std::remove_if(bindings.begin(), bindings.end(),
        [code](const Binding& b){ return b.code == code; }), bindings.end());
    return true;
}

// ── Update ────────────────────────────────────────────────────────────────
static bool codeInList(const std::vector<InputCode>& list, InputCode code) {
    for (auto c : list) if (c == code) return true;
    return false;
}

void InputMapper::Update(const RawInputFrame& frame) {
    for (auto& [name, def] : m_impl->actions) {
        ActionState& prev = m_impl->states[name];
        float newVal = 0.0f;

        for (const auto& b : def.bindings) {
            float contrib = 0.0f;
            switch (b.type) {
            case BindingType::Digital: {
                bool held = codeInList(frame.keysHeld, b.code);
                contrib = held ? 1.0f : 0.0f;
                break;
            }
            case BindingType::AnalogAxis: {
                auto it = frame.axes.find(b.code);
                if (it != frame.axes.end()) {
                    float v = it->second;
                    if (std::abs(v) < b.deadzone) v = 0.0f;
                    contrib = v * b.scale;
                }
                break;
            }
            case BindingType::CompositeAxis: {
                bool pos = codeInList(frame.keysHeld, b.code);
                bool neg = codeInList(frame.keysHeld, b.negCode);
                contrib = (pos ? 1.0f : 0.0f) - (neg ? 1.0f : 0.0f);
                break;
            }
            }
            if (b.invert) contrib = -contrib;
            // Take the binding with largest absolute contribution
            if (std::abs(contrib) > std::abs(newVal)) newVal = contrib;
        }

        ActionState cur;
        cur.value        = newVal;
        cur.held         = (newVal != 0.0f);
        cur.justPressed  = (!prev.held && cur.held);
        cur.justReleased = (prev.held  && !cur.held);

        bool changed = (cur.value != prev.value ||
                        cur.justPressed || cur.justReleased);
        m_impl->states[name] = cur;
        if (changed) {
            for (auto& cb : m_impl->callbacks) cb(name, cur);
        }
    }
}

// ── Query ─────────────────────────────────────────────────────────────────
bool  InputMapper::IsHeld(const std::string& a) const {
    auto it = m_impl->states.find(a);
    return it != m_impl->states.end() && it->second.held;
}
bool  InputMapper::IsJustPressed(const std::string& a) const {
    auto it = m_impl->states.find(a);
    return it != m_impl->states.end() && it->second.justPressed;
}
bool  InputMapper::IsJustReleased(const std::string& a) const {
    auto it = m_impl->states.find(a);
    return it != m_impl->states.end() && it->second.justReleased;
}
float InputMapper::AxisValue(const std::string& a) const {
    auto it = m_impl->states.find(a);
    return it != m_impl->states.end() ? it->second.value : 0.0f;
}
const ActionState* InputMapper::GetState(const std::string& a) const {
    auto it = m_impl->states.find(a);
    return it != m_impl->states.end() ? &it->second : nullptr;
}

void InputMapper::OnAction(ActionCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

// ── Serialisation ─────────────────────────────────────────────────────────
std::unordered_map<std::string,std::string> InputMapper::Serialize() const {
    std::unordered_map<std::string,std::string> out;
    for (const auto& [name, def] : m_impl->actions) {
        for (size_t i = 0; i < def.bindings.size(); ++i) {
            const auto& b = def.bindings[i];
            std::string key = name + "." + std::to_string(i) + ".code";
            out[key] = std::to_string(b.code);
            out[name + "." + std::to_string(i) + ".type"] =
                std::to_string(static_cast<int>(b.type));
            out[name + "." + std::to_string(i) + ".device"] =
                std::to_string(static_cast<int>(b.device));
        }
    }
    return out;
}

void InputMapper::Deserialize(const std::unordered_map<std::string,std::string>&) {
    // Minimal stub; full implementation would reconstruct ActionDefs from the map.
}

// ── Convenience ───────────────────────────────────────────────────────────
void InputMapper::MapKey(const std::string& actionName, InputCode keyCode) {
    Binding b;
    b.type   = BindingType::Digital;
    b.device = DeviceType::Keyboard;
    b.code   = keyCode;
    ActionDef def;
    def.name     = actionName;
    def.bindings = {b};
    def.isAnalog = false;
    RegisterAction(def);
}

void InputMapper::MapAxis(const std::string& actionName,
                           InputCode negKey, InputCode posKey)
{
    Binding b;
    b.type    = BindingType::CompositeAxis;
    b.device  = DeviceType::Keyboard;
    b.code    = posKey;
    b.negCode = negKey;
    ActionDef def;
    def.name     = actionName;
    def.bindings = {b};
    def.isAnalog = true;
    RegisterAction(def);
}

} // namespace Runtime::Input
