#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Version kinds
// ──────────────────────────────────────────────────────────────

enum class VersionKind { Snapshot, SceneFrame, ObjectFrame, SystemFrame };

// ──────────────────────────────────────────────────────────────
// Version entry — metadata for one stored version blob
// ──────────────────────────────────────────────────────────────

struct VersionEntry {
    uint64_t    id          = 0;
    VersionKind kind        = VersionKind::Snapshot;
    std::string tag;          // user label or auto "auto_YYYYMMDD_HHMMSS"
    std::string author;
    std::string timestamp;    // ISO-8601
    std::string description;
    std::string dataPath;     // relative path to stored blob
    uint64_t    parentId    = 0;
    bool        pinned      = false;
};

// ──────────────────────────────────────────────────────────────
// Diff result between two versions
// ──────────────────────────────────────────────────────────────

struct VersionDiff {
    uint64_t                 fromId = 0;
    uint64_t                 toId   = 0;
    std::vector<std::string> addedKeys;
    std::vector<std::string> removedKeys;
    std::vector<std::string> modifiedKeys;
};

// ──────────────────────────────────────────────────────────────
// VersionManager — create, query, restore and diff version entries
// ──────────────────────────────────────────────────────────────

class VersionManager {
public:
    explicit VersionManager(const std::string& versionRoot);  // e.g. "Versions/"

    // Create
    uint64_t CreateSnapshot(const std::string& tag,
                             const std::string& description,
                             const std::string& dataJson,
                             const std::string& author = "");
    uint64_t CreateFrame(VersionKind kind,
                          const std::string& tag,
                          const std::string& dataJson,
                          uint64_t parentId = 0);

    // Query
    std::vector<VersionEntry> GetAll()               const;
    std::vector<VersionEntry> GetByKind(VersionKind kind) const;
    const VersionEntry*       GetById(uint64_t id)   const;
    const VersionEntry*       GetByTag(const std::string& tag) const;
    std::vector<VersionEntry> GetPinned()             const;
    std::vector<VersionEntry> GetRecent(size_t count) const;

    // Restore
    std::string LoadData(uint64_t id)                                    const;
    bool        RestoreTo(uint64_t id, const std::string& outputPath)   const;

    // Diff
    VersionDiff Diff(uint64_t fromId, uint64_t toId) const;

    // Modify
    bool Pin           (uint64_t id, bool pin = true);
    bool Delete        (uint64_t id);   // only non-pinned
    bool SetDescription(uint64_t id, const std::string& desc);

    // Persistence
    bool SaveIndex(const std::string& indexPath = "") const;
    bool LoadIndex(const std::string& indexPath = "");

    size_t Count() const;

private:
    std::string kindSubdir(VersionKind kind) const;
    std::string entryPath(const VersionEntry& e) const;
    static std::string CurrentTimestamp();

    // Simple key extraction from a JSON string: returns { "key": "value" } pairs
    static std::vector<std::pair<std::string,std::string>> ParseFlatJson(const std::string& json);

    std::string               m_root;
    std::vector<VersionEntry> m_entries;
    uint64_t                  m_nextId = 1;
};

} // namespace Core
