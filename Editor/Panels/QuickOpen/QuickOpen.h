#pragma once
/**
 * @file QuickOpen.h
 * @brief PL-06 — Ctrl+P quick-open overlay.
 *
 * Shows a floating search box with fuzzy matching of assets, entities,
 * and recently opened files.  Results are drawn by EditorRenderer
 * when IsVisible() returns true.
 *
 * Integration:
 *   - Call Show() on Ctrl+P keypress.
 *   - Append characters via TypeChar(), clear with Clear(), submit with Confirm().
 *   - EditorRenderer draws the overlay on top of all panels.
 *   - On confirm, the selected item is forwarded via the ConfirmCallback.
 */
#include <functional>
#include <string>
#include <vector>

namespace Editor {

struct QuickOpenItem {
    enum class Kind { Asset, Entity, RecentFile, Command };
    Kind        kind = Kind::Asset;
    std::string label;    ///< Display text
    std::string path;     ///< Full path or identifier
    float       score = 0.f; ///< Higher = better match
};

class QuickOpen {
public:
    using ConfirmFn = std::function<void(const QuickOpenItem&)>;

    // ── Lifecycle ─────────────────────────────────────────────────────
    bool IsVisible() const  { return m_visible; }
    void Show();
    void Hide();
    void Toggle();

    // ── Input handling ────────────────────────────────────────────────
    void TypeChar(char c);
    void Backspace();
    void Clear();
    void MoveSelectionDown();
    void MoveSelectionUp();
    void Confirm();
    void Cancel();

    // ── Data source ───────────────────────────────────────────────────
    void SetAssetsRoot(const std::string& path);
    void ScanAssets();
    void SetEntities(const std::vector<std::string>& entityNames);
    void AddRecentFile(const std::string& path);
    void AddCommand(const std::string& label, const std::string& id);

    // ── Query ─────────────────────────────────────────────────────────
    const std::string&                 Query()     const { return m_query; }
    const std::vector<QuickOpenItem>&  Results()   const { return m_results; }
    int                                Selection() const { return m_selection; }

    // ── Callback ──────────────────────────────────────────────────────
    void SetConfirmCallback(ConfirmFn fn) { m_confirmFn = std::move(fn); }

private:
    void Rebuild();  ///< Filter+score m_allItems → m_results
    static float FuzzyScore(const std::string& query, const std::string& candidate);

    bool              m_visible   = false;
    std::string       m_query;
    int               m_selection = 0;
    std::string       m_assetsRoot;
    std::vector<QuickOpenItem> m_allItems;
    std::vector<QuickOpenItem> m_results;
    ConfirmFn         m_confirmFn;
};

} // namespace Editor
