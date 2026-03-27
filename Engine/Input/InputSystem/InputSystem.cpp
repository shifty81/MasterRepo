#include "Engine/Input/InputSystem/InputSystem.h"
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Engine {

static const uint32_t kKeyCount = (uint32_t)InputKey::COUNT;

struct CbEntry {
    uint32_t  id{0};
    std::string action;
    uint8_t   type{0}; // 0=press 1=release 2=axis
    std::function<void()>      cbVoid;
    std::function<void(float)> cbAxis;
};

struct InputSystem::Impl {
    // Key states: 0=up, 1=down, 2=just-pressed, 3=just-released
    uint8_t  cur [kKeyCount]{};
    uint8_t  prev[kKeyCount]{};
    float    axes[kKeyCount]{};

    std::vector<ActionBinding> actionBindings;
    std::vector<AxisBinding>   axisBindings;

    float deadZone{0.1f};
    std::vector<CbEntry> callbacks;
    uint32_t nextCbId{1};
    std::vector<std::string> contextStack;

    bool KeyState(InputKey k) const {
        uint32_t i=(uint32_t)k;
        return i<kKeyCount && (cur[i]==1||cur[i]==2);
    }
    bool JustPressed (InputKey k) const { uint32_t i=(uint32_t)k; return i<kKeyCount&&cur[i]==2; }
    bool JustReleased(InputKey k) const { uint32_t i=(uint32_t)k; return i<kKeyCount&&cur[i]==3; }

    const ActionBinding* FindAction(const std::string& name) const {
        for (auto& b : actionBindings) if (b.actionName==name) return &b;
        return nullptr;
    }
};

InputSystem::InputSystem()  : m_impl(new Impl) {}
InputSystem::~InputSystem() { Shutdown(); delete m_impl; }

void InputSystem::Init()     {}
void InputSystem::Shutdown() {}

void InputSystem::Tick()
{
    // Transition states: 2→1, 3→0
    for (uint32_t i=0; i<kKeyCount; i++) {
        if (m_impl->cur[i]==2) m_impl->cur[i]=1;
        else if (m_impl->cur[i]==3) m_impl->cur[i]=0;
    }

    // Fire callbacks
    for (auto& cb : m_impl->callbacks) {
        const auto* b = m_impl->FindAction(cb.action);
        if (!b) continue;
        if (cb.type==0 && m_impl->JustPressed(b->key))
            { if(cb.cbVoid) cb.cbVoid(); }
        if (cb.type==1 && m_impl->JustReleased(b->key))
            { if(cb.cbVoid) cb.cbVoid(); }
        if (cb.type==2) {
            float v = m_impl->axes[(uint32_t)b->key];
            if(cb.cbAxis) cb.cbAxis(v);
        }
    }
}

void InputSystem::OnKey(InputKey key, InputEvent ev)
{
    uint32_t i=(uint32_t)key;
    if (i>=kKeyCount) return;
    if (ev==InputEvent::Pressed)  m_impl->cur[i] = m_impl->cur[i]?1:2;
    else                          m_impl->cur[i] = 3;
}

void InputSystem::OnMouseButton(InputKey button, InputEvent ev) { OnKey(button,ev); }
void InputSystem::OnMouseMove(float dx, float dy)
{
    m_impl->axes[(uint32_t)InputKey::MouseAxisX] = dx;
    m_impl->axes[(uint32_t)InputKey::MouseAxisY] = dy;
}
void InputSystem::OnMouseWheel(float dy) { m_impl->axes[(uint32_t)InputKey::MouseWheelY]=dy; }
void InputSystem::OnAxis(InputKey axis, float value)
{
    uint32_t i=(uint32_t)axis;
    if (i<kKeyCount) {
        float v = std::abs(value)<m_impl->deadZone ? 0.f : value;
        m_impl->axes[i]=v;
    }
}

void InputSystem::BindAction(const std::string& name, InputKey key, InputKey mod)
{
    for (auto& b : m_impl->actionBindings) if (b.actionName==name) { b.key=key; b.modifier=mod; return; }
    m_impl->actionBindings.push_back({name, key, mod});
}

void InputSystem::BindAxis(const std::string& name, InputKey key, float scale)
{
    for (auto& b : m_impl->axisBindings) if (b.actionName==name) { b.key=key; b.scale=scale; return; }
    m_impl->axisBindings.push_back({name, key, scale});
}

void InputSystem::UnbindAction(const std::string& name)
{
    auto& v = m_impl->actionBindings;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& b){ return b.actionName==name; }), v.end());
}

void InputSystem::UnbindAxis(const std::string& name)
{
    auto& v = m_impl->axisBindings;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& b){ return b.actionName==name; }), v.end());
}

bool InputSystem::IsPressed(const std::string& action) const
{
    const auto* b = m_impl->FindAction(action);
    return b && m_impl->JustPressed(b->key);
}

bool InputSystem::IsHeld(const std::string& action) const
{
    const auto* b = m_impl->FindAction(action);
    return b && m_impl->KeyState(b->key);
}

bool InputSystem::IsReleased(const std::string& action) const
{
    const auto* b = m_impl->FindAction(action);
    return b && m_impl->JustReleased(b->key);
}

float InputSystem::GetAxis(const std::string& action) const
{
    // Check axis bindings first
    for (auto& ab : m_impl->axisBindings) {
        if (ab.actionName==action) {
            uint32_t i=(uint32_t)ab.key;
            return i<kKeyCount ? m_impl->axes[i]*ab.scale : 0.f;
        }
    }
    // Fallback to action binding (digital → axis)
    const auto* b = m_impl->FindAction(action);
    if (b) return m_impl->KeyState(b->key) ? 1.f : 0.f;
    return 0.f;
}

bool InputSystem::KeyPressed (InputKey k) const { return m_impl->JustPressed(k); }
bool InputSystem::KeyHeld    (InputKey k) const { return m_impl->KeyState(k); }
bool InputSystem::KeyReleased(InputKey k) const { return m_impl->JustReleased(k); }
float InputSystem::AxisRaw   (InputKey k) const
{
    uint32_t i=(uint32_t)k;
    return i<kKeyCount ? m_impl->axes[i] : 0.f;
}

uint32_t InputSystem::OnPressed(const std::string& action, std::function<void()> cb)
{
    uint32_t id = m_impl->nextCbId++;
    m_impl->callbacks.push_back({id, action, 0, cb, {}});
    return id;
}

uint32_t InputSystem::OnReleased(const std::string& action, std::function<void()> cb)
{
    uint32_t id = m_impl->nextCbId++;
    m_impl->callbacks.push_back({id, action, 1, cb, {}});
    return id;
}

uint32_t InputSystem::OnAxis(const std::string& action, std::function<void(float)> cb)
{
    uint32_t id = m_impl->nextCbId++;
    m_impl->callbacks.push_back({id, action, 2, {}, cb});
    return id;
}

void InputSystem::RemoveCallback(uint32_t id)
{
    auto& v = m_impl->callbacks;
    v.erase(std::remove_if(v.begin(),v.end(),[&](auto& e){ return e.id==id; }), v.end());
}

void InputSystem::PushContext (const std::string& ctx) { m_impl->contextStack.push_back(ctx); }
void InputSystem::PopContext  () { if (!m_impl->contextStack.empty()) m_impl->contextStack.pop_back(); }
void InputSystem::ClearContext() { m_impl->contextStack.clear(); }
void InputSystem::SetDeadZone (float dz) { m_impl->deadZone=dz; }

bool InputSystem::LoadBindingsFromJSON(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

} // namespace Engine
