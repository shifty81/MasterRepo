#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Renderer/RHI/Shader.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Matrix.h"
#include <memory>

namespace NF {
class ForwardRenderer;
class RenderDevice;
}

namespace NF::Game {

// ---------------------------------------------------------------------------
// PlayerCharacterRenderer
//
// Owns the GPU mesh and character shader for the low-poly player character.
// Call Init() once after the render device is ready, then call Render() each
// frame inside a ForwardRenderer scene.
//
// The character is rendered as a single combined mesh at the player's world
// position, rotated to match the player's yaw.
// ---------------------------------------------------------------------------
class PlayerCharacterRenderer
{
public:
    // Compile the character shader and upload the mesh to the GPU.
    // renderDevice is non-owning.
    bool Init(RenderDevice* renderDevice);

    // Release all GPU resources.
    void Shutdown();

    // Render the character at the given world-space feet position and yaw.
    // Must be called between ForwardRenderer::BeginScene() and EndScene().
    void Render(ForwardRenderer& renderer,
                const NF::Vector3& feetPosition,
                float yawRadians);

    [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }

private:
    // Build the model transform: translate to feet position, rotate by yaw.
    static Matrix4x4 BuildTransform(const NF::Vector3& pos, float yaw);

    Mesh    m_Mesh;
    Shader  m_Shader;
    bool    m_Ready{false};
};

} // namespace NF::Game
