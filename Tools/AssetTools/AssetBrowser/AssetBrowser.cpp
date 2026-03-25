#include "Tools/AssetTools/AssetBrowser/AssetBrowser.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Tools {

// ── Impl ─────────────────────────────────────────────────────────────────────

struct AssetBrowser::Impl {
    std::string rootPath;
    std::string currentPath;
    std::string selectedPath;

    std::vector<std::string> backStack;
    std::vector<std::string> forwardStack;

    AssetFilter              filter;
    std::vector<AssetEntry>  entries;   // filtered snapshot

    std::function<void(const std::string&)> onAssetOpened;
    std::function<void(const std::string&)> onSelectionChanged;

    // ── helpers ──────────────────────────────────────────────────────────────

    static AssetEntry MakeEntry(const fs::directory_entry& de) {
        AssetEntry e;
        e.fullPath    = de.path().string();
        e.name        = de.path().filename().string();
        e.isDirectory = de.is_directory();
        if (!e.isDirectory && de.is_regular_file()) {
            std::error_code ec;
            e.sizeBytes = static_cast<uint64_t>(de.file_size(ec));
            e.extension = de.path().extension().string();
        }
        return e;
    }

    static std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    bool PassesFilter(const AssetEntry& e) const {
        if (e.isDirectory && !filter.showDirectories) return false;

        if (!filter.nameSubstring.empty()) {
            if (ToLower(e.name).find(ToLower(filter.nameSubstring)) == std::string::npos)
                return false;
        }
        if (!filter.extensions.empty() && !e.isDirectory) {
            bool found = false;
            for (const auto& ext : filter.extensions) {
                if (ToLower(e.extension) == ToLower(ext)) { found = true; break; }
            }
            if (!found) return false;
        }
        return true;
    }

    void RebuildEntries() {
        entries.clear();
        if (currentPath.empty()) return;

        std::error_code ec;
        for (const auto& de : fs::directory_iterator(currentPath, ec)) {
            if (ec) break;
            AssetEntry e = MakeEntry(de);
            if (PassesFilter(e)) entries.push_back(std::move(e));
        }
        // Directories first, then alphabetical
        std::stable_sort(entries.begin(), entries.end(), [](const AssetEntry& a, const AssetEntry& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
    }
};

// ── AssetBrowser ─────────────────────────────────────────────────────────────

AssetBrowser::AssetBrowser() : m_impl(new Impl{}) {}
AssetBrowser::~AssetBrowser() { delete m_impl; }

void AssetBrowser::SetRootPath(const std::string& path) {
    m_impl->rootPath = path;
    m_impl->backStack.clear();
    m_impl->forwardStack.clear();
    NavigateTo(path);
}

const std::string& AssetBrowser::RootPath() const { return m_impl->rootPath; }

void AssetBrowser::NavigateTo(const std::string& path) {
    if (!m_impl->currentPath.empty())
        m_impl->backStack.push_back(m_impl->currentPath);
    m_impl->forwardStack.clear();
    m_impl->currentPath = path;
    m_impl->RebuildEntries();
}

void AssetBrowser::NavigateUp() {
    fs::path p(m_impl->currentPath);
    if (p.has_parent_path() && p != p.parent_path())
        NavigateTo(p.parent_path().string());
}

void AssetBrowser::NavigateBack() {
    if (m_impl->backStack.empty()) return;
    m_impl->forwardStack.push_back(m_impl->currentPath);
    m_impl->currentPath = m_impl->backStack.back();
    m_impl->backStack.pop_back();
    m_impl->RebuildEntries();
}

void AssetBrowser::NavigateForward() {
    if (m_impl->forwardStack.empty()) return;
    m_impl->backStack.push_back(m_impl->currentPath);
    m_impl->currentPath = m_impl->forwardStack.back();
    m_impl->forwardStack.pop_back();
    m_impl->RebuildEntries();
}

const std::string& AssetBrowser::CurrentPath() const { return m_impl->currentPath; }

void AssetBrowser::Refresh() { m_impl->RebuildEntries(); }

const std::vector<AssetEntry>& AssetBrowser::Entries() const { return m_impl->entries; }

void AssetBrowser::SetFilter(const AssetFilter& filter) {
    m_impl->filter = filter;
    m_impl->RebuildEntries();
}

void AssetBrowser::ClearFilter() {
    m_impl->filter = {};
    m_impl->RebuildEntries();
}

void AssetBrowser::Select(const std::string& path) {
    m_impl->selectedPath = path;
    if (m_impl->onSelectionChanged) m_impl->onSelectionChanged(path);
}

const std::string& AssetBrowser::SelectedPath() const { return m_impl->selectedPath; }

void AssetBrowser::ClearSelection() {
    m_impl->selectedPath.clear();
    if (m_impl->onSelectionChanged) m_impl->onSelectionChanged("");
}

void AssetBrowser::OpenAsset(const std::string& path) {
    if (m_impl->onAssetOpened) m_impl->onAssetOpened(path);
}

bool AssetBrowser::ImportAsset(const std::string& sourcePath, const std::string& destPath) {
    std::error_code ec;
    fs::copy_file(sourcePath, destPath,
                  fs::copy_options::overwrite_existing, ec);
    if (!ec) Refresh();
    return !ec;
}

bool AssetBrowser::DeleteAsset(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
    if (!ec) Refresh();
    return !ec;
}

bool AssetBrowser::RenameAsset(const std::string& oldPath, const std::string& newPath) {
    std::error_code ec;
    fs::rename(oldPath, newPath, ec);
    if (!ec) Refresh();
    return !ec;
}

bool AssetBrowser::CreateFolder(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    if (!ec) Refresh();
    return !ec;
}

std::vector<AssetEntry> AssetBrowser::SearchAssets(const std::string& query) const {
    std::string low = Impl::ToLower(query);
    std::vector<AssetEntry> results;
    if (m_impl->rootPath.empty()) return results;

    std::error_code ec;
    for (const auto& de : fs::recursive_directory_iterator(m_impl->rootPath, ec)) {
        if (ec) break;
        AssetEntry e = Impl::MakeEntry(de);
        if (Impl::ToLower(e.name).find(low) != std::string::npos)
            results.push_back(std::move(e));
    }
    return results;
}

void AssetBrowser::OnAssetOpened(std::function<void(const std::string&)> callback) {
    m_impl->onAssetOpened = std::move(callback);
}

void AssetBrowser::OnSelectionChanged(std::function<void(const std::string&)> callback) {
    m_impl->onSelectionChanged = std::move(callback);
}

AssetBrowserStats AssetBrowser::Stats() const {
    AssetBrowserStats s{};
    if (m_impl->rootPath.empty()) return s;

    std::error_code ec;
    for (const auto& de : fs::recursive_directory_iterator(m_impl->rootPath, ec)) {
        if (ec) break;
        if (de.is_directory()) {
            ++s.totalDirs;
        } else if (de.is_regular_file()) {
            ++s.totalFiles;
            s.totalSizeBytes += static_cast<uint64_t>(de.file_size(ec));
            if (ec) ec.clear();
        }
    }
    return s;
}

} // namespace Tools
