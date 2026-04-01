/// @file GamepadTests.cpp — Unit tests for the Gamepad input device.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Input/Devices/Gamepad.h"

using namespace NF::Input;
using Catch::Matchers::WithinAbs;

static constexpr float kEps = 1e-5f;

static Gamepad::State MakePressedState(GamepadButton btn, bool connected = true) {
    Gamepad::State s;
    s.Connected = connected;
    s.Buttons[static_cast<std::size_t>(btn)] = true;
    return s;
}

TEST_CASE("Gamepad is disconnected by default", "[input][gamepad]") {
    Gamepad gp;
    REQUIRE_FALSE(gp.IsConnected());
}

TEST_CASE("Gamepad marks connected after Update with connected state", "[input][gamepad]") {
    Gamepad gp;
    Gamepad::State s;
    s.Connected = true;
    gp.Update(s);
    REQUIRE(gp.IsConnected());
}

TEST_CASE("IsButtonDown returns true when button is held", "[input][gamepad]") {
    Gamepad gp;
    gp.Update(MakePressedState(GamepadButton::South));
    REQUIRE(gp.IsButtonDown(GamepadButton::South));
    REQUIRE_FALSE(gp.IsButtonDown(GamepadButton::North));
}

TEST_CASE("IsButtonPressed fires only on first frame", "[input][gamepad]") {
    Gamepad gp;
    gp.Update(MakePressedState(GamepadButton::East));
    REQUIRE(gp.IsButtonPressed(GamepadButton::East));

    gp.Update(MakePressedState(GamepadButton::East));
    REQUIRE_FALSE(gp.IsButtonPressed(GamepadButton::East));
    REQUIRE(gp.IsButtonDown(GamepadButton::East));
}

TEST_CASE("IsButtonReleased fires only when button goes up", "[input][gamepad]") {
    Gamepad gp;
    gp.Update(MakePressedState(GamepadButton::West));
    REQUIRE_FALSE(gp.IsButtonReleased(GamepadButton::West));

    Gamepad::State released;
    released.Connected = true;
    gp.Update(released);

    REQUIRE(gp.IsButtonReleased(GamepadButton::West));
    REQUIRE_FALSE(gp.IsButtonDown(GamepadButton::West));
}

TEST_CASE("Left stick dead-zone zeroes small values", "[input][gamepad]") {
    Gamepad gp;
    Gamepad::State s;
    s.Connected  = true;
    s.LeftStick  = {0.05f, -0.05f}; // within dead-zone of 0.1
    gp.Update(s);

    REQUIRE_THAT(gp.GetLeftStick().X, WithinAbs(0.f, kEps));
    REQUIRE_THAT(gp.GetLeftStick().Y, WithinAbs(0.f, kEps));
}

TEST_CASE("Left stick values outside dead-zone are non-zero", "[input][gamepad]") {
    Gamepad gp;
    Gamepad::State s;
    s.Connected = true;
    s.LeftStick = {0.8f, -0.6f};
    gp.Update(s);

    REQUIRE(gp.GetLeftStick().X > 0.f);
    REQUIRE(gp.GetLeftStick().Y < 0.f);
}

TEST_CASE("Trigger values are clamped to [0, 1]", "[input][gamepad]") {
    Gamepad gp;
    Gamepad::State s;
    s.Connected     = true;
    s.LeftTrigger   = 2.f;   // over-range
    s.RightTrigger  = -0.5f; // under-range
    gp.Update(s);

    REQUIRE_THAT(gp.GetLeftTrigger(),  WithinAbs(1.f, kEps));
    REQUIRE_THAT(gp.GetRightTrigger(), WithinAbs(0.f, kEps));
}

TEST_CASE("DPad buttons can be pressed independently", "[input][gamepad]") {
    Gamepad gp;
    gp.Update(MakePressedState(GamepadButton::DPadUp));
    REQUIRE(gp.IsButtonDown(GamepadButton::DPadUp));
    REQUIRE_FALSE(gp.IsButtonDown(GamepadButton::DPadDown));
}
