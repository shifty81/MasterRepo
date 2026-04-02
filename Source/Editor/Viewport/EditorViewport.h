#pragma once
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Renderer/RHI/RenderDevice.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief 3-D viewport with an orbit camera.
class EditorViewport {
public:
    /// @brief Initialise the viewport with the given render device.
    /// @param device Non-owning pointer; must outlive this viewport.
    void Init(RenderDevice* device);

    /// @brief Set the UIRenderer used for drawing overlays.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Resize the viewport framebuffer.
    /// @param width  New width in pixels.
    /// @param height New height in pixels.
    void Resize(int width, int height);

    /// @brief Advance camera state.
    /// @param dt Delta time in seconds.
    void Update(float dt);

    /// @brief Issue draw calls for this viewport within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief Return the current view matrix.
    [[nodiscard]] Matrix4x4 GetViewMatrix() const noexcept;

    /// @brief Return the current projection matrix.
    [[nodiscard]] Matrix4x4 GetProjectionMatrix() const noexcept;

private:
    RenderDevice* m_Device{nullptr};
    UIRenderer*   m_Renderer{nullptr};
    int   m_Width{1280};
    int   m_Height{720};
    float m_Pitch{0.f};
    float m_Yaw{0.f};
    float m_Zoom{5.f};
    Vector3 m_Target{};
};

} // namespace NF::Editor
