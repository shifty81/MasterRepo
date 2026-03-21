#pragma once
// PluginMarketplace — Phase 10.11: Import/export plugin packs
//
// Manages a local catalogue of installable plugin packs.  Each pack bundles
// one or more Atlas Engine plugins (AI, Builder, Editor, Runtime) along with
// metadata, dependencies, and optional asset payload.
//
// Marketplace sources can be:
//   • A local directory on disk  (offline-first)
//   • A remote JSON index URL   (requires network, optional)
//
// The class is intentionally thin — it delegates actual plugin loading to
// Core::PluginSystem and uses Core::Database for local catalogue persistence.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Plugins {

// ─────────────────────────────────────────────────────────────────────────────
// Data model
// ─────────────────────────────────────────────────────────────────────────────

enum class PackState {
    Available,      // listed but not installed
    Installed,      // installed and ready to load
    UpdateAvailable,// installed but a newer version exists in the catalogue
    Disabled,       // installed but manually disabled
    Error,          // installation or load failed
};

struct PluginPackDependency {
    std::string packId;
    std::string minVersion; // semver string, e.g. "1.2.0"
};

struct PluginPackEntry {
    std::string id;           // unique identifier, e.g. "com.atlas.ai_assistant"
    std::string displayName;
    std::string description;
    std::string author;
    std::string version;      // semver
    std::string category;     // "AI", "Builder", "Editor", "Runtime", "Tools"
    std::string sourceUrl;    // download / local path
    std::string iconPath;     // 64×64 PNG path (optional)
    std::string licenseType;  // "MIT", "GPL", "Commercial", …
    uint64_t    downloadBytes = 0;
    bool        requiresRestart = false;
    std::vector<PluginPackDependency> dependencies;
    PackState   state = PackState::Available;
    std::string installedPath; // filled when Installed
    std::string errorMessage;  // filled when Error
};

// ─────────────────────────────────────────────────────────────────────────────
// Marketplace config
// ─────────────────────────────────────────────────────────────────────────────

struct MarketplaceConfig {
    std::string installDir    = "Plugins/Installed";    // where packs are unpacked
    std::string catalogueFile = "Config/marketplace.json"; // local cache
    // Remote index URL (empty = offline only)
    std::string remoteIndexUrl;
    // Local source directories scanned at startup
    std::vector<std::string> localSourceDirs;
};

// ─────────────────────────────────────────────────────────────────────────────
// PluginMarketplace
// ─────────────────────────────────────────────────────────────────────────────

class PluginMarketplace {
public:
    explicit PluginMarketplace(MarketplaceConfig cfg = {});
    ~PluginMarketplace();

    // ── Catalogue ────────────────────────────────────────────────────────────

    /// Scan local source directories and (optionally) fetch remote index.
    /// Merges results and persists to cfg.catalogueFile.
    void RefreshCatalogue();

    /// Return all known packs (installed + available).
    std::vector<PluginPackEntry> GetAllPacks() const;

    /// Return packs filtered by state.
    std::vector<PluginPackEntry> GetPacksByState(PackState state) const;

    /// Return packs filtered by category (case-insensitive).
    std::vector<PluginPackEntry> GetPacksByCategory(const std::string& cat) const;

    /// Look up a single pack by ID.  Returns nullptr if not found.
    const PluginPackEntry* FindPack(const std::string& id) const;

    // ── Install / uninstall ───────────────────────────────────────────────────

    struct InstallResult {
        bool        success = false;
        std::string packId;
        std::string installedPath;
        std::string errorMessage;
    };

    /// Install a pack from its sourceUrl/path.
    /// Progress callback receives [0.0, 1.0] values.
    InstallResult Install(const std::string& packId,
                          std::function<void(float)> progressCb = {});

    /// Uninstall (remove files) for the given pack ID.
    bool Uninstall(const std::string& packId);

    /// Update an installed pack to the version in the catalogue.
    InstallResult Update(const std::string& packId,
                         std::function<void(float)> progressCb = {});

    // ── Enable / disable ─────────────────────────────────────────────────────

    bool Enable (const std::string& packId);
    bool Disable(const std::string& packId);

    // ── Export ───────────────────────────────────────────────────────────────

    /// Bundle an installed pack back into a distributable archive (.atlasPack).
    /// Returns path to the created archive or empty string on failure.
    std::string Export(const std::string& packId,
                       const std::string& outputDir);

    // ── Persistence ──────────────────────────────────────────────────────────

    /// Save current catalogue state to disk.
    bool SaveCatalogue() const;

    /// Load catalogue from disk.
    bool LoadCatalogue();

    // ── Callbacks ────────────────────────────────────────────────────────────

    using CatalogueChangedFn = std::function<void()>;
    void SetCatalogueChangedCallback(CatalogueChangedFn cb);

private:
    MarketplaceConfig              m_cfg;
    std::vector<PluginPackEntry>   m_packs;
    CatalogueChangedFn             m_onChanged;

    bool ScanLocalDir(const std::string& dir);
    bool ParsePackManifest(const std::string& jsonPath, PluginPackEntry& out);
    std::string PackArchivePath(const PluginPackEntry& pack) const;
    void NotifyChanged();
};

} // namespace Plugins
