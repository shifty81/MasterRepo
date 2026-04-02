#include "Game/Voxel/VoxelSerializer.h"
#include "Core/Logging/Log.h"
#include "Core/Serialization/Archive.h"
#include <fstream>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Single chunk
// ---------------------------------------------------------------------------

std::vector<std::byte> VoxelSerializer::SerializeChunk(const Chunk& chunk)
{
    BinaryArchive ar; // write mode

    ChunkSaveHeader header;
    header.CoordX = chunk.GetCoord().X;
    header.CoordY = chunk.GetCoord().Y;
    header.CoordZ = chunk.GetCoord().Z;

    ar.Serialize(header.Magic);
    ar.Serialize(header.Version);
    ar.Serialize(header.CoordX);
    ar.Serialize(header.CoordY);
    ar.Serialize(header.CoordZ);

    // Write voxel data as a flat byte array.
    const auto& raw = chunk.GetRawData();
    for (auto v : raw) {
        VoxelId id = v;
        ar.Serialize(id);
    }

    const auto& buf = ar.Data();
    return { buf.begin(), buf.end() };
}

bool VoxelSerializer::DeserializeChunk(const std::byte* data, size_t size,
                                        Chunk& outChunk)
{
    BinaryArchive ar(data, size);
    ChunkSaveHeader header;

    try {
        ar.Serialize(header.Magic);
        ar.Serialize(header.Version);
        ar.Serialize(header.CoordX);
        ar.Serialize(header.CoordY);
        ar.Serialize(header.CoordZ);
    } catch (const std::out_of_range&) {
        Logger::Log(LogLevel::Error, "VoxelSerializer",
                    "Chunk header read past end of buffer");
        return false;
    }

    if (header.Magic != kChunkSaveMagic) {
        Logger::Log(LogLevel::Error, "VoxelSerializer",
                    "Invalid chunk magic number");
        return false;
    }

    // Reconstruct the chunk at the correct coord.
    ChunkCoord coord{ header.CoordX, header.CoordY, header.CoordZ };
    if (outChunk.GetCoord() != coord) {
        Logger::Log(LogLevel::Warning, "VoxelSerializer",
                    "Coord mismatch during DeserializeChunk");
    }

    try {
        for (int32_t i = 0; i < kChunkVolume; ++i) {
            VoxelId id = 0;
            ar.Serialize(id);
            uint8_t x = static_cast<uint8_t>(i % kChunkSize);
            uint8_t y = static_cast<uint8_t>((i / kChunkSize) % kChunkSize);
            uint8_t z = static_cast<uint8_t>(i / (kChunkSize * kChunkSize));
            outChunk.SetVoxel(x, y, z, id);
        }
    } catch (const std::out_of_range&) {
        Logger::Log(LogLevel::Error, "VoxelSerializer",
                    "Chunk voxel data truncated");
        return false;
    }

    outChunk.ClearDirty();
    return true;
}

// ---------------------------------------------------------------------------
// In-memory map round-trip
// ---------------------------------------------------------------------------

std::vector<std::byte> VoxelSerializer::SerializeMap(const ChunkMap& map)
{
    BinaryArchive ar; // write mode

    // Preamble: total chunk count.
    uint32_t chunkCount = static_cast<uint32_t>(map.ChunkCount());
    ar.Serialize(chunkCount);

    const auto& coords = map.GetLoadedCoords();
    for (const auto& coord : coords) {
        const Chunk* chunk = map.GetChunk(coord);
        if (!chunk) continue;

        auto chunkBytes = SerializeChunk(*chunk);
        uint32_t chunkSize = static_cast<uint32_t>(chunkBytes.size());
        ar.Serialize(chunkSize);
        for (auto b : chunkBytes) ar.Serialize(b);
    }

    const auto& buf = ar.Data();
    return { buf.begin(), buf.end() };
}

bool VoxelSerializer::DeserializeMap(ChunkMap& map,
                                      const std::byte* data, size_t size)
{
    BinaryArchive ar(data, size);

    uint32_t chunkCount = 0;
    try {
        ar.Serialize(chunkCount);
    } catch (const std::out_of_range&) {
        Logger::Log(LogLevel::Error, "VoxelSerializer",
                    "Map data too small for header");
        return false;
    }

    for (uint32_t i = 0; i < chunkCount; ++i) {
        uint32_t chunkSize = 0;
        try {
            ar.Serialize(chunkSize);
        } catch (const std::out_of_range&) {
            Logger::Log(LogLevel::Error, "VoxelSerializer",
                        "Map data truncated reading chunk size");
            return false;
        }

        std::vector<std::byte> chunkBuf(chunkSize);
        try {
            for (uint32_t b = 0; b < chunkSize; ++b)
                ar.Serialize(chunkBuf[b]);
        } catch (const std::out_of_range&) {
            Logger::Log(LogLevel::Error, "VoxelSerializer",
                        "Map data truncated reading chunk bytes");
            return false;
        }

        // Peek at the header to find the coord before creating the chunk.
        if (chunkBuf.size() < sizeof(ChunkSaveHeader)) {
            Logger::Log(LogLevel::Error, "VoxelSerializer",
                        "Chunk blob smaller than header");
            return false;
        }
        ChunkSaveHeader peek{};
        BinaryArchive peekAr(chunkBuf.data(), chunkBuf.size());
        try {
            peekAr.Serialize(peek.Magic);
            peekAr.Serialize(peek.Version);
            peekAr.Serialize(peek.CoordX);
            peekAr.Serialize(peek.CoordY);
            peekAr.Serialize(peek.CoordZ);
        } catch (...) {
            Logger::Log(LogLevel::Error, "VoxelSerializer",
                        "Failed to peek chunk coord");
            return false;
        }

        ChunkCoord coord{ peek.CoordX, peek.CoordY, peek.CoordZ };
        Chunk* chunk = map.GetOrCreateChunk(coord);
        if (!DeserializeChunk(chunkBuf.data(), chunkBuf.size(), *chunk))
            return false;
    }

    Logger::Log(LogLevel::Info, "VoxelSerializer",
                "Loaded " + std::to_string(chunkCount) + " chunks");
    return true;
}

// ---------------------------------------------------------------------------
// File I/O
// ---------------------------------------------------------------------------

bool VoxelSerializer::SaveMap(const ChunkMap& map, const std::string& filePath)
{
    auto bytes = SerializeMap(map);

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::Log(LogLevel::Error, "VoxelSerializer",
                    "Cannot open file for writing: " + filePath);
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!file.good()) {
        Logger::Log(LogLevel::Error, "VoxelSerializer",
                    "Write failed: " + filePath);
        return false;
    }

    Logger::Log(LogLevel::Info, "VoxelSerializer",
                "Saved " + std::to_string(map.ChunkCount())
                + " chunks to " + filePath);
    return true;
}

bool VoxelSerializer::LoadMap(ChunkMap& map, const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logger::Log(LogLevel::Warning, "VoxelSerializer",
                    "Cannot open chunk map file: " + filePath);
        return false;
    }

    const auto fileSize = file.tellg();
    if (fileSize <= 0) {
        Logger::Log(LogLevel::Warning, "VoxelSerializer",
                    "Chunk map file is empty: " + filePath);
        return false;
    }

    file.seekg(0);
    std::vector<std::byte> buffer(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(fileSize));

    return DeserializeMap(map, buffer.data(), buffer.size());
}

} // namespace NF::Game
