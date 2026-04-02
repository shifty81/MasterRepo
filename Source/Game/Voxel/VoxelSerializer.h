#pragma once
#include "Game/Voxel/ChunkMap.h"
#include <string>
#include <vector>

namespace NF::Game {

/// @brief Magic number at the start of every chunk save file.
///
/// "NFCK" — NovaForge Chunk
inline constexpr uint32_t kChunkSaveMagic   = 0x4E46434Bu;
inline constexpr uint32_t kChunkSaveVersion = 1u;

/// @brief Header written before each chunk's voxel data.
struct ChunkSaveHeader {
    uint32_t Magic   = kChunkSaveMagic;
    uint32_t Version = kChunkSaveVersion;
    int32_t  CoordX  = 0;
    int32_t  CoordY  = 0;
    int32_t  CoordZ  = 0;
};

/// @brief Serializes and deserializes individual chunks and ChunkMaps.
///
/// Uses the same @c BinaryArchive pattern established by @c WorldSaveLoad.
class VoxelSerializer {
public:
    // ---- Single chunk -------------------------------------------------------

    /// @brief Serialize a single chunk to a byte buffer.
    [[nodiscard]] static std::vector<std::byte> SerializeChunk(const Chunk& chunk);

    /// @brief Deserialize a single chunk from a byte buffer.
    /// @param data  Pointer to the serialized bytes.
    /// @param size  Number of bytes available.
    /// @param[out] outChunk  Receives the deserialized chunk on success.
    /// @return True on success.
    [[nodiscard]] static bool DeserializeChunk(const std::byte* data, size_t size,
                                               Chunk& outChunk);

    // ---- ChunkMap -----------------------------------------------------------

    /// @brief Save all chunks in @p map to @p filePath.
    /// @return True on success.
    static bool SaveMap(const ChunkMap& map, const std::string& filePath);

    /// @brief Load all chunks from @p filePath into @p map.
    ///        Existing chunks in the map are replaced.
    /// @return True on success.
    static bool LoadMap(ChunkMap& map, const std::string& filePath);

    // ---- In-memory round-trip -----------------------------------------------

    /// @brief Serialize all chunks in @p map to an in-memory buffer.
    [[nodiscard]] static std::vector<std::byte> SerializeMap(const ChunkMap& map);

    /// @brief Deserialize chunks from an in-memory buffer into @p map.
    [[nodiscard]] static bool DeserializeMap(ChunkMap& map,
                                             const std::byte* data, size_t size);
};

} // namespace NF::Game
