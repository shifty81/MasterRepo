#pragma once
#include <cstdint>

namespace Engine::Camera {

enum class WorldMode : uint8_t {
    SideScroller2D,
    TopDown2D,
    TopDownOrbit2_5D,
    Isometric2_5D
} // namespace Engine::Camera;

struct CameraProjectionPolicy {
    WorldMode mode = WorldMode::TopDown2D;

    float zoom = 1.0f;

    float orbitAngleDegrees = 0.0f;
    float elevationScale = 1.0f;

    float isoAngleX = 35.264f;
    float isoAngleY = 45.0f;
    float tileWidth = 1.0f;
    float tileHeight = 0.5f;

    bool snapToGrid = false;
} // namespace Engine::Camera;

} // namespace Engine::Camera
