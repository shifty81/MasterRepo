#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Tools {

// ── Data types ────────────────────────────────────────────────────────────────

struct AssetEntry {
    std::string name;
    std::string fullPath;
    std::string extension;
    uint64_t    sizeBytes{0};
    bool        isDirectory{false};
    std::string thumbnailPath;
};

struct AssetFilter {
    std::vector<std::string> extensions;     // e.g. {".png", ".wav"}
    std::string              nameSubstring;  // case-insensitive substring match
    bool                     showDirectories{true};
};

struct AssetBrowserStats {
    uint64_t totalFiles{0};
    uint64_t totalDirs{0};
    uint64_t totalSizeBytes{0};
};

// ── AssetBrowser ─────────────────────────────────────────────────────────────

class AssetBrowser {
public:
    AssetBrowser();
    ~AssetBrowser();

    // Root / navigation
    void               SetRootPath(const std::string& path);
    const std::string& RootPath() const;

    void               NavigateTo(const std::string& path);
    void               NavigateUp();
    void               NavigateBack();
    void               NavigateForward();
    const std::string& CurrentPath() const;
    void               Refresh();

    // Listing
    const std::vector<AssetEntry>& Entries() const;
    void SetFilter(const AssetFilter& filter);
    void ClearFilter();

    // Selection
    void               Select(const std::string& path);
    const std::string& SelectedPath() const;
    void               ClearSelection();

    // Operations
    void OpenAsset(const std::string& path);
    bool ImportAsset(const std::string& sourcePath, const std::string& destPath);
    bool DeleteAsset(const std::string& path);
    bool RenameAsset(const std::string& oldPath, const std::string& newPath);
    bool CreateFolder(const std::string& path);

    // Search
    std::vector<AssetEntry> SearchAssets(const std::string& query) const;

    // Callbacks
    void OnAssetOpened(std::function<void(const std::string&)> callback);
    void OnSelectionChanged(std::function<void(const std::string&)> callback);

    // Stats
    AssetBrowserStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Tools
