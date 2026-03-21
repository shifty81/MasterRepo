#pragma once
/**
 * @file ProjectExplorer.h
 * @brief IDE project file tree explorer.
 *
 * ProjectExplorer maintains a virtual tree of the project directory:
 *   - Scan a root directory recursively
 *   - Filter by file extension
 *   - Expand / collapse directories
 *   - Selection state with callbacks
 *   - File-change watching via timestamp polling (PollChanges())
 *   - Serialise/restore expand state
 *
 * The tree is purely data-side; rendering is the caller's responsibility.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace IDE {

// ── Node ─────────────────────────────────────────────────────────────────
enum class NodeType : uint8_t {
    Directory,
    File,
};

struct ProjectNode {
    uint32_t            id{0};
    NodeType            type{NodeType::File};
    std::string         name;           ///< File or directory name
    std::string         path;           ///< Full absolute path
    std::string         extension;      ///< Lowercase, e.g. ".cpp" (empty for dirs)
    bool                expanded{false};
    bool                selected{false};
    uint32_t            parentId{0};    ///< 0 = root
    std::vector<uint32_t> childIds;
};

// ── Filter ────────────────────────────────────────────────────────────────
struct ExplorerFilter {
    std::vector<std::string> extensions;  ///< Empty = show all
    bool                     showHidden{false};
    std::string              nameContains;///< Substring filter on file name
};

// ── Callbacks ─────────────────────────────────────────────────────────────
using SelectionChangedFn = std::function<void(const ProjectNode& node)>;
using FileChangedFn      = std::function<void(const std::string& path,
                                               bool added)>;

/// ProjectExplorer — virtual project file tree with watch support.
class ProjectExplorer {
public:
    ProjectExplorer();
    ~ProjectExplorer();

    // ── root / scan ───────────────────────────────────────────
    void SetRoot(const std::string& rootPath);
    const std::string& RootPath() const;

    /// Rescan the directory tree from root.
    void Refresh();

    // ── filter ────────────────────────────────────────────────
    void SetFilter(const ExplorerFilter& filter);
    const ExplorerFilter& Filter() const;

    // ── tree access ───────────────────────────────────────────
    const ProjectNode* GetNode(uint32_t id) const;
    const ProjectNode* FindByPath(const std::string& path) const;

    /// Root-level nodes (direct children of root dir).
    std::vector<uint32_t> RootNodes() const;

    /// Children of a given node (filtered).
    std::vector<uint32_t> ChildrenOf(uint32_t id) const;

    size_t TotalNodes() const;
    size_t FileCount() const;
    size_t DirectoryCount() const;

    // ── expand / collapse ─────────────────────────────────────
    void Expand(uint32_t id);
    void Collapse(uint32_t id);
    void ExpandAll();
    void CollapseAll();
    bool IsExpanded(uint32_t id) const;

    // ── selection ─────────────────────────────────────────────
    void Select(uint32_t id);
    void Deselect(uint32_t id);
    void ClearSelection();
    std::vector<uint32_t> SelectedNodes() const;
    const ProjectNode* FirstSelected() const;

    // ── watch / poll ─────────────────────────────────────────
    /// Poll for filesystem changes; fires FileChangedFn callbacks.
    void PollChanges();

    // ── callbacks ─────────────────────────────────────────────
    void OnSelectionChanged(SelectionChangedFn fn);
    void OnFileChanged(FileChangedFn fn);

    // ── state persistence ─────────────────────────────────────
    std::vector<std::string> GetExpandedPaths() const;
    void RestoreExpandedPaths(const std::vector<std::string>& paths);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
