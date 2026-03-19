#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "Core/VersionSystem/VersionSnapshot.h"

namespace Core::VersionSystem {

/// Directory snapshot and version-history manager.
///
/// VersionSystem walks a directory tree and records the path, size, and
/// content hash of every regular file, producing a VersionSnapshot.
/// Snapshots can be accumulated into a history list and serialised to or from
/// a JSON file for persistence between sessions.
///
/// File hashing uses the FNV-1a 64-bit algorithm for speed; the result is
/// encoded as a 16-character lowercase hex string.
///
/// Usage:
///   Core::VersionSystem::VersionSystem vs;
///
///   auto snap = vs.CreateSnapshot("Engine/", "Before refactor");
///   vs.AddSnapshot(snap);
///   vs.SaveHistory("Versions/history.json");
///
///   // Later:
///   vs.LoadHistory("Versions/history.json");
///   for (const auto& s : vs.GetHistory()) { ... }
class VersionSystem {
public:
    VersionSystem()  = default;
    ~VersionSystem() = default;

    VersionSystem(const VersionSystem&)            = delete;
    VersionSystem& operator=(const VersionSystem&) = delete;

    /// Scan @p rootPath recursively and build a VersionSnapshot.
    /// Symlinks are followed; unreadable files are skipped silently.
    /// @param description  Human-readable note stored in the snapshot.
    [[nodiscard]] VersionSnapshot CreateSnapshot(const std::string& rootPath,
                                                  const std::string& description);

    /// Append a snapshot to the in-memory history list.
    void AddSnapshot(VersionSnapshot snap);

    /// Return the full snapshot history (oldest first).
    [[nodiscard]] std::vector<VersionSnapshot> GetHistory() const;

    /// Persist the history to a JSON file at @p path.
    /// Overwrites any existing file.
    [[nodiscard]] bool SaveHistory(const std::string& path) const;

    /// Load (replace) the history from a JSON file at @p path.
    bool LoadHistory(const std::string& path);

    /// Compute the FNV-1a 64-bit hash of the file at @p path and return it
    /// as a 16-character lowercase hex string.
    /// Returns an empty string if the file cannot be read.
    [[nodiscard]] std::string ComputeFileHash(const std::string& path) const;

private:
    // --- Helpers ---

    /// Generate a pseudo-UUID-like unique ID.
    [[nodiscard]] static std::string GenerateID();

    /// Return the current UTC time as an ISO 8601 string.
    [[nodiscard]] static std::string CurrentTimestamp();

    /// Escape a string for embedding inside a JSON double-quoted value.
    [[nodiscard]] static std::string JsonEscape(const std::string& s);

    // --- State ---

    mutable std::mutex             m_Mutex;
    std::vector<VersionSnapshot>   m_History;
};

} // namespace Core::VersionSystem
