#include "Editor/Viewport/EditorViewport.h"
#include "UI/Rendering/UIRenderer.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace NF::Editor {

// Camera sensitivity constants
static constexpr float kOrbitSensitivity = 0.005f; // radians per pixel
static constexpr float kPanSensitivity   = 0.003f; // world units per pixel per unit zoom
static constexpr float kZoomSensitivity  = 0.12f;  // fraction of current zoom per wheel tick
static constexpr float kMinZoom          = 0.2f;
static constexpr float kMaxZoom          = 500.f;
static constexpr float kMaxPitch         = 1.55f;  // ~89 degrees (avoid gimbal lock)

void EditorViewport::Init(RenderDevice* device) {
    m_Device = device;
}

void EditorViewport::Resize(int width, int height) {
    m_Width  = width;
    m_Height = height;
}

void EditorViewport::Update([[maybe_unused]] float dt) {
    if (!m_Input || m_BoundsW <= 0.f || m_BoundsH <= 0.f) return;

    // Only respond when the cursor is inside the viewport panel.
    const bool inViewport =
        m_Input->mouseX >= m_BoundsX && m_Input->mouseX < m_BoundsX + m_BoundsW &&
        m_Input->mouseY >= m_BoundsY && m_Input->mouseY < m_BoundsY + m_BoundsH;

    if (!inViewport) return;

    const float dx = m_Input->mouseDeltaX;
    const float dy = m_Input->mouseDeltaY;

    // --- Orbit: right mouse drag ---
    if (m_Input->rightDown && (dx != 0.f || dy != 0.f)) {
        m_Yaw   += dx * kOrbitSensitivity;
        m_Pitch -= dy * kOrbitSensitivity; // invert Y for natural feel
        m_Pitch  = std::clamp(m_Pitch, -kMaxPitch, kMaxPitch);
    }

    // --- Pan: middle mouse drag ---
    if (m_Input->middleDown && (dx != 0.f || dy != 0.f)) {
        // Build camera right and up axes from current yaw/pitch.
        const float cp = std::cos(m_Pitch);
        const float sp = std::sin(m_Pitch);
        const float cy = std::cos(m_Yaw);
        const float sy = std::sin(m_Yaw);

        Vector3 right  = Vector3{ cy, 0.f, -sy }.Normalized();
        Vector3 camUp  = Vector3{ sp * sy, cp, sp * cy }.Normalized();

        // Scale pan speed with zoom so distant scenes pan at a natural rate.
        const float panScale = kPanSensitivity * m_Zoom;
        m_Target = m_Target - right * (dx * panScale) + camUp * (dy * panScale);
    }

    // --- Zoom: mouse wheel ---
    if (m_Input->wheelDelta != 0.f) {
        m_Zoom *= 1.f - m_Input->wheelDelta * kZoomSensitivity;
        m_Zoom  = std::clamp(m_Zoom, kMinZoom, kMaxZoom);
    }
}

void EditorViewport::Draw(float x, float y, float w, float h) {
    // Cache bounds so Update() can do mouse-in-viewport testing.
    m_BoundsX = x;
    m_BoundsY = y;
    m_BoundsW = w;
    m_BoundsH = h;

    if (!m_Renderer) return;

    static constexpr uint32_t kCamInfoColor  = 0xCCCCCCFF;
    static constexpr uint32_t kBorderColor   = 0x505050FF;

    // Draw subtle border around viewport to show its bounds
    m_Renderer->DrawOutlineRect({x, y, w, h}, kBorderColor);

    // Camera info overlay (bottom-left corner of viewport).
    // Converts pitch/yaw to degrees for readability.
    const float dpi = m_Renderer->GetDpiScale();
    const float padX = 8.f * dpi;
    const float padY = 8.f * dpi;

    auto toDeg = [](float rad) -> int {
        return static_cast<int>(rad * (180.f / std::numbers::pi_v<float>));
    };

    // Draw camera controls info
    std::string controls = "RMB: Rotate | MMB: Pan | Wheel: Zoom";
    m_Renderer->DrawText(controls,
                         x + padX,
                         y + h - 38.f * dpi,
                         0x808080FF, 1.3f);

    std::string camStr = "Camera: Yaw " + std::to_string(toDeg(m_Yaw))
                       + " Pitch " + std::to_string(toDeg(m_Pitch))
                       + " Zoom " + std::to_string(static_cast<int>(m_Zoom));
    m_Renderer->DrawText(camStr,
                         x + padX,
                         y + h - 20.f * dpi,
                         kCamInfoColor, 1.3f);
}

Matrix4x4 EditorViewport::GetViewMatrix() const noexcept {
    // Build simple orbit look-at from pitch/yaw/zoom.
    const float cp = std::cos(m_Pitch), sp = std::sin(m_Pitch);
    const float cy = std::cos(m_Yaw),  sy = std::sin(m_Yaw);
    Vector3 eye{
        m_Target.X + m_Zoom * cp * sy,
        m_Target.Y + m_Zoom * sp,
        m_Target.Z + m_Zoom * cp * cy
    };
    Vector3 forward = (m_Target - eye).Normalized();
    Vector3 right   = forward.Cross({0.f, 1.f, 0.f}).Normalized();
    Vector3 up      = right.Cross(forward);

    Matrix4x4 v = Matrix4x4::Identity();
    v.M[0][0] = right.X;    v.M[1][0] = right.Y;    v.M[2][0] = right.Z;
    v.M[0][1] = up.X;       v.M[1][1] = up.Y;       v.M[2][1] = up.Z;
    v.M[0][2] = -forward.X; v.M[1][2] = -forward.Y; v.M[2][2] = -forward.Z;
    v.M[3][0] = -right.Dot(eye);
    v.M[3][1] = -up.Dot(eye);
    v.M[3][2] =  forward.Dot(eye);
    return v;
}

Matrix4x4 EditorViewport::GetProjectionMatrix() const noexcept {
    // Perspective projection (45° FOV, near=0.1, far=1000).
    const float aspect = (m_Height > 0)
        ? static_cast<float>(m_Width) / static_cast<float>(m_Height) : 1.f;
    const float fovY  = 0.7854f; // ~45 degrees in radians
    const float nearZ = 0.1f, farZ = 1000.f;
    const float f     = 1.f / std::tan(fovY * 0.5f);

    Matrix4x4 p{};
    p.M[0][0] = f / aspect;
    p.M[1][1] = f;
    p.M[2][2] = (farZ + nearZ) / (nearZ - farZ);
    p.M[2][3] = -1.f;
    p.M[3][2] = (2.f * farZ * nearZ) / (nearZ - farZ);
    return p;
}

} // namespace NF::Editor
