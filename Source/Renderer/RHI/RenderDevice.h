#pragma once
#include <cstdint>

namespace NF {

/// @brief Selects the underlying graphics API.
enum class GraphicsAPI { OpenGL, Null };

/// @brief Manages the low-level rendering device lifecycle and per-frame operations.
class RenderDevice {
public:
    bool Init(GraphicsAPI api, void* nativeWindowHandle = nullptr);
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Clear(float r, float g, float b, float a);
    void Resize(int w, int h) noexcept;

    /// @brief Set the active viewport in framebuffer pixels.
    void SetViewport(int x, int y, int w, int h) noexcept;

    /// @brief Enable or disable GPU scissor clipping.
    void EnableScissor(bool enabled) noexcept;

    /// @brief Set the active scissor rect in framebuffer pixels.
    void SetScissorRect(int x, int y, int w, int h) noexcept;

    [[nodiscard]] GraphicsAPI GetAPI() const noexcept { return m_API; }

private:
    GraphicsAPI m_API{GraphicsAPI::Null};
    bool        m_Initialised{false};
    void*       m_NativeWindowHandle{nullptr};
    void*       m_DeviceContext{nullptr};
    void*       m_RenderContext{nullptr};
    int         m_ViewportX{0};
    int         m_ViewportY{0};
    int         m_ViewportW{1};
    int         m_ViewportH{1};
};

} // namespace NF
