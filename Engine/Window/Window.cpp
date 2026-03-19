#include "Engine/Window/Window.h"
#include "Engine/Core/Logger.h"

// Stub implementation — real backend will link SDL2 or GLFW.

namespace Engine::Window {

Window::Window(const WindowConfig& cfg) : m_config(cfg) {}

Window::~Window() {
    if (m_initialized)
        Shutdown();
}

bool Window::Init() {
    Engine::Core::Logger::Info("Window::Init — stub (SDL2/GLFW not linked)");
    m_initialized = true;
    return true;
}

void Window::Shutdown() {
    if (!m_initialized)
        return;
    Engine::Core::Logger::Info("Window::Shutdown");
    m_initialized = false;
    m_handle      = nullptr;
}

bool Window::ShouldClose() const {
    return m_shouldClose;
}

void Window::PollEvents() {
    // No-op until a real windowing backend is linked.
}

void Window::SwapBuffers() {
    // No-op until a real windowing backend is linked.
}

int Window::GetWidth() const {
    return m_config.width;
}

int Window::GetHeight() const {
    return m_config.height;
}

float Window::GetAspectRatio() const {
    if (m_config.height == 0)
        return 1.0f;
    return static_cast<float>(m_config.width) / static_cast<float>(m_config.height);
}

void Window::SetTitle(const std::string& title) {
    m_config.title = title;
}

bool Window::IsMinimized() const {
    return false;
}

} // namespace Engine::Window
