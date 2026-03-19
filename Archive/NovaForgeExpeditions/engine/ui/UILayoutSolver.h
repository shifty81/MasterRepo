#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace atlas::ui {

struct UIConstraint {
    int32_t minWidth = 0;
    int32_t minHeight = 0;
    int32_t preferredWidth = 100;
    int32_t preferredHeight = 100;
    int32_t maxWidth = INT32_MAX;
    int32_t maxHeight = INT32_MAX;
    float weight = 1.0f;
};

struct UILayoutRect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t w = 0;
    int32_t h = 0;
};

enum class LayoutDirection : uint8_t {
    Horizontal,
    Vertical
};

struct LayoutEntry {
    uint32_t widgetId = 0;
    UIConstraint constraint;
    UILayoutRect resolved;
};

class UILayoutSolver {
public:
    void Clear();

    void AddEntry(uint32_t widgetId, const UIConstraint& constraint);

    void Solve(const UILayoutRect& bounds, LayoutDirection direction);

    const UILayoutRect* GetResolved(uint32_t widgetId) const;

    const std::vector<LayoutEntry>& Entries() const;

    size_t EntryCount() const;

    /// Compute a deterministic hash of all resolved layout rects.
    /// Identical inputs on any platform must produce the same hash.
    uint64_t LayoutHash() const;

private:
    std::vector<LayoutEntry> m_entries;
};

} // namespace atlas::ui
