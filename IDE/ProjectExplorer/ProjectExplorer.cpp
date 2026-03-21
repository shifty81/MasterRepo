#include "IDE/ProjectExplorer/ProjectExplorer.h"
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <cctype>

namespace IDE {

namespace fs = std::filesystem;

struct ProjectExplorer::Impl {
    std::string   rootPath;
    ExplorerFilter filter;

    std::unordered_map<uint32_t, ProjectNode> nodes;
    std::vector<uint32_t>                     rootNodes;
    uint32_t                                  nextId{1};

    std::unordered_map<std::string, std::filesystem::file_time_type> mtimes;

    std::vector<SelectionChangedFn> selectionCbs;
    std::vector<FileChangedFn>      fileChangedCbs;

    std::string toLower(std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    bool passesFilter(const fs::directory_entry& entry) const {
        if (!filter.showHidden) {
            const std::string name = entry.path().filename().string();
            if (!name.empty() && name[0] == '.') return false;
        }
        if (!filter.nameContains.empty()) {
            std::string n = entry.path().filename().string();
            // Case-insensitive contains
            std::string nl = n, fl = filter.nameContains;
            for (auto& c : nl) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            for (auto& c : fl) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (nl.find(fl) == std::string::npos) return false;
        }
        if (!filter.extensions.empty() && entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            bool match = false;
            for (const auto& fe : filter.extensions) {
                std::string fle = fe;
                for (auto& c : fle) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (ext == fle) { match = true; break; }
            }
            if (!match) return false;
        }
        return true;
    }

    uint32_t addNode(const fs::directory_entry& entry, uint32_t parentId) {
        ProjectNode node;
        node.id       = nextId++;
        node.parentId = parentId;
        node.name     = entry.path().filename().string();
        node.path     = entry.path().string();
        node.type     = entry.is_directory() ? NodeType::Directory : NodeType::File;
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            node.extension = ext;
        }
        nodes[node.id] = node;
        return node.id;
    }

