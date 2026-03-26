#include "Engine/Window/Window.h"
#include "Engine/Core/Logger.h"
#include <GLFW/glfw3.h>
#include <string>

namespace Engine::Window {

// ── GLFW error callback ────────────────────────────────────────────────────
static void glfwErrorCallback(int code, const char* desc) {
    Engine::Core::Logger::Error("GLFW error " + std::to_string(code) + ": " + desc);
}

// ── Constructor / Destructor ───────────────────────────────────────────────

Window::Window(const WindowConfig& cfg) : m_config(cfg) {}

Window::~Window() {
    if (m_initialized)
        Shutdown();
}

// ── Init ───────────────────────────────────────────────────────────────────

bool Window::Init() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        Engine::Core::Logger::Error("Window::Init — glfwInit() failed");
        return false;
    }

    // Request OpenGL 2.1 compatibility profile (works on all mesa drivers)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, m_config.resizable ? GLFW_TRUE : GLFW_FALSE);
    GLFWmonitor* monitor = nullptr;
    if (m_config.mode == WindowMode::Fullscreen)
        monitor = glfwGetPrimaryMonitor();

    // Attempt with 4x MSAA first; some GPU/driver combos reject the pixel
    // format when MSAA is combined with an older GL version, so fall back to
    // no MSAA rather than leaving the user with no window at all.
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* win = glfwCreateWindow(
        m_config.width, m_config.height,
        m_config.title.c_str(),
        monitor, nullptr);

    if (!win) {
        Engine::Core::Logger::Warn("Window::Init — MSAA window failed, retrying without MSAA");
        glfwWindowHint(GLFW_SAMPLES, 0);
        win = glfwCreateWindow(
            m_config.width, m_config.height,
            m_config.title.c_str(),
            monitor, nullptr);
    }

    if (!win) {
        Engine::Core::Logger::Error("Window::Init — glfwCreateWindow() failed");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(win);
    glfwSwapInterval(m_config.vsync ? 1 : 0);

    // Store 'this' so static callbacks can reach us
    glfwSetWindowUserPointer(win, this);

    // Register callbacks
    glfwSetCursorPosCallback      (win, s_CursorPosCallback);
    glfwSetMouseButtonCallback    (win, s_MouseButtonCallback);
    glfwSetKeyCallback            (win, s_KeyCallback);
    glfwSetCharCallback           (win, s_CharCallback);
    glfwSetFramebufferSizeCallback(win, s_FramebufferSizeCallback);

    m_handle      = win;
    m_initialized = true;

    Engine::Core::Logger::Info("Window initialized (" +
        std::to_string(m_config.width) + "x" + std::to_string(m_config.height) + ")");
    return true;
}

// ── Shutdown ───────────────────────────────────────────────────────────────

void Window::Shutdown() {
    if (!m_initialized) return;
    if (m_handle) {
        glfwDestroyWindow(static_cast<GLFWwindow*>(m_handle));
        m_handle = nullptr;
    }
    glfwTerminate();
    m_initialized = false;
    Engine::Core::Logger::Info("Window shutdown");
}

// ── Per-frame ──────────────────────────────────────────────────────────────

bool Window::ShouldClose() const {
    if (!m_handle) return true;
    return glfwWindowShouldClose(static_cast<GLFWwindow*>(m_handle)) != 0;
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    if (m_handle)
        glfwSwapBuffers(static_cast<GLFWwindow*>(m_handle));
}

// ── Queries ────────────────────────────────────────────────────────────────

int Window::GetWidth() const {
    if (!m_handle) return m_config.width;
    int w, h;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_handle), &w, &h);
    return w;
}

int Window::GetHeight() const {
    if (!m_handle) return m_config.height;
    int w, h;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_handle), &w, &h);
    return h;
}

float Window::GetAspectRatio() const {
    int h = GetHeight();
    return h == 0 ? 1.f : static_cast<float>(GetWidth()) / static_cast<float>(h);
}

void Window::SetTitle(const std::string& title) {
    m_config.title = title;
    if (m_handle)
        glfwSetWindowTitle(static_cast<GLFWwindow*>(m_handle), title.c_str());
}

bool Window::IsMinimized() const {
    if (!m_handle) return false;
    return glfwGetWindowAttrib(static_cast<GLFWwindow*>(m_handle), GLFW_ICONIFIED) != 0;
}

// ── Static GLFW callbacks ──────────────────────────────────────────────────

void Window::s_CursorPosCallback(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    self->m_mouse.x = x;
    self->m_mouse.y = y;
    if (self->onMouseMove) self->onMouseMove(x, y);
}

void Window::s_MouseButtonCallback(GLFWwindow* w, int btn, int action, int /*mods*/) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    bool pressed = (action == GLFW_PRESS);
    if (btn >= 0 && btn < 8) self->m_mouse.buttons[btn] = pressed;
    if (self->onMouseButton) self->onMouseButton(btn, pressed);
}

void Window::s_KeyCallback(GLFWwindow* w, int key, int /*scan*/, int action, int /*mods*/) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    if (action == GLFW_REPEAT) return; // ignore key-repeat for now
    bool pressed = (action == GLFW_PRESS);
    if (self->onKey) self->onKey(key, pressed);
}

void Window::s_CharCallback(GLFWwindow* w, unsigned int codepoint) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    if (self->onChar) self->onChar(codepoint);
}

void Window::s_FramebufferSizeCallback(GLFWwindow* w, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    if (self->onResize) self->onResize(width, height);
}

} // namespace Engine::Window
