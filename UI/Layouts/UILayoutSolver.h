#pragma once
/**
 * @file UILayoutSolver.h
 * @brief Constraint-based UI layout solver (horizontal or vertical pass).
 *
 * Adapted from Nova-Forge-Expeditions (atlas::ui → UI namespace).
 *
 * UILayoutSolver distributes available space among a list of widgets
 * according to min/preferred/max constraints and flex weights:
 *
 *   UIConstraint: minWidth/Height, preferredWidth/Height, maxWidth/Height, weight.
 *   UILayoutRect: x, y, w, h (resolved pixel bounds).
 *   LayoutDirection: Horizontal, Vertical.
 *
 *   UILayoutSolver:
 *     - AddEntry(widgetId, constraint) / Clear()
 *     - Solve(bounds, direction): allocate space to all entries
 *     - GetResolved(widgetId): resolved UILayoutRect
 *     - Entries() / EntryCount()
 *     - LayoutHash(): deterministic hash of all resolved rects
 */

#include <cstdint>
#include <string>
#include <vector>

namespace UI {

// ── Constraint ────────────────────────────────────────────────────────────
struct UIConstraint {
    int32_t minWidth       = 0;
    int32_t minHeight      = 0;
    int32_t preferredWidth = 100;
    int32_t preferredHeight = 100;
    int32_t maxWidth       = INT32_MAX;
    int32_t maxHeight      = INT32_MAX;
    float   weight         = 1.0f;
};

// ── Resolved rect ─────────────────────────────────────────────────────────
struct UILayoutRect {
    int32_t x{0}, y{0}, w{0}, h{0};
};

// ── Layout direction ──────────────────────────────────────────────────────
enum class LayoutDirection : uint8_t { Horizontal, Vertical };

// ── Entry ─────────────────────────────────────────────────────────────────
struct LayoutEntry {
    uint32_t       widgetId{0};
    UIConstraint   constraint;
    UILayoutRect   resolved;
};

// ── Solver ────────────────────────────────────────────────────────────────
class UILayoutSolver {
public:
    void Clear();
    void AddEntry(uint32_t widgetId, const UIConstraint& constraint);

    void Solve(const UILayoutRect& bounds, LayoutDirection direction);

    const UILayoutRect*        GetResolved(uint32_t widgetId) const;
    const std::vector<LayoutEntry>& Entries()    const;
    size_t                         EntryCount()  const;

    /// Deterministic hash of all resolved rects.
    uint64_t LayoutHash() const;

private:
    std::vector<LayoutEntry> m_entries;
};

} // namespace UI
