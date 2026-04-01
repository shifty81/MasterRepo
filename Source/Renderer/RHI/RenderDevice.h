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
    /// @return True on success.
    bool Init(GraphicsAPI api);

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

    /// @brief Return the active graphics API.
    [[nodiscard]] GraphicsAPI GetAPI() const noexcept { return m_API; }

private:
    GraphicsAPI m_API{GraphicsAPI::Null};
    bool        m_Initialised{false};
};

} // namespace NF
