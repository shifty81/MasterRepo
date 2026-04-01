#pragma once
#include "UI/Framework/Widget.h"
#include <cstdint>
#include <string_view>

namespace NF {

/// @brief Immediate-mode 2-D renderer used by the UI layer.
///
/// All drawing calls must be issued between BeginFrame() and EndFrame().
class UIRenderer {
public:
    /// @brief Initialise the renderer (allocate GPU resources if available).
    /// @return True on success.
    bool Init();

    /// @brief Release all renderer-owned resources.
    void Shutdown();

    /// @brief Open a new UI frame.
    void BeginFrame();

    /// @brief Draw a filled rectangle.
    /// @param rect  Screen-space rectangle to fill.
    /// @param color Packed ARGB or RGBA colour (0xRRGGBBAA).
    void DrawRect(const Rect& rect, uint32_t color);

    /// @brief Draw a string at the given screen position.
    /// @param text  Text to render.
    /// @param x     Left edge in pixels.
    /// @param y     Top edge in pixels.
    /// @param color Packed colour (0xRRGGBBAA).
    void DrawText(std::string_view text, float x, float y, uint32_t color);

    /// @brief Close the frame and flush all pending draw commands.
    void EndFrame();

private:
    bool m_Initialised{false};
};

} // namespace NF
