#pragma once
#include "Core/Math/Vector.h"
#include <cstdint>
#include <vector>

namespace NF {

/// @brief A single interleaved vertex stored on the CPU.
struct Vertex {
    Vector3 Position{};
    Vector3 Normal{};
    Vector2 TexCoord{};
};

/// @brief CPU-side mesh data; uploaded to the GPU via Mesh::Upload.
struct MeshData {
    std::vector<Vertex>   Vertices;
    std::vector<uint32_t> Indices;
};

/// @brief GPU mesh resource: owns a VAO / VBO / IBO and issues draw calls.
class Mesh {
public:
    /// @brief Upload mesh data to the GPU.
    /// @param data CPU mesh to upload.
    void Upload(const MeshData& data);

    /// @brief Bind the vertex array object for subsequent draw calls.
    void Bind();

    /// @brief Issue the draw call for this mesh.
    void Draw();

    /// @brief Return the number of vertices in the mesh.
    [[nodiscard]] uint32_t GetVertexCount() const noexcept { return m_VertexCount; }

    /// @brief Return the number of indices in the mesh.
    [[nodiscard]] uint32_t GetIndexCount()  const noexcept { return m_IndexCount;  }

private:
    uint32_t m_VAO{0};
    uint32_t m_VBO{0};
    uint32_t m_IBO{0};
    uint32_t m_VertexCount{0};
    uint32_t m_IndexCount{0};
};

} // namespace NF
