#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "Core/ArchiveSystem/ArchiveRecord.h"

namespace Core::ArchiveSystem {

/// File archiving utility — copies files into a versioned archive store
/// and maintains a JSON manifest of every archived entry.
///
/// The archive destination is the "Archive/" subdirectory relative to the
/// provided root (or the current working directory if no root is set).
/// Files are placed in a flat structure using their entry ID as the filename
/// to avoid collision, while ArchiveRecord::OriginalPath preserves provenance.
///
/// Usage:
///   Core::ArchiveSystem::ArchiveSystem as("Archive/Deprecated");
///
///   as.Archive("Engine/OldSystem.cpp", "Replaced by new ECS", "deprecated,ecs");
///   as.SaveManifest("Archive/Deprecated/manifest.json");
///
///   // Later:
///   as.LoadManifest("Archive/Deprecated/manifest.json");
///   for (const auto& rec : as.GetRecords()) { ... }
class ArchiveSystem {
public:
    /// @param archiveRoot  Directory where archived files are stored.
    ///                     Created automatically if it does not exist.
    explicit ArchiveSystem(std::string archiveRoot = "Archive");

    ~ArchiveSystem() = default;

    ArchiveSystem(const ArchiveSystem&)            = delete;
    ArchiveSystem& operator=(const ArchiveSystem&) = delete;

    /// Copy @p srcPath into the archive directory and record metadata.
    /// @param srcPath  File to archive (must exist and be readable).
    /// @param reason   Why the file is being archived.
    /// @param tags     Optional comma-separated categorisation tags.
    /// @return True on success; false if the source file is missing or
    ///         the copy fails.
    bool Archive(const std::string& srcPath,
                 const std::string& reason,
                 const std::string& tags = "");

    /// Return a snapshot of all records held in memory.
    [[nodiscard]] std::vector<ArchiveRecord> GetRecords() const;

    /// Persist the in-memory manifest to a JSON file at @p manifestPath.
    /// Overwrites any existing file.
    /// @return True on success.
    [[nodiscard]] bool SaveManifest(const std::string& manifestPath) const;

    /// Load (replace) the in-memory manifest from a JSON file.
    /// @return True on success.
    bool LoadManifest(const std::string& manifestPath);

    /// Return the archive root directory set at construction.
    [[nodiscard]] const std::string& GetArchiveRoot() const noexcept { return m_ArchiveRoot; }

private:
    // --- Helpers ---

    /// Generate a short unique ID string (timestamp + counter).
    /// Must be called with m_Mutex held.
    [[nodiscard]] std::string GenerateID();

    /// Return the current UTC time as an ISO 8601 string.
    [[nodiscard]] static std::string CurrentTimestamp();

    /// Escape a string for embedding inside a JSON double-quoted value.
    [[nodiscard]] static std::string JsonEscape(const std::string& s);

    // --- State ---

    std::string                  m_ArchiveRoot;
    mutable std::mutex           m_Mutex;
    std::vector<ArchiveRecord>   m_Records;
    uint64_t                     m_Counter{0};  // accessed only under m_Mutex
};

} // namespace Core::ArchiveSystem
