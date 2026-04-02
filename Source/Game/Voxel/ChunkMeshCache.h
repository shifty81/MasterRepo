#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Renderer/RHI/Shader.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/VoxelMesher.h"
#include "Core/Math/Matrix.h"
#include <unordered_map>
#include <memory>

namespace NF::Game {

/// @brief Maintains a GPU @c Mesh cache for every loaded chunk.
///
/// On each frame call @c RebuildDirty() to re-mesh any chunks whose
/// voxel data has changed.  Then call @c Render() to submit all
/// cached meshes to the @c ForwardRenderer.
///
/// The cache is keyed by @c ChunkCoord and keeps meshes in sync with
/// the @c ChunkMap that owns the voxel data.
class ChunkMeshCache {
public:
    ChunkMeshCache() = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Initialise the voxel shader and material.
    ///
    /// Must be called after an OpenGL context is current.
    /// @param forwardRenderer  Pointer to the forward renderer used
    ///                         for submitting draw calls.
    void Init(ForwardRenderer* forwardRenderer);

    /// @brief Release all GPU resources.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Per-frame operations
    // -------------------------------------------------------------------------

    /// @brief Re-mesh any dirty chunks and upload to the GPU.
    ///
    /// Iterates the chunk map, finds dirty chunks, generates new mesh
    /// data via @c VoxelMesher, and uploads to the GPU.
    void RebuildDirty(ChunkMap& map);

    /// @brief Submit all cached chunk meshes to the renderer.
    ///
    /// Call between @c ForwardRenderer::BeginScene() and @c EndScene().
    void Render();

    /// @brief Remove the cached mesh for a chunk that has been unloaded.
    void Evict(const ChunkCoord& coord);

    /// @brief Remove all cached meshes.
    void Clear();

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Number of chunk meshes currently cached on the GPU.
    [[nodiscard]] int CachedCount() const noexcept {
        return static_cast<int>(m_Meshes.size());
    }

    /// @brief Total triangle count across all cached meshes.
    [[nodiscard]] int TotalTriangles() const noexcept;

private:
    struct CachedMesh {
        std::unique_ptr<Mesh> GpuMesh;
        int TriCount{0};
    };

    std::unordered_map<ChunkCoord, CachedMesh> m_Meshes;

    VoxelMesher        m_Mesher;
    ForwardRenderer*   m_Renderer{nullptr};
    Shader             m_Shader;
    Material           m_Material;
};

} // namespace NF::Game
