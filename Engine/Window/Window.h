#pragma once
#include <string>

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

    // Opaque handle — SDL2/GLFW can be attached later
    void* GetNativeHandle() const { return m_handle; }

private:
    WindowConfig m_config;
    void*        m_handle      = nullptr;
    bool         m_initialized = false;
    bool         m_shouldClose = false;
};

} // namespace Engine::Window
