#pragma once
#include "UI/Framework/Widget.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace NF {

/// @brief Immediate-mode 2-D renderer used by the UI layer.
///
/// Batches coloured quads and text quads between BeginFrame() / EndFrame().
/// Uses the existing NF::Shader class for GPU programs, the same VAO/VBO
/// pattern from MeshRenderer, and stb_easy_font for glyph generation.
class UIRenderer {
public:
    /// @brief Initialise the renderer (compile shader, allocate buffers).
    /// @return True on success.
    bool Init();

    /// @brief Release all renderer-owned resources.
    void Shutdown();

    /// @brief Set the DPI scale factor (monitor DPI / 96).  Applied as a
    /// multiplier to the @p scale argument in DrawText().
    /// @param scale DPI scale; 1.0 = standard 96 DPI, 1.5 = 144 DPI, etc.
    void SetDpiScale(float scale) noexcept { m_DpiScale = scale > 0.f ? scale : 1.f; }

    /// @brief Return the current DPI scale factor.
    [[nodiscard]] float GetDpiScale() const noexcept { return m_DpiScale; }

    /// @brief Set the viewport size used for the orthographic projection.
    /// @param width  Viewport width in pixels.
    /// @param height Viewport height in pixels.
    void SetViewportSize(float width, float height);

    /// @brief Open a new UI frame — clears the batch and sets 2-D GL state.
    void BeginFrame();

    /// @brief Draw a filled rectangle.
    /// @param rect  Screen-space rectangle to fill.
    /// @param color Packed colour (0xRRGGBBAA).
    void DrawRect(const Rect& rect, uint32_t color);

    /// @brief Draw an outline rectangle (1-pixel border).
    /// @param rect  Screen-space rectangle.
    /// @param color Packed colour (0xRRGGBBAA).
    void DrawOutlineRect(const Rect& rect, uint32_t color);

    /// @brief Draw a string at the given screen position.
    /// @param text  Text to render.
    /// @param x     Left edge in pixels.
    /// @param y     Top edge in pixels.
    /// @param color Packed colour (0xRRGGBBAA).
    /// @param scale Text scale multiplier (default 1.0 = ~7px tall).
    void DrawText(std::string_view text, float x, float y, uint32_t color,
                  float scale = 1.0f);

    /// @brief Close the frame and flush all pending draw commands to the GPU.
    void EndFrame();

private:
    /// @brief A single 2-D vertex for the UI batch.
    struct UIVertex {
        float X, Y;
        float R, G, B, A;
    };

    void Flush();

    bool  m_Initialised{false};
    float m_ViewportWidth{1280.f};
    float m_ViewportHeight{720.f};
    float m_DpiScale{1.f};

    uint32_t m_ShaderProgram{0};
    uint32_t m_VAO{0};
    uint32_t m_VBO{0};

    std::vector<UIVertex> m_Vertices;
};

} // namespace NF