    void scanDir(const fs::path& dir, uint32_t parentId) {
        std::error_code ec;
        std::vector<fs::directory_entry> entries;
        for (const auto& e : fs::directory_iterator(dir, ec)) {
            if (passesFilter(e) || e.is_directory()) entries.push_back(e);
        }
        // Dirs first, then files, both sorted alphabetically
        std::sort(entries.begin(), entries.end(), [](const fs::directory_entry& a, const fs::directory_entry& b){
            bool aDir = a.is_directory(), bDir = b.is_directory();
            if (aDir != bDir) return aDir > bDir;
            return a.path().filename() < b.path().filename();
        });

        for (const auto& e : entries) {
            if (!passesFilter(e) && !e.is_directory()) continue;
            uint32_t id = addNode(e, parentId);
            if (parentId == 0) rootNodes.push_back(id);
            else if (nodes.count(parentId)) nodes[parentId].childIds.push_back(id);
            if (e.is_directory()) scanDir(e.path(), id);
            // Track mtimes for watch
            if (e.is_regular_file()) {
                std::error_code ec2;
                mtimes[e.path().string()] = fs::last_write_time(e.path(), ec2);
            }
        }
    }
};

ProjectExplorer::ProjectExplorer() : m_impl(new Impl()) {}
ProjectExplorer::~ProjectExplorer() { delete m_impl; }

void ProjectExplorer::SetRoot(const std::string& rootPath) {
    m_impl->rootPath = rootPath;
}
const std::string& ProjectExplorer::RootPath() const { return m_impl->rootPath; }

void ProjectExplorer::Refresh() {
    m_impl->nodes.clear();
    m_impl->rootNodes.clear();
    m_impl->nextId = 1;
    m_impl->mtimes.clear();
    if (m_impl->rootPath.empty()) return;
    std::error_code ec;
    fs::path root(m_impl->rootPath);
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return;
    m_impl->scanDir(root, 0);
}

void ProjectExplorer::SetFilter(const ExplorerFilter& f) {
    m_impl->filter = f;
}
const ExplorerFilter& ProjectExplorer::Filter() const { return m_impl->filter; }

const ProjectNode* ProjectExplorer::GetNode(uint32_t id) const {
    auto it = m_impl->nodes.find(id);
    return it != m_impl->nodes.end() ? &it->second : nullptr;
}

const ProjectNode* ProjectExplorer::FindByPath(const std::string& path) const {
    for (const auto& [id, n] : m_impl->nodes)
        if (n.path == path) return &n;
    return nullptr;
}

std::vector<uint32_t> ProjectExplorer::RootNodes() const {
    return m_impl->rootNodes;
}

std::vector<uint32_t> ProjectExplorer::ChildrenOf(uint32_t id) const {
    auto it = m_impl->nodes.find(id);
    if (it == m_impl->nodes.end()) return {};
    return it->second.childIds;
}

size_t ProjectExplorer::TotalNodes()     const { return m_impl->nodes.size(); }
size_t ProjectExplorer::FileCount()      const {
    size_t n = 0;
    for (const auto& [_,nd] : m_impl->nodes) if (nd.type == NodeType::File) ++n;
    return n;
}
size_t ProjectExplorer::DirectoryCount() const {
    size_t n = 0;
    for (const auto& [_,nd] : m_impl->nodes) if (nd.type == NodeType::Directory) ++n;
    return n;
}

void ProjectExplorer::Expand(uint32_t id) {
    auto it = m_impl->nodes.find(id);
    if (it != m_impl->nodes.end()) it->second.expanded = true;
}
void ProjectExplorer::Collapse(uint32_t id) {
    auto it = m_impl->nodes.find(id);
    if (it != m_impl->nodes.end()) it->second.expanded = false;
}
void ProjectExplorer::ExpandAll()   { for (auto& [_,n] : m_impl->nodes) n.expanded = true; }
void ProjectExplorer::CollapseAll() { for (auto& [_,n] : m_impl->nodes) n.expanded = false; }
bool ProjectExplorer::IsExpanded(uint32_t id) const {
    auto it = m_impl->nodes.find(id);
    return it != m_impl->nodes.end() && it->second.expanded;
}

void ProjectExplorer::Select(uint32_t id) {
    auto it = m_impl->nodes.find(id);
    if (it == m_impl->nodes.end()) return;
    it->second.selected = true;
    for (auto& cb : m_impl->selectionCbs) cb(it->second);
}
void ProjectExplorer::Deselect(uint32_t id) {
    auto it = m_impl->nodes.find(id);
    if (it != m_impl->nodes.end()) it->second.selected = false;
}
void ProjectExplorer::ClearSelection() {
    for (auto& [_,n] : m_impl->nodes) n.selected = false;
}
std::vector<uint32_t> ProjectExplorer::SelectedNodes() const {
    std::vector<uint32_t> out;
    for (const auto& [id,n] : m_impl->nodes) if (n.selected) out.push_back(id);
    return out;
}
const ProjectNode* ProjectExplorer::FirstSelected() const {
    for (const auto& [_,n] : m_impl->nodes) if (n.selected) return &n;
    return nullptr;
}

void ProjectExplorer::PollChanges() {
    if (m_impl->rootPath.empty()) return;
    std::error_code ec;
    fs::path root(m_impl->rootPath);
    if (!fs::exists(root, ec)) return;

    for (const auto& e : fs::recursive_directory_iterator(root, ec)) {
        if (!e.is_regular_file()) continue;
        std::string p = e.path().string();
        auto mtime = fs::last_write_time(e.path(), ec);
        if (ec) continue;
        auto it = m_impl->mtimes.find(p);
        if (it == m_impl->mtimes.end()) {
            // New file
            m_impl->mtimes[p] = mtime;
            for (auto& cb : m_impl->fileChangedCbs) cb(p, true);
        } else if (mtime != it->second) {
            it->second = mtime;
            for (auto& cb : m_impl->fileChangedCbs) cb(p, false);
        }
    }
}

void ProjectExplorer::OnSelectionChanged(SelectionChangedFn fn) {
    m_impl->selectionCbs.push_back(std::move(fn));
}
void ProjectExplorer::OnFileChanged(FileChangedFn fn) {
    m_impl->fileChangedCbs.push_back(std::move(fn));
}

std::vector<std::string> ProjectExplorer::GetExpandedPaths() const {
    std::vector<std::string> out;
    for (const auto& [_,n] : m_impl->nodes)
        if (n.expanded) out.push_back(n.path);
    std::sort(out.begin(), out.end());
    return out;
}

void ProjectExplorer::RestoreExpandedPaths(const std::vector<std::string>& paths) {
    for (auto& [_,n] : m_impl->nodes) {
        n.expanded = std::find(paths.begin(), paths.end(), n.path) != paths.end();
    }
}

} // namespace IDE
