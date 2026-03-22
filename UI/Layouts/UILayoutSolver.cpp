#include "UI/Layouts/UILayoutSolver.h"
#include <algorithm>

namespace UI {

void UILayoutSolver::Clear()  { m_entries.clear(); }

void UILayoutSolver::AddEntry(uint32_t widgetId, const UIConstraint& c) {
    m_entries.push_back({widgetId, c, {}});
}

void UILayoutSolver::Solve(const UILayoutRect& bounds, LayoutDirection dir) {
    if (m_entries.empty()) return;

    // Clamp helper
    auto clamp = [](int32_t v, int32_t lo, int32_t hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    };

    if (dir == LayoutDirection::Horizontal) {
        int32_t available = bounds.w;
        // First pass: allocate minimums
        int32_t totalMin = 0;
        float   totalWeight = 0;
        for (auto& e : m_entries) {
            totalMin    += e.constraint.minWidth;
            totalWeight += e.constraint.weight > 0 ? e.constraint.weight : 1.0f;
        }
        int32_t flex = available - totalMin;
        if (flex < 0) flex = 0;

        int32_t cursor = bounds.x;
        for (auto& e : m_entries) {
            float share = (e.constraint.weight > 0 ? e.constraint.weight : 1.0f) / totalWeight;
            int32_t extra  = static_cast<int32_t>(flex * share);
            int32_t w = clamp(e.constraint.minWidth + extra,
                              e.constraint.minWidth,
                              e.constraint.maxWidth);
            e.resolved.x = cursor;
            e.resolved.y = bounds.y;
            e.resolved.w = w;
            e.resolved.h = clamp(e.constraint.preferredHeight,
                                 e.constraint.minHeight,
                                 std::min(e.constraint.maxHeight, bounds.h));
            cursor += w;
        }
    } else {
        // Vertical
        int32_t available = bounds.h;
        int32_t totalMin  = 0;
        float   totalWeight = 0;
        for (auto& e : m_entries) {
            totalMin    += e.constraint.minHeight;
            totalWeight += e.constraint.weight > 0 ? e.constraint.weight : 1.0f;
        }
        int32_t flex = available - totalMin;
        if (flex < 0) flex = 0;

        int32_t cursor = bounds.y;
        for (auto& e : m_entries) {
            float share = (e.constraint.weight > 0 ? e.constraint.weight : 1.0f) / totalWeight;
            int32_t extra  = static_cast<int32_t>(flex * share);
            int32_t h = clamp(e.constraint.minHeight + extra,
                              e.constraint.minHeight,
                              e.constraint.maxHeight);
            e.resolved.x = bounds.x;
            e.resolved.y = cursor;
            e.resolved.w = clamp(e.constraint.preferredWidth,
                                 e.constraint.minWidth,
                                 std::min(e.constraint.maxWidth, bounds.w));
            e.resolved.h = h;
            cursor += h;
        }
    }
}

const UILayoutRect* UILayoutSolver::GetResolved(uint32_t widgetId) const {
    for (auto& e : m_entries) if (e.widgetId == widgetId) return &e.resolved;
    return nullptr;
}

const std::vector<LayoutEntry>& UILayoutSolver::Entries()   const { return m_entries; }
size_t                          UILayoutSolver::EntryCount() const { return m_entries.size(); }

uint64_t UILayoutSolver::LayoutHash() const {
    // FNV-1a over all resolved rects
    uint64_t h = 14695981039346656037ull;
    auto mix = [&](uint64_t v) {
        h ^= v; h *= 1099511628211ull;
    };
    for (auto& e : m_entries) {
        mix(e.widgetId);
        mix(static_cast<uint64_t>(static_cast<uint32_t>(e.resolved.x)));
        mix(static_cast<uint64_t>(static_cast<uint32_t>(e.resolved.y)));
        mix(static_cast<uint64_t>(static_cast<uint32_t>(e.resolved.w)));
        mix(static_cast<uint64_t>(static_cast<uint32_t>(e.resolved.h)));
    }
    return h;
}

} // namespace UI
