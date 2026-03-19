#include "Engine/Camera/Camera.h"

namespace Engine::Camera {

static constexpr float kPi = 3.14159265358979323846f;

void Camera::SetMode(CameraMode mode) {
    m_mode = mode;
} // namespace Engine::Camera

CameraMode Camera::GetMode() const {
    return m_mode;
} // namespace Engine::Camera

void Camera::SetPosition(float x, float y, float z) {
    m_position = {x, y, z};
} // namespace Engine::Camera

Vec3 Camera::GetPosition() const {
    return m_position;
} // namespace Engine::Camera

void Camera::SetTarget(float x, float y, float z) {
    m_target = {x, y, z};
} // namespace Engine::Camera

Vec3 Camera::GetTarget() const {
    return m_target;
} // namespace Engine::Camera

void Camera::SetFOV(float fovDegrees) {
    m_fov = std::clamp(fovDegrees, 1.0f, 179.0f);
} // namespace Engine::Camera

float Camera::GetFOV() const {
    return m_fov;
} // namespace Engine::Camera

void Camera::SetClipPlanes(float nearPlane, float farPlane) {
    m_nearPlane = nearPlane > 0.0f ? nearPlane : 0.01f;
    m_farPlane = farPlane > m_nearPlane ? farPlane : m_nearPlane + 1.0f;
} // namespace Engine::Camera

float Camera::GetNearPlane() const {
    return m_nearPlane;
} // namespace Engine::Camera

float Camera::GetFarPlane() const {
    return m_farPlane;
} // namespace Engine::Camera

void Camera::SetOrbitalRadius(float radius) {
    m_orbitalRadius = radius > 0.1f ? radius : 0.1f;
} // namespace Engine::Camera

float Camera::GetOrbitalRadius() const {
    return m_orbitalRadius;
} // namespace Engine::Camera

void Camera::SetYawPitch(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = std::clamp(pitch, -89.0f, 89.0f);
} // namespace Engine::Camera

float Camera::GetYaw() const {
    return m_yaw;
} // namespace Engine::Camera

float Camera::GetPitch() const {
    return m_pitch;
} // namespace Engine::Camera

Vec3 Camera::GetForward() const {
    float yawRad = m_yaw * kPi / 180.0f;
    float pitchRad = m_pitch * kPi / 180.0f;
    return Vec3{
        std::cos(pitchRad) * std::sin(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::cos(yawRad)
    }.Normalized();
} // namespace Engine::Camera

Vec3 Camera::GetRight() const {
    float yawRad = m_yaw * kPi / 180.0f;
    return Vec3{
        std::cos(yawRad),
        0.0f,
        -std::sin(yawRad)
    }.Normalized();
} // namespace Engine::Camera

void Camera::MoveForward(float amount) {
    Vec3 fwd = GetForward();
    m_position = m_position + fwd * (amount * m_moveSpeed);
} // namespace Engine::Camera

void Camera::MoveRight(float amount) {
    Vec3 right = GetRight();
    m_position = m_position + right * (amount * m_moveSpeed);
} // namespace Engine::Camera

void Camera::MoveUp(float amount) {
    m_position.y += amount * m_moveSpeed;
} // namespace Engine::Camera

void Camera::Orbit(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch = std::clamp(m_pitch + deltaPitch, -89.0f, 89.0f);

    if (m_mode == CameraMode::Orbital) {
        float yawRad = m_yaw * kPi / 180.0f;
        float pitchRad = m_pitch * kPi / 180.0f;
        m_position.x = m_target.x + m_orbitalRadius * std::cos(pitchRad) * std::sin(yawRad);
        m_position.y = m_target.y + m_orbitalRadius * std::sin(pitchRad);
        m_position.z = m_target.z + m_orbitalRadius * std::cos(pitchRad) * std::cos(yawRad);
    }
} // namespace Engine::Camera

void Camera::Update(float dt) {
    (void)dt;
    if (m_mode == CameraMode::Strategy) {
        m_position.y = std::max(m_position.y, 1.0f);
    }
} // namespace Engine::Camera

} // namespace Engine::Camera
