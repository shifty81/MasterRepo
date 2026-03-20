#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Editor {

// ──────────────────────────────────────────────────────────────
// ContentBrowser — file-system based asset browser panel
// ──────────────────────────────────────────────────────────────

struct ContentEntry {
    std::string name;
    std::string fullPath;
    bool        isDirectory = false;
    std::string extension;
    uint64_t    sizeBytes   = 0;
};

class ContentBrowser {
public:
    void SetRootPath(const std::string& path);
    const std::string& RootPath() const   { return m_root; }
    const std::string& CurrentPath() const { return m_current; }

    // Navigation
    bool NavigateTo(const std::string& path);
    bool NavigateUp();
    bool NavigateBack();
    bool NavigateForward();

    // Refresh directory listing
    void Refresh();

    const std::vector<ContentEntry>& Entries() const { return m_entries; }

    // Selection
    void Select(const std::string& path);
    void ClearSelection();
    const std::string& SelectedPath() const { return m_selected; }
    bool IsSelected(const std::string& path) const { return m_selected == path; }

    // Filtering
    void SetFilter(const std::string& extension); // empty = all
    void ClearFilter();

    // Callbacks
    using DoubleClickFn = std::function<void(const ContentEntry&)>;
    void SetDoubleClickCallback(DoubleClickFn fn);
    void FireDoubleClick(const ContentEntry& entry);

    // Asset type icon hint
    static std::string IconForEntry(const ContentEntry& entry);

private:
    std::string m_root;
    std::string m_current;
    std::string m_selected;
    std::string m_filter;
    std::vector<ContentEntry>  m_entries;
    std::vector<std::string>   m_backStack;
    std::vector<std::string>   m_fwdStack;
    DoubleClickFn              m_dblClickFn;

    void loadEntries(const std::string& path);
};

} // namespace Editor
