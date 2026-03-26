#pragma once
/**
 * @file GamepadSystem.h
 * @brief Full gamepad / controller input support with button mapping and
 *        rumble (vibration) feedback.
 *
 * Abstracts XInput / SDL gamepad behind a platform-neutral API.
 * Multiple controllers are supported concurrently.
 *
 * Typical usage:
 * @code
 *   GamepadSystem gp;
 *   gp.Init();
 *   gp.Poll();   // called each frame
 *   if (gp.IsButtonDown(0, GamepadButton::A)) { ... }
 *   float lx = gp.Axis(0, GamepadAxis::LeftX);
 *   gp.Rumble(0, 0.8f, 0.3f, 200);  // left 80%, right 30%, 200ms
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

// ── Gamepad buttons ───────────────────────────────────────────────────────────

enum class GamepadButton : uint8_t {
    A = 0, B, X, Y,
    LB, RB, LT_Digital, RT_Digital,
    Start, Back, Guide,
    DPadUp, DPadDown, DPadLeft, DPadRight,
    LStickClick, RStickClick,
    COUNT
};

// ── Gamepad axes ──────────────────────────────────────────────────────────────

enum class GamepadAxis : uint8_t {
    LeftX = 0, LeftY,
    RightX, RightY,
    LeftTrigger, RightTrigger,
    COUNT
};

// ── Per-controller state snapshot ─────────────────────────────────────────────

struct GamepadState {
    uint32_t  controllerId{0};
    bool      connected{false};
    bool      buttons[static_cast<int>(GamepadButton::COUNT)]{};
    float     axes[static_cast<int>(GamepadAxis::COUNT)]{};
    uint64_t  lastUpdateMs{0};
};

// ── GamepadSystem ─────────────────────────────────────────────────────────────

class GamepadSystem {
public:
    GamepadSystem();
    ~GamepadSystem();

    void Init();
    void Shutdown();

    /// Poll all connected controllers.  Call once per frame.
    void Poll();

    // ── Query ─────────────────────────────────────────────────────────────────

    uint32_t ConnectedCount() const;
    bool     IsConnected(uint32_t controllerId) const;

    bool     IsButtonDown(uint32_t controllerId, GamepadButton button) const;
    bool     IsButtonPressed(uint32_t controllerId, GamepadButton button) const; ///< edge: off→on this frame
    bool     IsButtonReleased(uint32_t controllerId, GamepadButton button) const; ///< edge: on→off

    float    Axis(uint32_t controllerId, GamepadAxis axis) const;

    /// Apply deadzone to an axis value.
    static float ApplyDeadzone(float value, float deadzone = 0.15f);

    GamepadState State(uint32_t controllerId) const;
    std::vector<uint32_t> ConnectedControllerIds() const;

    // ── Rumble ────────────────────────────────────────────────────────────────

    /// Set vibration levels (0–1) for durationMs milliseconds.
    void Rumble(uint32_t controllerId, float leftMotor, float rightMotor,
                uint32_t durationMs = 200);
    void StopRumble(uint32_t controllerId);

    // ── Button mapping ────────────────────────────────────────────────────────

    /// Map a logical action name to a physical button.
    void MapAction(const std::string& action, GamepadButton button,
                   uint32_t controllerId = 0);

    bool IsActionDown(const std::string& action, uint32_t controllerId = 0) const;
    bool IsActionPressed(const std::string& action, uint32_t controllerId = 0) const;

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnControllerConnected(std::function<void(uint32_t id)> cb);
    void OnControllerDisconnected(std::function<void(uint32_t id)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Engine
