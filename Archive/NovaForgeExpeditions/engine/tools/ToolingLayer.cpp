#include "ToolingLayer.h"
#include "../core/Logger.h"

namespace atlas::tools {

ToolModuleRegistry& ToolingLayer::Registry() {
    return m_registry;
}

const ToolModuleRegistry& ToolingLayer::Registry() const {
    return m_registry;
}

void ToolingLayer::Activate() {
    if (!m_active) {
        m_active = true;
        Logger::Info("ToolingLayer activated");
    }
}

void ToolingLayer::Deactivate() {
    if (m_active) {
        m_active = false;
        Logger::Info("ToolingLayer deactivated");
    }
}

void ToolingLayer::Toggle() {
    if (m_active)
        Deactivate();
    else
        Activate();
}

bool ToolingLayer::IsActive() const {
    return m_active;
}

bool ToolingLayer::HandleInput(uint32_t keyCode, bool pressed) {
    // Check for toggle hotkey first.
    if (m_toggleKey != 0 && keyCode == m_toggleKey && pressed) {
        Toggle();
        return true;
    }

    if (!m_active) return false;
    return m_registry.BroadcastInput(keyCode, pressed);
}

void ToolingLayer::Update(float deltaTime) {
    if (!m_active) return;
    m_registry.UpdateAll(deltaTime);
}

void ToolingLayer::Render() {
    if (!m_active) return;
    m_registry.RenderAll();
}

void ToolingLayer::SetToggleKey(uint32_t keyCode) {
    m_toggleKey = keyCode;
}

uint32_t ToolingLayer::ToggleKey() const {
    return m_toggleKey;
}

} // namespace atlas::tools
