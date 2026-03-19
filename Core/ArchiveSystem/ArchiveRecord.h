#pragma once

#include <string>

namespace Core::ArchiveSystem {

/// Metadata record for a single archived file.
///
/// Each time ArchiveSystem::Archive() copies a file into the archive store it
/// creates one ArchiveRecord that is appended to the in-memory manifest and
/// optionally persisted via SaveManifest().
struct ArchiveRecord {
    /// Unique identifier for this archive entry (populated by ArchiveSystem).
    std::string ID;

    /// Absolute or repo-relative path the file was copied from.
    std::string OriginalPath;

    /// Path inside the archive directory where the copy lives.
    std::string ArchivePath;

    /// ISO 8601 UTC timestamp of when the file was archived (e.g. "2024-01-15T10:30:00Z").
    std::string Timestamp;

    /// Human-readable reason the file was archived.
    std::string Reason;

    /// Comma-separated tags for categorisation (e.g. "deprecated,refactor").
    std::string Tags;
};

} // namespace Core::ArchiveSystem
