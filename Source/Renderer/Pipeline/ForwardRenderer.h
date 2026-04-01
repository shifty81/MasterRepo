#pragma once
#include "Core/Math/Matrix.h"
#include <vector>

namespace NF {

class RenderDevice;
class Mesh;
class Material;

/// @brief Single-pass forward renderer; collects draw calls per-scene and flushes them in order.
class ForwardRenderer {
public:
    /// @brief Attach the renderer to a live render device.
    /// @param device Non-owning pointer; must remain valid for the lifetime of this object.
    void Init(RenderDevice* device);

    /// @brief Release all renderer-owned resources.
    void Shutdown();

    /// @brief Open a new scene with the given view and projection matrices.
    /// @param viewMatrix  World-to-camera transform.
    /// @param projMatrix  Camera-to-clip transform.
    void BeginScene(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix);

    /// @brief Queue a mesh for rendering with the given material and model transform.
    /// @param mesh      Geometry to draw.
    /// @param material  Surface material to apply.
    /// @param transform Model-to-world matrix.
    void Submit(Mesh& mesh, Material& material, const Matrix4x4& transform);

    /// @brief Close the current scene.
    void EndScene();

    /// @brief Issue all queued draw calls to the GPU.
    void Flush();

private:
    struct DrawCommand {
        Mesh*      MeshPtr{nullptr};
        Material*  MaterialPtr{nullptr};
        Matrix4x4  Transform{Matrix4x4::Identity()};
    };

    RenderDevice*             m_Device{nullptr};
    Matrix4x4                 m_View{Matrix4x4::Identity()};
    Matrix4x4                 m_Proj{Matrix4x4::Identity()};
    std::vector<DrawCommand>  m_Queue;
};

} // namespace NF
