#pragma once
#include "UI/Framework/Widget.h"
#include <cstdint>
#include <string_view>
#include <vector>

// Windows defines DrawText as a macro (DrawTextA/DrawTextW) which conflicts
// with NF::UIRenderer::DrawText.  Undefine it here so the method is visible
// by its correct name regardless of include order.
#ifdef DrawText
#  undef DrawText
#endif

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

    void BeginFrame();

    /// @brief Push a clip rect in top-left UI coordinates.
    void PushClipRect(const Rect& rect);

    /// @brief Pop the most recently pushed clip rect.
    void PopClipRect();

    void DrawRect(const Rect& rect, uint32_t color);
    void DrawOutlineRect(const Rect& rect, uint32_t color);
    void DrawText(std::string_view text, float x, float y, uint32_t color,
                  float scale = 1.0f);
    void EndFrame();

private:
    struct UIVertex {
        float X, Y;
        float R, G, B, A;
    };

    [[nodiscard]] bool HasClipRect() const noexcept { return !m_ClipStack.empty(); }
    [[nodiscard]] Rect CurrentClipRect() const noexcept;
    [[nodiscard]] Rect IntersectRect(const Rect& a, const Rect& b) const noexcept;
    [[nodiscard]] bool ClipRectToCurrent(const Rect& input, Rect& output) const noexcept;
    void Flush();

    bool  m_Initialised{false};
    float m_ViewportWidth{1280.f};
    float m_ViewportHeight{720.f};
    float m_DpiScale{1.f};

    uint32_t m_ShaderProgram{0};
    uint32_t m_VAO{0};
    uint32_t m_VBO{0};

    std::vector<UIVertex> m_Vertices;
    std::vector<Rect>     m_ClipStack;
};

} // namespace NF
