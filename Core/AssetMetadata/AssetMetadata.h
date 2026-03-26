#pragma once
/**
 * @file AssetMetadata.h
 * @brief Enhanced versioned asset metadata with AI reasoning annotations.
 *
 * Extends standard asset records with:
 *   - AI-generated notes explaining why the asset was created or modified
 *   - Version history with author (human or AI) and diff summary
 *   - Tag-based search and filtering
 *   - JSON serialisation for persistence in `WorkspaceState/asset_meta.json`
 *
 * Typical usage:
 * @code
 *   AssetMetadata meta;
 *   meta.Init("WorkspaceState/asset_meta.json");
 *   meta.Register("Ships/Cruiser.json", "ship_prefab");
 *   meta.Annotate("Ships/Cruiser.json", "AI created 20-gun cruiser hull from PartLibrary v3");
 *   meta.Commit("Ships/Cruiser.json", "Added engine nozzles", "AI");
 *   meta.AddTag("Ships/Cruiser.json", "warship");
 *   auto rec = meta.Get("Ships/Cruiser.json");
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Core {

// ── Version entry in an asset's history ──────────────────────────────────────

struct AssetVersion {
    uint32_t    version{1};
    std::string author;          ///< "Human", "AI", or agent name
    std::string changeSummary;
    std::string aiReasoning;     ///< why AI made this change (may be empty)
    uint64_t    timestampMs{0};
};

// ── Full asset metadata record ────────────────────────────────────────────────

struct AssetRecord {
    std::string              path;          ///< repo-relative path
    std::string              assetType;     ///< e.g. "ship_prefab", "texture"
    std::string              aiAnnotation;  ///< latest AI-generated note
    std::vector<std::string> tags;
    std::vector<AssetVersion> history;
    uint64_t                 createdMs{0};
    uint64_t                 modifiedMs{0};
};

// ── AssetMetadata ─────────────────────────────────────────────────────────────

class AssetMetadata {
public:
    AssetMetadata();
    ~AssetMetadata();

    /// Load existing metadata from disk.
    void Init(const std::string& persistPath = "WorkspaceState/asset_meta.json");
    void Shutdown();

    // ── Registration ──────────────────────────────────────────────────────────

    void Register(const std::string& path, const std::string& assetType);
    void Unregister(const std::string& path);
    bool IsRegistered(const std::string& path) const;

    // ── Mutation ──────────────────────────────────────────────────────────────

    void Annotate(const std::string& path, const std::string& aiNote);
    void AddTag(const std::string& path, const std::string& tag);
    void RemoveTag(const std::string& path, const std::string& tag);

    /// Record a new version entry.
    void Commit(const std::string& path,
                const std::string& changeSummary,
                const std::string& author = "Human",
                const std::string& aiReasoning = "");

    // ── Query ─────────────────────────────────────────────────────────────────

    AssetRecord                Get(const std::string& path) const;
    std::vector<AssetRecord>   All()                        const;
    std::vector<AssetRecord>   ByTag(const std::string& tag) const;
    std::vector<AssetRecord>   ByType(const std::string& type) const;
    std::vector<AssetVersion>  History(const std::string& path) const;

    // ── Persistence ───────────────────────────────────────────────────────────

    void Save() const;
    void Load();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Core
