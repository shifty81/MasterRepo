#include "Engine/Camera/CameraController.h"
#include <cmath>
#include <algorithm>

namespace Engine::Camera {

struct CameraController::Impl {
    Camera&                 camera;
    CameraControllerConfig  config;

    // Orbit state
    float yaw{0.0f};
    float pitch{-0.3f};  // slight downward look
    float radius{10.0f};
    Vec3  pivot{0.0f, 0.0f, 0.0f};

    // Fly state
    Vec3 flyVelocity{0.0f, 0.0f, 0.0f};

    // Follow state
    Vec3 followTarget{0.0f, 0.0f, 0.0f};
    Vec3 currentFollowPos{0.0f, 0.0f, 0.0f};
    TargetQuery followQuery;

    explicit Impl(Camera& cam) : camera(cam) {}
};

CameraController::CameraController(Camera& camera)
    : m_impl(new Impl(camera)) {}
CameraController::~CameraController() { delete m_impl; }

void CameraController::SetConfig(const CameraControllerConfig& cfg) {
    m_impl->config = cfg;
}
const CameraControllerConfig& CameraController::GetConfig() const {
    return m_impl->config;
}
void CameraController::SetMode(ControlMode mode) {
    m_impl->config.mode = mode;
}
ControlMode CameraController::GetMode() const {
    return m_impl->config.mode;
}

void CameraController::SetFollowTarget(float x, float y, float z) {
    m_impl->followTarget = {x, y, z};
}
void CameraController::SetFollowTargetQuery(TargetQuery fn) {
    m_impl->followQuery = std::move(fn);
}
void CameraController::SetOrbitPivot(float x, float y, float z) {
    m_impl->pivot = {x, y, z};
}
Vec3 CameraController::GetOrbitPivot() const { return m_impl->pivot; }

Camera& CameraController::GetCamera()             { return m_impl->camera; }
const Camera& CameraController::GetCamera() const { return m_impl->camera; }

static float Lerp(float a, float b, float t) { return a + (b - a) * t; }

static float Clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void CameraController::Update(const CameraInputDelta& input, float dt) {
    auto& im = *m_impl;
    const auto& cfg = im.config;

    switch (cfg.mode) {

    case ControlMode::Orbit: {
        im.yaw   += input.dx * cfg.orbitSensitivity;
        im.pitch -= input.dy * cfg.orbitSensitivity;
        im.pitch  = Clamp(im.pitch, -1.5f, 1.5f);
        im.radius = Clamp(im.radius - input.scroll, cfg.minZoom, cfg.maxZoom);

        float cy = std::cos(im.yaw),   sy = std::sin(im.yaw);
        float cp = std::cos(im.pitch), sp = std::sin(im.pitch);
        Vec3 offset{cy * cp * im.radius,
                    sp * im.radius,
                    sy * cp * im.radius};
        Vec3 pos{im.pivot.x + offset.x,
                 im.pivot.y + offset.y,
                 im.pivot.z + offset.z};
        im.camera.SetPosition(pos.x, pos.y, pos.z);
        im.camera.SetTarget(im.pivot.x, im.pivot.y, im.pivot.z);
        break;
    }

    case ControlMode::Fly: {
        im.yaw   += input.dx * cfg.orbitSensitivity;
        im.pitch -= input.dy * cfg.orbitSensitivity;
        im.pitch  = Clamp(im.pitch, -1.5f, 1.5f);

        float cy = std::cos(im.yaw),   sy = std::sin(im.yaw);
        float cp = std::cos(im.pitch);
        Vec3 fwd{cy * cp, std::sin(im.pitch), sy * cp};
        Vec3 right{-sy, 0.0f, cy};
        Vec3 upVec{0.0f, 1.0f, 0.0f};

        float speed = input.boost ? cfg.flySpeed * cfg.flyBoostMultiplier : cfg.flySpeed;
        Vec3 pos = im.camera.GetPosition();
        pos.x += (fwd.x * input.forward + right.x * input.strafe) * speed * dt;
        pos.y += (fwd.y * input.forward + upVec.y * input.up)     * speed * dt;
        pos.z += (fwd.z * input.forward + right.z * input.strafe) * speed * dt;
        im.camera.SetPosition(pos.x, pos.y, pos.z);
        im.camera.SetTarget(pos.x + fwd.x, pos.y + fwd.y, pos.z + fwd.z);
        break;
    }

    case ControlMode::Follow: {
        if (im.followQuery) im.followTarget = im.followQuery();

        float lag = Clamp(cfg.followLag, 0.0f, 1.0f);
        float alpha = 1.0f - std::pow(lag, cfg.smoothDamping * dt);
        im.currentFollowPos.x = Lerp(im.currentFollowPos.x, im.followTarget.x, alpha);
        im.currentFollowPos.y = Lerp(im.currentFollowPos.y, im.followTarget.y, alpha);
        im.currentFollowPos.z = Lerp(im.currentFollowPos.z, im.followTarget.z, alpha);

        im.yaw   += input.dx * cfg.orbitSensitivity;
        im.pitch -= input.dy * cfg.orbitSensitivity;
        im.pitch  = Clamp(im.pitch, -1.2f, 0.0f);
        im.radius = Clamp(im.radius - input.scroll, cfg.minZoom, cfg.maxZoom);

        float cy = std::cos(im.yaw), sy = std::sin(im.yaw);
        float cp = std::cos(im.pitch);
        Vec3& tgt = im.currentFollowPos;
        Vec3 camPos{tgt.x + cy * cp * cfg.followDistance,
                    tgt.y + cfg.followHeightOffset - std::sin(im.pitch) * cfg.followDistance,
                    tgt.z + sy * cp * cfg.followDistance};
        im.camera.SetPosition(camPos.x, camPos.y, camPos.z);
        im.camera.SetTarget(tgt.x, tgt.y + cfg.followHeightOffset * 0.5f, tgt.z);
        break;
    }
    }
}

} // namespace Engine::Camera
