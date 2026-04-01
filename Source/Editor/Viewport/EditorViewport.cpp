#include "Source/Editor/Viewport/EditorViewport.h"
#include <cmath>

namespace NF::Editor {

void EditorViewport::Init(RenderDevice* device) {
    m_Device = device;
}

void EditorViewport::Resize(int width, int height) {
    m_Width  = width;
    m_Height = height;
}

void EditorViewport::Update([[maybe_unused]] float dt) {
    // Orbit camera input would be handled here.
}

void EditorViewport::Draw() {
    if (!m_Device) return;
    m_Device->Clear(0.15f, 0.15f, 0.15f, 1.f);
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
    v.M[0][0] = right.X;   v.M[1][0] = right.Y;   v.M[2][0] = right.Z;
    v.M[0][1] = up.X;      v.M[1][1] = up.Y;      v.M[2][1] = up.Z;
    v.M[0][2] = -forward.X; v.M[1][2] = -forward.Y; v.M[2][2] = -forward.Z;
    v.M[3][0] = -right.Dot(eye);
    v.M[3][1] = -up.Dot(eye);
    v.M[3][2] =  forward.Dot(eye);
    return v;
}

Matrix4x4 EditorViewport::GetProjectionMatrix() const noexcept {
    // Perspective projection (45° FOV, near=0.1, far=1000).
    const float aspect = (m_Height > 0) ? static_cast<float>(m_Width) / static_cast<float>(m_Height) : 1.f;
    const float fovY   = 0.7854f; // ~45 degrees in radians
    const float nearZ  = 0.1f, farZ = 1000.f;
    const float f      = 1.f / std::tan(fovY * 0.5f);

    Matrix4x4 p{};
    p.M[0][0] = f / aspect;
    p.M[1][1] = f;
    p.M[2][2] = (farZ + nearZ) / (nearZ - farZ);
    p.M[2][3] = -1.f;
    p.M[3][2] = (2.f * farZ * nearZ) / (nearZ - farZ);
    return p;
}

} // namespace NF::Editor
