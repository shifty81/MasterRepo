#pragma once
#include <cstdint>

namespace atlas::ui {

class UIRenderer;
struct UIContext;

/// Toggleable diagnostics overlay that renders live engine statistics.
/// Intended for use in both editor and client main loops.
///
/// Displays:
///   - FPS / frame time
///   - Viewport resolution
///   - DPI scale
///   - Mouse position
///   - UI input capture flags
///
/// Toggle with DiagnosticsOverlay::Toggle() (wired to Ctrl+Backtick or F3
/// in Engine::ProcessWindowEvents, or via HeadlessGUI commands:
/// diag.toggle, diag.show, diag.hide, diag.status).
/// Custom implementation — no ImGui dependency.
class DiagnosticsOverlay {
public:
    /// Toggle the overlay on/off.
    static void Toggle();

    /// Enable or disable the overlay explicitly.
    static void SetEnabled(bool enabled);

    /// Returns true when the overlay is currently visible.
    static bool IsEnabled();

    /// Render the overlay.  Call once per frame after all other UI
    /// rendering but before EndFrame().
    /// @param renderer  The active UIRenderer.
    /// @param ctx       Current UI context (viewport, dt, tick).
    /// @param dpiScale  Display DPI scale factor.
    /// @param mouseX    Current mouse X position.
    /// @param mouseY    Current mouse Y position.
    static void Render(UIRenderer* renderer,
                       const UIContext& ctx,
                       float dpiScale = 1.0f,
                       int32_t mouseX = 0,
                       int32_t mouseY = 0);

private:
    static bool s_enabled;
};

} // namespace atlas::ui
