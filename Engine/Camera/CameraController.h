#pragma once
/**
 * @file CameraController.h
 * @brief Higher-level camera controller with orbit / fly / follow modes.
 *
 * CameraController wraps Engine::Camera::Camera and provides smooth
 * input-driven movement for the three most common game camera patterns:
 *
 *   Orbit  — revolve around a fixed target point (editor / third-person)
 *   Fly    — free-flight first-person navigation (editor fly-through)
 *   Follow — camera tracks a moving target entity, with configurable lag
 */

#include "Engine/Camera/Camera.h"
#include <cstdint>
#include <functional>

namespace Engine::Camera {

enum class ControlMode { Orbit, Fly, Follow };

struct CameraControllerConfig {
    ControlMode mode{ControlMode::Orbit};
    float orbitSensitivity{0.005f};   ///< Radians per pixel drag
    float flySpeed{5.0f};             ///< Units/sec in fly mode
    float flyBoostMultiplier{3.0f};   ///< Speed when sprint held
    float followLag{0.1f};            ///< 0 = instant, 1 = very slow
    float followDistance{6.0f};       ///< Follow-cam distance
    float followHeightOffset{2.0f};   ///< Follow-cam height offset
    float minZoom{1.0f};              ///< Minimum orbit radius
    float maxZoom{200.0f};            ///< Maximum orbit radius
    float smoothDamping{8.0f};        ///< Smooth factor applied each frame
};

/// Input deltas supplied by the platform input layer each tick.
struct CameraInputDelta {
    float dx{0.0f};        ///< Mouse horizontal delta (pixels)
    float dy{0.0f};        ///< Mouse vertical delta (pixels)
    float scroll{0.0f};    ///< Scroll wheel delta
    float forward{0.0f};   ///< WASD forward axis  (-1 / 0 / +1)
    float strafe{0.0f};    ///< WASD lateral axis  (-1 / 0 / +1)
    float up{0.0f};        ///< Vertical axis (Q/E or controller)
    bool  boost{false};    ///< Sprint/boost modifier
};

/// CameraController — smooth orbit/fly/follow camera driver.
class CameraController {
public:
    explicit CameraController(Camera& camera);
    ~CameraController();

    // ── configuration ─────────────────────────────────────────
    void SetConfig(const CameraControllerConfig& cfg);
    const CameraControllerConfig& GetConfig() const;
    void SetMode(ControlMode mode);
    ControlMode GetMode() const;

    // ── follow target ─────────────────────────────────────────
    /// Set the world-space position the camera will follow.
    void SetFollowTarget(float x, float y, float z);
    /// Optional per-frame callback that returns the follow target position.
    using TargetQuery = std::function<Vec3()>;
    void SetFollowTargetQuery(TargetQuery fn);

    // ── orbit pivot ───────────────────────────────────────────
    void SetOrbitPivot(float x, float y, float z);
    Vec3 GetOrbitPivot() const;

    // ── per-tick ──────────────────────────────────────────────
    /// Process input and advance the camera by dt seconds.
    void Update(const CameraInputDelta& input, float dt);

    // ── camera accessors ──────────────────────────────────────
    Camera& GetCamera();
    const Camera& GetCamera() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine::Camera
