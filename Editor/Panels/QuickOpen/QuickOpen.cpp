#include "Editor/Panels/QuickOpen/QuickOpen.h"
#include "Engine/Core/Logger.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace Editor {

void QuickOpen::Show() {
    m_visible   = true;
    m_query.clear();
    m_selection = 0;
    Rebuild();
}

void QuickOpen::Hide()   { m_visible = false; }
void QuickOpen::Toggle() { m_visible ? Hide() : Show(); }
void QuickOpen::Cancel() { Hide(); }

void QuickOpen::TypeChar(char c) {
    if (!m_visible) return;
    m_query += c;
    m_selection = 0;
    Rebuild();
}

void QuickOpen::Backspace() {
    if (!m_query.empty()) { m_query.pop_back(); Rebuild(); }
}

void QuickOpen::Clear() { m_query.clear(); m_selection = 0; Rebuild(); }

void QuickOpen::MoveSelectionDown() {
    if (!m_results.empty())
        m_selection = (m_selection + 1) % (int)m_results.size();
}

void QuickOpen::MoveSelectionUp() {
    if (!m_results.empty())
        m_selection = (m_selection - 1 + (int)m_results.size()) % (int)m_results.size();
}

void QuickOpen::Confirm() {
    if (!m_visible || m_results.empty()) { Hide(); return; }
    int idx = std::clamp(m_selection, 0, (int)m_results.size() - 1);
    const auto& item = m_results[idx];
    Engine::Core::Logger::Info("QuickOpen: " + item.label + " [" + item.path + "]");
    if (m_confirmFn) m_confirmFn(item);
    Hide();
}

// ── Data source ────────────────────────────────────────────────────────────
void QuickOpen::SetAssetsRoot(const std::string& path) {
    m_assetsRoot = path;
}

void QuickOpen::ScanAssets() {
    // Remove old asset items, keep others
    m_allItems.erase(
        std::remove_if(m_allItems.begin(), m_allItems.end(),
            [](const QuickOpenItem& i){ return i.kind == QuickOpenItem::Kind::Asset; }),
        m_allItems.end());

    if (m_assetsRoot.empty() || !std::filesystem::exists(m_assetsRoot)) return;

    static const std::vector<std::string> kExts = {
        ".obj",".fbx",".gltf",".glb",
        ".png",".jpg",".hdr",".tga",
        ".json",".scene",".mat",".h",".cpp"
    };

    try {
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(m_assetsRoot)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            bool isKnown = false;
            for (const auto& k : kExts) if (ext == k) { isKnown = true; break; }
            if (!isKnown) continue;

            QuickOpenItem item;
            item.kind  = QuickOpenItem::Kind::Asset;
            item.label = entry.path().filename().string();
            item.path  = entry.path().string();
            m_allItems.push_back(std::move(item));
        }
    } catch (...) {}

    Rebuild();
}

void QuickOpen::SetEntities(const std::vector<std::string>& names) {
    // Remove old entity items
    m_allItems.erase(
        std::remove_if(m_allItems.begin(), m_allItems.end(),
            [](const QuickOpenItem& i){ return i.kind == QuickOpenItem::Kind::Entity; }),
        m_allItems.end());

    for (const auto& name : names) {
        QuickOpenItem item;
        item.kind  = QuickOpenItem::Kind::Entity;
        item.label = name;
        item.path  = "entity://" + name;
        m_allItems.push_back(std::move(item));
    }
    Rebuild();
}

void QuickOpen::AddRecentFile(const std::string& path) {
    QuickOpenItem item;
    item.kind  = QuickOpenItem::Kind::RecentFile;
    item.label = std::filesystem::path(path).filename().string();
    item.path  = path;
    item.score = 100.f;  // boost recent files
    m_allItems.insert(m_allItems.begin(), std::move(item));
    if (m_allItems.size() > 200) m_allItems.resize(200);
    Rebuild();
}

void QuickOpen::AddCommand(const std::string& label, const std::string& id) {
    QuickOpenItem item;
    item.kind  = QuickOpenItem::Kind::Command;
    item.label = label;
    item.path  = "cmd://" + id;
    m_allItems.push_back(std::move(item));
    Rebuild();
}

// ── Fuzzy scoring ──────────────────────────────────────────────────────────
float QuickOpen::FuzzyScore(const std::string& query, const std::string& candidate) {
    if (query.empty()) return 1.f;

    // Simple subsequence match with consecutive-run bonus
    int qi = 0, ci = 0;
    int consecutive = 0;
    float score = 0.f;
    std::string ql = query, cl = candidate;
    std::transform(ql.begin(), ql.end(), ql.begin(), ::tolower);
    std::transform(cl.begin(), cl.end(), cl.begin(), ::tolower);

    while (qi < (int)ql.size() && ci < (int)cl.size()) {
        if (ql[qi] == cl[ci]) {
            score += 1.f + consecutive * 0.5f;
            ++consecutive;
            ++qi;
        } else {
            consecutive = 0;
        }
        ++ci;
    }
    if (qi < (int)ql.size()) return 0.f;  // not all query chars matched
    // Penalise long candidates
    score -= (float)(cl.size() - ql.size()) * 0.01f;
    return score;
}

void QuickOpen::Rebuild() {
    m_results.clear();
    for (auto item : m_allItems) {
        item.score = FuzzyScore(m_query, item.label);
        if (item.score > 0.f || m_query.empty())
            m_results.push_back(item);
    }
    std::stable_sort(m_results.begin(), m_results.end(),
        [](const QuickOpenItem& a, const QuickOpenItem& b){ return a.score > b.score; });
    if (m_results.size() > 20) m_results.resize(20);
    m_selection = std::clamp(m_selection, 0,
                             m_results.empty() ? 0 : (int)m_results.size() - 1);
}

} // namespace Editor
