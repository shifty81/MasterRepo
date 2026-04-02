#pragma once
#include "Core/Serialization/Archive.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game {

/// @brief Header written at the start of every world save file.
struct WorldSaveHeader {
    uint32_t Magic   = 0x4E465356; ///< "NFSV" — NovaForge Save Version
    uint32_t Version = 1;
    uint32_t Seed    = 0;
    uint32_t EntityCount = 0;
};

/// @brief Provides basic save and load hooks for a dev world.
///
/// Serializes a minimal world snapshot (header + entity list) using
/// @c BinaryArchive.  The intent is to prove deterministic round-trip
/// persistence; full component serialization is deferred to Phase 2+.
class WorldSaveLoad {
public:
    WorldSaveLoad() = default;

    /// @brief Save world state to a file on disk.
    /// @param path  Absolute or project-relative file path.
    /// @param seed  World seed to embed in the header.
    /// @param entityIds Entity identifiers to persist.
    /// @return True on success.
    bool SaveToFile(const std::string& path, uint32_t seed,
                    const std::vector<uint32_t>& entityIds);

    /// @brief Load world state from a file on disk.
    /// @param path Absolute or project-relative file path.
    /// @return True on success; call GetHeader() / GetEntityIds() afterwards.
    bool LoadFromFile(const std::string& path);

    /// @brief Serialize world state into an in-memory buffer.
    /// @return Byte buffer containing the serialized data.
    std::vector<std::byte> SaveToMemory(uint32_t seed,
                                        const std::vector<uint32_t>& entityIds);

    /// @brief Deserialize world state from an in-memory buffer.
    /// @param data Pointer to the serialized bytes.
    /// @param size Number of bytes available.
    /// @return True on success.
    bool LoadFromMemory(const std::byte* data, size_t size);

    // -- Accessors (valid after a successful Load) ----------------------------

    [[nodiscard]] const WorldSaveHeader& GetHeader() const noexcept { return m_Header; }
    [[nodiscard]] const std::vector<uint32_t>& GetEntityIds() const noexcept { return m_EntityIds; }

private:
    WorldSaveHeader        m_Header;
    std::vector<uint32_t>  m_EntityIds;
};

} // namespace NF::Game
