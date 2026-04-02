#pragma once
#include <array>
#include <cstdint>

namespace NF::Editor {

/// @brief Snapshot of OS input events accumulated each Win32 message loop tick.
///
/// Call FlushFrameEvents() once per game loop iteration (after TickFrame)
/// to clear edge-triggered flags so they fire for exactly one frame.
struct EditorInputState {
    // ---- Mouse position -------------------------------------------------------
    float mouseX{0.f};             ///< Cursor X in client-area pixels.
    float mouseY{0.f};             ///< Cursor Y in client-area pixels.
    float mouseDeltaX{0.f};        ///< Cursor X displacement this frame (pixels).
    float mouseDeltaY{0.f};        ///< Cursor Y displacement this frame (pixels).

    // ---- Mouse buttons --------------------------------------------------------
    bool  leftDown{false};         ///< Left button is currently held.
    bool  leftJustPressed{false};  ///< Left button went down this frame.
    bool  leftJustReleased{false}; ///< Left button went up this frame.

    bool  rightDown{false};        ///< Right button is currently held.
    bool  rightJustPressed{false}; ///< Right button went down this frame.

    bool  middleDown{false};       ///< Middle button is currently held.
    bool  middleJustPressed{false};///< Middle button went down this frame.

    float wheelDelta{0.f};         ///< Mouse-wheel ticks this frame (+ve = scroll up).

    // ---- Keyboard -------------------------------------------------------------
    /// @brief Indexed by Win32 Virtual-Key code (0–255).
    std::array<bool, 256> keysDown{};        ///< Key is currently held.
    std::array<bool, 256> keysJustPressed{}; ///< Key went down this frame.

    // ---- Helpers --------------------------------------------------------------

    /// @brief Test whether a Win32 Virtual-Key is currently held.
    [[nodiscard]] bool IsKeyDown(uint8_t vk) const noexcept { return keysDown[vk]; }

    /// @brief Test whether a Win32 Virtual-Key was pressed this frame.
    [[nodiscard]] bool IsKeyJustPressed(uint8_t vk) const noexcept { return keysJustPressed[vk]; }

    /// @brief Clear all edge-triggered (single-frame) fields after processing.
    void FlushFrameEvents() noexcept {
        leftJustPressed  = false;
        leftJustReleased = false;
        rightJustPressed = false;
        middleJustPressed = false;
        wheelDelta       = 0.f;
        mouseDeltaX      = 0.f;
        mouseDeltaY      = 0.f;
        keysJustPressed.fill(false);
    }
};

} // namespace NF::Editor
