#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace IDE {

// ── Data types ────────────────────────────────────────────────────────────────

struct BreadcrumbSegment {
    std::string label;    // display name of this path segment
    std::string path;     // full path up to and including this segment
    bool        isLeaf{false};
};

struct BreadcrumbBarStats {
    uint64_t navigations{0};   // times the user clicked a breadcrumb
    uint64_t pathChanges{0};   // times SetPath was called
};

// ── BreadcrumbBar ─────────────────────────────────────────────────────────────

class BreadcrumbBar {
public:
    BreadcrumbBar();
    ~BreadcrumbBar();

    // Set the current path (file or directory).
    // The path is split on '/' and '\\'.
    void SetPath(const std::string& path);

    // Current full path.
    std::string GetPath() const;

    // Parsed segments (root → leaf).
    std::vector<BreadcrumbSegment> Segments() const;

    // Navigate to the segment at the given index (0 = root).
    // Fires OnNavigate if the path changes.
    bool NavigateTo(size_t segmentIndex);

    // Navigate up one level.
    bool NavigateUp();

    // Navigate forward (after NavigateUp).
    bool NavigateForward();

    // Whether forward navigation is available.
    bool CanGoForward() const;

    // Whether up/back navigation is available.
    bool CanGoUp() const;

    // Separator character used between segments in display strings.
    void SetSeparator(const std::string& sep);
    std::string GetSeparator() const;

    // Rendered label — segments joined by separator.
    std::string RenderedPath() const;

    // Callback — fired when the active path changes via navigation.
    void OnNavigate(std::function<void(const std::string& path)> callback);

    // Stats.
    BreadcrumbBarStats Stats() const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace IDE
