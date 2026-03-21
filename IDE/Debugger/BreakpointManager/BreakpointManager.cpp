#include "IDE/Debugger/BreakpointManager/BreakpointManager.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace IDE {

const char* BreakpointKindName(BreakpointKind k) {
    switch (k) {
    case BreakpointKind::Line:        return "Line";
    case BreakpointKind::Conditional: return "Conditional";
    case BreakpointKind::Data:        return "Data";
    case BreakpointKind::Function:    return "Function";
    }
    return "Unknown";
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct BreakpointManager::Impl {
    std::vector<Breakpoint>       breakpoints;
    uint32_t                      nextId{1};
    std::vector<HitCb>            hitCbs;
    std::vector<ChangedCb>        changedCbs;

    void fireChanged() { for (auto& cb : changedCbs) cb(); }

    Breakpoint* find(uint32_t id) {
        for (auto& bp : breakpoints)
            if (bp.id == id) return &bp;
        return nullptr;
    }
    const Breakpoint* find(uint32_t id) const {
        for (const auto& bp : breakpoints)
            if (bp.id == id) return &bp;
        return nullptr;
    }
};

BreakpointManager::BreakpointManager() : m_impl(new Impl()) {}
BreakpointManager::~BreakpointManager() { delete m_impl; }

uint32_t BreakpointManager::Add(const std::string& filePath, uint32_t line,
                                 BreakpointKind kind) {
    // Check for duplicate
    for (const auto& bp : m_impl->breakpoints)
        if (bp.filePath == filePath && bp.line == line) return bp.id;

    Breakpoint bp;
    bp.id       = m_impl->nextId++;
    bp.kind     = kind;
    bp.filePath = filePath;
    bp.line     = line;
    m_impl->breakpoints.push_back(bp);
    m_impl->fireChanged();
    return bp.id;
}

uint32_t BreakpointManager::AddFunction(const std::string& functionName) {
    Breakpoint bp;
    bp.id           = m_impl->nextId++;
    bp.kind         = BreakpointKind::Function;
    bp.functionName = functionName;
    m_impl->breakpoints.push_back(bp);
    m_impl->fireChanged();
    return bp.id;
}

bool BreakpointManager::Remove(uint32_t id) {
    auto& bps = m_impl->breakpoints;
    auto it = std::find_if(bps.begin(), bps.end(),
                           [id](const Breakpoint& bp) { return bp.id == id; });
    if (it == bps.end()) return false;
    bps.erase(it);
    m_impl->fireChanged();
    return true;
}

void BreakpointManager::RemoveAll(const std::string& filePath) {
    auto& bps = m_impl->breakpoints;
    auto before = bps.size();
    bps.erase(std::remove_if(bps.begin(), bps.end(),
                              [&](const Breakpoint& bp) {
                                  return bp.filePath == filePath; }),
              bps.end());
    if (bps.size() != before) m_impl->fireChanged();
}

void BreakpointManager::Clear() {
    m_impl->breakpoints.clear();
    m_impl->fireChanged();
}

bool BreakpointManager::SetEnabled(uint32_t id, bool enabled) {
    auto* bp = m_impl->find(id);
    if (!bp) return false;
    bp->enabled = enabled;
    m_impl->fireChanged();
    return true;
}

bool BreakpointManager::SetCondition(uint32_t id, const std::string& cond) {
    auto* bp = m_impl->find(id);
    if (!bp) return false;
    bp->condition = cond;
    if (!cond.empty()) bp->kind = BreakpointKind::Conditional;
    m_impl->fireChanged();
    return true;
}

bool BreakpointManager::SetHitCountTarget(uint32_t id, uint32_t target) {
    auto* bp = m_impl->find(id);
    if (!bp) return false;
    bp->hitCountTarget = target;
    return true;
}

bool BreakpointManager::SetLabel(uint32_t id, const std::string& label) {
    auto* bp = m_impl->find(id);
    if (!bp) return false;
    bp->label = label;
    return true;
}

bool BreakpointManager::Hit(uint32_t id) {
    auto* bp = m_impl->find(id);
    if (!bp || !bp->enabled) return false;
    ++bp->hitCount;
    if (bp->hitCountTarget > 0 && (bp->hitCount % bp->hitCountTarget) != 0)
        return false;
    for (auto& cb : m_impl->hitCbs) cb(*bp);
    return true;
}

const Breakpoint* BreakpointManager::Find(uint32_t id) const {
    return m_impl->find(id);
}

std::vector<Breakpoint> BreakpointManager::All() const {
    return m_impl->breakpoints;
}

std::vector<Breakpoint> BreakpointManager::InFile(
        const std::string& filePath) const {
    std::vector<Breakpoint> out;
    for (const auto& bp : m_impl->breakpoints)
        if (bp.filePath == filePath) out.push_back(bp);
    return out;
}

std::optional<Breakpoint> BreakpointManager::AtLine(
        const std::string& filePath, uint32_t line) const {
    for (const auto& bp : m_impl->breakpoints)
        if (bp.filePath == filePath && bp.line == line) return bp;
    return {};
}

size_t BreakpointManager::Count() const { return m_impl->breakpoints.size(); }

size_t BreakpointManager::EnabledCount() const {
    size_t n = 0;
    for (const auto& bp : m_impl->breakpoints) if (bp.enabled) ++n;
    return n;
}

// ── JSON export / import (minimal hand-rolled) ────────────────────────────
std::string BreakpointManager::ExportJSON() const {
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < m_impl->breakpoints.size(); ++i) {
        const auto& bp = m_impl->breakpoints[i];
        oss << "  {\"id\":" << bp.id
            << ",\"kind\":\"" << BreakpointKindName(bp.kind) << "\""
            << ",\"file\":\"" << bp.filePath << "\""
            << ",\"line\":" << bp.line
            << ",\"enabled\":" << (bp.enabled ? "true" : "false")
            << ",\"condition\":\"" << bp.condition << "\""
            << ",\"hitCount\":" << bp.hitCount
            << ",\"label\":\"" << bp.label << "\""
            << "}";
        if (i + 1 < m_impl->breakpoints.size()) oss << ",";
        oss << "\n";
    }
    oss << "]\n";
    return oss.str();
}

bool BreakpointManager::ImportJSON(const std::string& /*json*/) {
    // Minimal stub: full JSON parsing would use a library.
    return true;
}

void BreakpointManager::OnBreakpointHit(HitCb cb) {
    m_impl->hitCbs.push_back(std::move(cb));
}
void BreakpointManager::OnBreakpointChanged(ChangedCb cb) {
    m_impl->changedCbs.push_back(std::move(cb));
}

} // namespace IDE
