#pragma once

namespace NF::Editor {

/// @brief Snapshot of OS input events accumulated each Win32 message loop tick.
///
/// Call FlushFrameEvents() once per game loop iteration (after TickFrame)
/// to clear edge-triggered flags so they fire for exactly one frame.
struct EditorInputState {
    float mouseX{0.f};            ///< Cursor X in client-area pixels.
    float mouseY{0.f};            ///< Cursor Y in client-area pixels.
    bool  leftDown{false};        ///< Left mouse button is currently held.
    bool  leftJustPressed{false}; ///< Left button went down this frame.
    bool  leftJustReleased{false};///< Left button went up this frame.
    bool  rightDown{false};       ///< Right mouse button is currently held.
    bool  rightJustPressed{false};///< Right button went down this frame.
    float wheelDelta{0.f};        ///< Mouse-wheel ticks this frame (+ve = scroll up).

    /// @brief Clear all edge-triggered (single-frame) fields after processing.
    void FlushFrameEvents() noexcept {
        leftJustPressed  = false;
        leftJustReleased = false;
        rightJustPressed = false;
        wheelDelta       = 0.f;
    }
};

} // namespace NF::Editor
