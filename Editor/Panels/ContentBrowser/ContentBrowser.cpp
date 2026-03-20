#include "Editor/Panels/ContentBrowser/ContentBrowser.h"
#include <filesystem>
#include <algorithm>

namespace Editor {

void ContentBrowser::SetRootPath(const std::string& path) {
    m_root = path;
    m_current = path;
    m_backStack.clear();
    m_fwdStack.clear();
    Refresh();
}

bool ContentBrowser::NavigateTo(const std::string& path) {
    if (!std::filesystem::is_directory(path)) return false;
    m_backStack.push_back(m_current);
    m_fwdStack.clear();
    m_current = path;
    Refresh();
    return true;
}

bool ContentBrowser::NavigateUp() {
    auto parent = std::filesystem::path(m_current).parent_path().string();
    if (parent == m_current) return false;
    return NavigateTo(parent);
}

bool ContentBrowser::NavigateBack() {
    if (m_backStack.empty()) return false;
    m_fwdStack.push_back(m_current);
    m_current = m_backStack.back();
    m_backStack.pop_back();
    Refresh();
    return true;
}

bool ContentBrowser::NavigateForward() {
    if (m_fwdStack.empty()) return false;
    m_backStack.push_back(m_current);
    m_current = m_fwdStack.back();
    m_fwdStack.pop_back();
    Refresh();
    return true;
}

void ContentBrowser::loadEntries(const std::string& path) {
    m_entries.clear();
    if (!std::filesystem::exists(path)) return;
    for (const auto& e : std::filesystem::directory_iterator(path)) {
        ContentEntry ce;
        ce.name       = e.path().filename().string();
        ce.fullPath   = e.path().string();
        ce.isDirectory = e.is_directory();
        if (!ce.isDirectory) {
            ce.extension = e.path().extension().string();
            ce.sizeBytes = e.is_regular_file() ? e.file_size() : 0;
        }
        // Apply filter
        if (!m_filter.empty() && !ce.isDirectory && ce.extension != m_filter)
            continue;
        m_entries.push_back(std::move(ce));
    }
    // Sort: directories first, then by name
    std::sort(m_entries.begin(), m_entries.end(),
        [](const ContentEntry& a, const ContentEntry& b){
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
}

void ContentBrowser::Refresh() { loadEntries(m_current); }

void ContentBrowser::Select(const std::string& path) { m_selected = path; }
void ContentBrowser::ClearSelection()                 { m_selected.clear(); }

void ContentBrowser::SetFilter(const std::string& ext) { m_filter = ext; Refresh(); }
void ContentBrowser::ClearFilter()                     { m_filter.clear(); Refresh(); }

void ContentBrowser::SetDoubleClickCallback(DoubleClickFn fn) {
    m_dblClickFn = std::move(fn);
}

void ContentBrowser::FireDoubleClick(const ContentEntry& entry) {
    if (entry.isDirectory) NavigateTo(entry.fullPath);
    else if (m_dblClickFn) m_dblClickFn(entry);
}

std::string ContentBrowser::IconForEntry(const ContentEntry& entry) {
    if (entry.isDirectory) return "[DIR]";
    const std::string& ext = entry.extension;
    if (ext==".h"||ext==".cpp"||ext==".cc") return "[CODE]";
    if (ext==".png"||ext==".jpg"||ext==".tga") return "[IMG]";
    if (ext==".atlasb") return "[ASSET]";
    if (ext==".wav"||ext==".ogg")           return "[AUDIO]";
    if (ext==".obj"||ext==".fbx"||ext==".gltf") return "[MESH]";
    if (ext==".lua"||ext==".py")            return "[SCRIPT]";
    return "[FILE]";
}

} // namespace Editor
