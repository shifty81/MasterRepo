#pragma once
#include <string>
#include <functional>

// Forward-declare GLFWwindow to avoid pulling GLFW headers into every TU
struct GLFWwindow;

namespace Engine::Window {

enum class WindowMode { Windowed, Fullscreen, Borderless };

struct WindowConfig {
    std::string title  = "MasterRepo";
    int         width  = 1280;
    int         height = 720;
    WindowMode  mode   = WindowMode::Windowed;
    bool        vsync  = true;
    bool        resizable = true;
};

struct MouseState {
    double x = 0, y = 0;
    bool   buttons[8] = {};
};

struct KeyEvent {
    int  key;
    bool pressed;
};

class Window {
public:
    explicit Window(const WindowConfig& cfg);
    ~Window();

    bool Init();
    void Shutdown();

    bool  ShouldClose() const;
    void  PollEvents();
    void  SwapBuffers();

    int   GetWidth()       const;
    int   GetHeight()      const;
    float GetAspectRatio() const;

    void  SetTitle(const std::string& title);
    bool  IsMinimized() const;

    // Input callbacks (set before calling Init or PollEvents)
    std::function<void(double x, double y)>       onMouseMove;
    std::function<void(int btn, bool pressed)>    onMouseButton;
    std::function<void(int key, bool pressed)>    onKey;
    std::function<void(int w, int h)>             onResize;

    const MouseState& GetMouse() const { return m_mouse; }

    // GLFW window handle for renderer use
    GLFWwindow* GetGLFWHandle() const {
        return static_cast<GLFWwindow*>(m_handle);
    }

private:
    WindowConfig m_config;
    void*        m_handle      = nullptr;
    bool         m_initialized = false;
    MouseState   m_mouse;

    // Static GLFW callback trampolines
    static void s_CursorPosCallback(GLFWwindow* w, double x, double y);
    static void s_MouseButtonCallback(GLFWwindow* w, int btn, int action, int mods);
    static void s_KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void s_FramebufferSizeCallback(GLFWwindow* w, int width, int height);
};

} // namespace Engine::Window
