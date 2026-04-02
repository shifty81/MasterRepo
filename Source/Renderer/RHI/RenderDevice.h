#pragma once
#include <cstdint>

namespace NF {

/// @brief Selects the underlying graphics API.
enum class GraphicsAPI { OpenGL, Null };

/// @brief Manages the low-level rendering device lifecycle and per-frame operations.
class RenderDevice {
public:
    /// @brief Initialise the device for the given graphics API.
    /// @param api The graphics API to use.
    /// @param nativeWindowHandle Optional platform window handle (HWND on
    ///        Windows).  Required when @p api is OpenGL so that a rendering
    ///        context can be created and bound to the window.
    /// @return True on success.
    bool Init(GraphicsAPI api, void* nativeWindowHandle = nullptr);

    /// @brief Release all device resources.
    void Shutdown();

    /// @brief Begin a new frame; must be called before any draw operations.
    void BeginFrame();

    /// @brief End the current frame and present the back buffer.
    void EndFrame();

    /// @brief Clear the active framebuffer to the specified RGBA colour.
    /// @param r Red component in [0, 1].
    /// @param g Green component in [0, 1].
    /// @param b Blue component in [0, 1].
    /// @param a Alpha component in [0, 1].
    void Clear(float r, float g, float b, float a);

    /// @brief Update the OpenGL viewport to the new client dimensions.
    ///
    /// Must be called whenever the window is resized so that OpenGL renders
    /// across the full client area.  Safe to call with the initial window size
    /// during Init.
    /// @param w New client width in pixels (ignored when <= 0).
    /// @param h New client height in pixels (ignored when <= 0).
    void Resize(int w, int h) noexcept;

    /// @brief Return the active graphics API.
    [[nodiscard]] GraphicsAPI GetAPI() const noexcept { return m_API; }

private:
    GraphicsAPI m_API{GraphicsAPI::Null};
    bool        m_Initialised{false};
    void*       m_NativeWindowHandle{nullptr}; ///< HWND on Windows
    void*       m_DeviceContext{nullptr};       ///< HDC on Windows
    void*       m_RenderContext{nullptr};       ///< HGLRC on Windows
};

} // namespace NF
