#pragma once
/**
 * @file VoxelTerrain.h
 * @brief Sparse voxel grid: set/get voxel, marching-cubes mesh-gen, chunk dirty, raycast.
 *
 * Features:
 *   - 3D voxel grid partitioned into chunks (default 16³)
 *   - Voxel types: uint8_t material id (0=air)
 *   - SetVoxel / GetVoxel / FillBox / FillSphere
 *   - Marching-cubes mesh generation per chunk
 *   - Chunk dirty flag: SetVoxel marks chunk dirty
 *   - RegenerateDirty(): rebuilds meshes for dirty chunks
 *   - Raycast(origin, dir, maxDist) → hit voxel pos + material
 *   - Multiple terrain objects
 *   - GetMesh(chunkIdx) → vertex/index arrays
 *   - Serialise/Deserialise (run-length encoded)
 *   - On-chunk-updated callback
 *
 * Typical usage:
 * @code
 *   VoxelTerrain vt;
 *   VoxelTerrainDesc d; d.sizeX=64; d.sizeY=32; d.sizeZ=64; d.voxelSize=0.5f;
 *   uint32_t id = vt.Create(d);
 *   vt.FillBox(id, {0,0,0},{63,15,63}, 1);  // fill lower half with stone
 *   vt.RegenerateDirty(id);
 *   float outPos[3]; uint8_t mat;
 *   vt.Raycast(id, eye, dir, 100.f, outPos, mat);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct VoxelTerrainDesc {
    uint32_t sizeX      {64};
    uint32_t sizeY      {64};
    uint32_t sizeZ      {64};
    uint32_t chunkSize  {16};
    float    voxelSize  {0.5f};  ///< metres
};

struct VoxelMesh {
    std::vector<float>    vertices;  ///< interleaved XYZ + NXY NZ
    std::vector<uint32_t> indices;
    uint32_t              chunkIdx{0};
};

class VoxelTerrain {
public:
    VoxelTerrain();
    ~VoxelTerrain();

    void Init    ();
    void Shutdown();

    // Terrain lifecycle
    uint32_t Create (const VoxelTerrainDesc& desc, const float origin[3]=nullptr);
    void     Destroy(uint32_t id);
    bool     Has    (uint32_t id) const;

    // Voxel access
    void    SetVoxel(uint32_t id, int32_t x, int32_t y, int32_t z, uint8_t material);
    uint8_t GetVoxel(uint32_t id, int32_t x, int32_t y, int32_t z) const;
    void    FillBox   (uint32_t id, const int32_t min[3], const int32_t max[3], uint8_t mat);
    void    FillSphere(uint32_t id, const float centre[3], float radius, uint8_t mat);

    // Mesh generation
    void RegenerateDirty(uint32_t id);
    void RegenerateAll  (uint32_t id);
    const VoxelMesh* GetMesh(uint32_t id, uint32_t chunkIdx) const;
    uint32_t         ChunkCount(uint32_t id) const;
    bool             IsChunkDirty(uint32_t id, uint32_t chunkIdx) const;

    // Raycast
    bool Raycast(uint32_t id, const float origin[3], const float dir[3],
                  float maxDist, float outPos[3], uint8_t& outMaterial) const;

    // Serialise
    bool Save(uint32_t id, const std::string& path) const;
    bool Load(uint32_t id, const std::string& path);

    // Callback
    void SetOnChunkUpdated(std::function<void(uint32_t terrainId, uint32_t chunkIdx)> cb);

    std::vector<uint32_t> GetAll() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
