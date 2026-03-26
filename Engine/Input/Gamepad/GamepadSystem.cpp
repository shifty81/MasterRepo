#include "Engine/Input/Gamepad/GamepadSystem.h"
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace Engine {

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

struct ControllerData {
    GamepadState prev;
    GamepadState curr;
    uint64_t     rumbleEndMs{0};
};

struct GamepadSystem::Impl {
    std::unordered_map<uint32_t, ControllerData>      controllers;
    std::unordered_map<std::string, std::pair<uint32_t,GamepadButton>> actionMap;
    std::function<void(uint32_t)> onConnected;
    std::function<void(uint32_t)> onDisconnected;

    static constexpr uint32_t MAX_CONTROLLERS = 4;
};

GamepadSystem::GamepadSystem() : m_impl(new Impl()) {}
GamepadSystem::~GamepadSystem() { delete m_impl; }

void GamepadSystem::Init()     {}
void GamepadSystem::Shutdown() { m_impl->controllers.clear(); }

void GamepadSystem::Poll() {
    // Platform stub: on Windows this would call XInputGetState.
    // Maintain previous state for edge detection.
    uint64_t now = NowMs();
    for (auto& [id, data] : m_impl->controllers) {
        data.prev = data.curr;
        // Stop rumble when duration expires
        if (data.rumbleEndMs > 0 && now >= data.rumbleEndMs) {
            data.rumbleEndMs = 0;
            StopRumble(id);
        }
    }
}

uint32_t GamepadSystem::ConnectedCount() const {
    uint32_t n = 0;
    for (auto& [id, d] : m_impl->controllers) if (d.curr.connected) ++n;
    return n;
}

bool GamepadSystem::IsConnected(uint32_t id) const {
    auto it = m_impl->controllers.find(id);
    return it != m_impl->controllers.end() && it->second.curr.connected;
}

bool GamepadSystem::IsButtonDown(uint32_t id, GamepadButton btn) const {
    auto it = m_impl->controllers.find(id);
    if (it == m_impl->controllers.end()) return false;
    return it->second.curr.buttons[static_cast<int>(btn)];
}

bool GamepadSystem::IsButtonPressed(uint32_t id, GamepadButton btn) const {
    auto it = m_impl->controllers.find(id);
    if (it == m_impl->controllers.end()) return false;
    int b = static_cast<int>(btn);
    return it->second.curr.buttons[b] && !it->second.prev.buttons[b];
}

bool GamepadSystem::IsButtonReleased(uint32_t id, GamepadButton btn) const {
    auto it = m_impl->controllers.find(id);
    if (it == m_impl->controllers.end()) return false;
    int b = static_cast<int>(btn);
    return !it->second.curr.buttons[b] && it->second.prev.buttons[b];
}

float GamepadSystem::Axis(uint32_t id, GamepadAxis axis) const {
    auto it = m_impl->controllers.find(id);
    if (it == m_impl->controllers.end()) return 0.f;
    return it->second.curr.axes[static_cast<int>(axis)];
}

float GamepadSystem::ApplyDeadzone(float value, float deadzone) {
    if (std::abs(value) < deadzone) return 0.f;
    float sign = value > 0.f ? 1.f : -1.f;
    return sign * (std::abs(value) - deadzone) / (1.f - deadzone);
}

GamepadState GamepadSystem::State(uint32_t id) const {
    auto it = m_impl->controllers.find(id);
    if (it == m_impl->controllers.end()) return {};
    return it->second.curr;
}

std::vector<uint32_t> GamepadSystem::ConnectedControllerIds() const {
    std::vector<uint32_t> out;
    for (auto& [id, d] : m_impl->controllers)
        if (d.curr.connected) out.push_back(id);
    return out;
}

void GamepadSystem::Rumble(uint32_t id, float left, float right, uint32_t durationMs) {
    auto& data = m_impl->controllers[id];
    data.rumbleEndMs = NowMs() + durationMs;
    // Platform: XInputSetState / SDL_GameControllerRumble would be called here.
    (void)left; (void)right;
}

void GamepadSystem::StopRumble(uint32_t id) {
    (void)id;
    // Platform: set left/right motor to 0.
}

void GamepadSystem::MapAction(const std::string& action, GamepadButton btn,
                              uint32_t controllerId) {
    m_impl->actionMap[action] = {controllerId, btn};
}

bool GamepadSystem::IsActionDown(const std::string& action, uint32_t cid) const {
    auto it = m_impl->actionMap.find(action);
    if (it == m_impl->actionMap.end()) return false;
    return IsButtonDown(cid, it->second.second);
}

bool GamepadSystem::IsActionPressed(const std::string& action, uint32_t cid) const {
    auto it = m_impl->actionMap.find(action);
    if (it == m_impl->actionMap.end()) return false;
    return IsButtonPressed(cid, it->second.second);
}

void GamepadSystem::OnControllerConnected(std::function<void(uint32_t)> cb) {
    m_impl->onConnected = std::move(cb);
}

void GamepadSystem::OnControllerDisconnected(std::function<void(uint32_t)> cb) {
    m_impl->onDisconnected = std::move(cb);
}

} // namespace Engine
