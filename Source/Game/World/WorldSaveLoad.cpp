#include "Game/World/WorldSaveLoad.h"
#include "Core/Logging/Log.h"
#include <fstream>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Save to file
// ---------------------------------------------------------------------------

bool WorldSaveLoad::SaveToFile(const std::string& path, uint32_t seed,
                               const std::vector<uint32_t>& entityIds)
{
    auto bytes = SaveToMemory(seed, entityIds);
    if (bytes.empty()) return false;

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        Logger::Log(LogLevel::Error, "WorldSave",
                    "Cannot open file for writing: " + path);
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!file.good()) {
        Logger::Log(LogLevel::Error, "WorldSave",
                    "Write failed: " + path);
        return false;
    }

    Logger::Log(LogLevel::Info, "WorldSave",
                "Saved " + std::to_string(entityIds.size())
                + " entities to " + path);
    return true;
}

// ---------------------------------------------------------------------------
// Load from file
// ---------------------------------------------------------------------------

bool WorldSaveLoad::LoadFromFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        Logger::Log(LogLevel::Warning, "WorldSave",
                    "Cannot open save file: " + path);
        return false;
    }

    const auto fileSize = file.tellg();
    if (fileSize <= 0) {
        Logger::Log(LogLevel::Warning, "WorldSave",
                    "Save file is empty: " + path);
        return false;
    }

    file.seekg(0);
    std::vector<std::byte> buffer(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(fileSize));

    if (!LoadFromMemory(buffer.data(), buffer.size())) return false;

    Logger::Log(LogLevel::Info, "WorldSave",
                "Loaded " + std::to_string(m_EntityIds.size())
                + " entities from " + path);
    return true;
}

// ---------------------------------------------------------------------------
// In-memory serialization
// ---------------------------------------------------------------------------

std::vector<std::byte> WorldSaveLoad::SaveToMemory(
    uint32_t seed, const std::vector<uint32_t>& entityIds)
{
    BinaryArchive ar;

    WorldSaveHeader header;
    header.Seed        = seed;
    header.EntityCount = static_cast<uint32_t>(entityIds.size());

    ar.Serialize(header.Magic);
    ar.Serialize(header.Version);
    ar.Serialize(header.Seed);
    ar.Serialize(header.EntityCount);

    for (auto id : entityIds) {
        uint32_t eid = id;
        ar.Serialize(eid);
    }

    return {ar.Data().begin(), ar.Data().end()};
}

bool WorldSaveLoad::LoadFromMemory(const std::byte* data, size_t size)
{
    BinaryArchive ar(data, size);

    try {
        ar.Serialize(m_Header.Magic);
        ar.Serialize(m_Header.Version);
        ar.Serialize(m_Header.Seed);
        ar.Serialize(m_Header.EntityCount);
    } catch (const std::out_of_range&) {
        Logger::Log(LogLevel::Error, "WorldSave",
                    "Save data too small for header");
        return false;
    }

    if (m_Header.Magic != 0x4E465356) {
        Logger::Log(LogLevel::Error, "WorldSave",
                    "Invalid save magic number");
        return false;
    }

    m_EntityIds.clear();
    m_EntityIds.reserve(m_Header.EntityCount);

    try {
        for (uint32_t i = 0; i < m_Header.EntityCount; ++i) {
            uint32_t eid = 0;
            ar.Serialize(eid);
            m_EntityIds.push_back(eid);
        }
    } catch (const std::out_of_range&) {
        Logger::Log(LogLevel::Error, "WorldSave",
                    "Save data truncated; expected "
                    + std::to_string(m_Header.EntityCount) + " entities");
        return false;
    }

    return true;
}

} // namespace NF::Game
