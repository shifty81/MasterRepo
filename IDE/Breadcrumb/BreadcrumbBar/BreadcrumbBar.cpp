#include "IDE/Breadcrumb/BreadcrumbBar/BreadcrumbBar.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace IDE {

// ── helpers ───────────────────────────────────────────────────────────────────

static std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : path) {
        if (c == '/' || c == '\\') {
            if (!cur.empty()) { parts.push_back(cur); cur.clear(); }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) parts.push_back(cur);
    return parts;
}

static std::string BuildPath(const std::vector<std::string>& parts, size_t count,
                              bool isAbsolute, const std::string& sep) {
    std::string result;
    if (isAbsolute) result = sep;
    for (size_t i = 0; i < count && i < parts.size(); ++i) {
        if (i > 0) result += sep;
        result += parts[i];
    }
    return result;
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct BreadcrumbBar::Impl {
    std::string                             fullPath;
    std::vector<std::string>                parts;
    bool                                    isAbsolute{false};
    std::string                             separator{"/"};
    std::vector<std::string>                history;   // back-stack of full paths
    size_t                                  historyPos{0};
    std::function<void(const std::string&)> onNavigate;
    BreadcrumbBarStats                      stats{};
};

// ── BreadcrumbBar ─────────────────────────────────────────────────────────────

BreadcrumbBar::BreadcrumbBar() : m_impl(new Impl{}) {}
BreadcrumbBar::~BreadcrumbBar() { delete m_impl; }

void BreadcrumbBar::SetPath(const std::string& path) {
    if (path == m_impl->fullPath) return;
    m_impl->fullPath  = path;
    m_impl->isAbsolute = (!path.empty() && (path[0] == '/' || path[0] == '\\'));
    m_impl->parts     = SplitPath(path);

    // Push to history
    if (m_impl->historyPos < m_impl->history.size()) {
        m_impl->history.resize(m_impl->historyPos);
    }
    m_impl->history.push_back(path);
    m_impl->historyPos = m_impl->history.size();

    ++m_impl->stats.pathChanges;
}

std::string BreadcrumbBar::GetPath() const { return m_impl->fullPath; }

std::vector<BreadcrumbSegment> BreadcrumbBar::Segments() const {
    std::vector<BreadcrumbSegment> segs;
    segs.reserve(m_impl->parts.size());
    for (size_t i = 0; i < m_impl->parts.size(); ++i) {
        BreadcrumbSegment seg;
        seg.label  = m_impl->parts[i];
        seg.isLeaf = (i + 1 == m_impl->parts.size());
        // Build path up to this segment
        std::string p;
        if (m_impl->isAbsolute) p = m_impl->separator;
        for (size_t j = 0; j <= i; ++j) {
            if (j > 0) p += m_impl->separator;
            p += m_impl->parts[j];
        }
        seg.path = p;
        segs.push_back(seg);
    }
    return segs;
}

bool BreadcrumbBar::NavigateTo(size_t segmentIndex) {
    if (segmentIndex >= m_impl->parts.size()) return false;
    std::string newPath = BuildPath(m_impl->parts, segmentIndex + 1,
                                    m_impl->isAbsolute, m_impl->separator);
    if (newPath == m_impl->fullPath) return false;
    SetPath(newPath);
    ++m_impl->stats.navigations;
    if (m_impl->onNavigate) m_impl->onNavigate(newPath);
    return true;
}

bool BreadcrumbBar::NavigateUp() {
    if (m_impl->parts.empty()) return false;
    return NavigateTo(m_impl->parts.size() - 2);
}

bool BreadcrumbBar::NavigateForward() {
    if (!CanGoForward()) return false;
    const std::string& next = m_impl->history[m_impl->historyPos];
    m_impl->fullPath  = next;
    m_impl->isAbsolute = (!next.empty() && (next[0] == '/' || next[0] == '\\'));
    m_impl->parts     = SplitPath(next);
    ++m_impl->historyPos;
    ++m_impl->stats.navigations;
    if (m_impl->onNavigate) m_impl->onNavigate(next);
    return true;
}

bool BreadcrumbBar::CanGoForward() const {
    return m_impl->historyPos < m_impl->history.size();
}

bool BreadcrumbBar::CanGoUp() const {
    return m_impl->parts.size() > 1;
}

void BreadcrumbBar::SetSeparator(const std::string& sep) { m_impl->separator = sep; }
std::string BreadcrumbBar::GetSeparator() const          { return m_impl->separator; }

std::string BreadcrumbBar::RenderedPath() const {
    std::string result;
    if (m_impl->isAbsolute) result = m_impl->separator;
    for (size_t i = 0; i < m_impl->parts.size(); ++i) {
        if (i > 0) result += m_impl->separator;
        result += m_impl->parts[i];
    }
    return result;
}

void BreadcrumbBar::OnNavigate(std::function<void(const std::string&)> callback) {
    m_impl->onNavigate = std::move(callback);
}

BreadcrumbBarStats BreadcrumbBar::Stats() const { return m_impl->stats; }

} // namespace IDE
