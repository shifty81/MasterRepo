#pragma once
/**
 * @file BreakpointManager.h
 * @brief IDE breakpoint management: add/remove/enable, hit-count, conditions.
 *
 * BreakpointManager tracks the full set of source-level breakpoints for the
 * integrated debugger panel:
 *
 *   - Breakpoint: file path, line number, enabled flag, optional condition
 *     expression, hit count, and a label.
 *   - BreakpointKind: Line, Conditional, Data (watchpoint), Function.
 *   - Add(file, line): insert a new line breakpoint; returns stable id.
 *   - Remove(id) / RemoveAll(file): remove one or all in a file.
 *   - Enable(id, bool): toggle without removing.
 *   - SetCondition(id, expr): attach a condition string (evaluated externally).
 *   - Hit(id): record a hit event; increments counter and fires OnHit callback.
 *   - Export / Import: JSON round-trip for workspace persistence.
 *   - OnBreakpointHit: callback fired when Hit() is called.
 *   - OnBreakpointChanged: callback fired on any structural change.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace IDE {

// ── Breakpoint kind ────────────────────────────────────────────────────────
enum class BreakpointKind : uint8_t { Line, Conditional, Data, Function };
const char* BreakpointKindName(BreakpointKind k);

// ── Breakpoint ────────────────────────────────────────────────────────────
struct Breakpoint {
    uint32_t        id{0};
    BreakpointKind  kind{BreakpointKind::Line};
    std::string     filePath;
    uint32_t        line{0};
    std::string     functionName;   ///< Used for Function breakpoints
    std::string     condition;      ///< Empty = unconditional
    std::string     label;
    bool            enabled{true};
    uint32_t        hitCount{0};
    uint32_t        hitCountTarget{0}; ///< 0 = always break; n = break every n hits
};

// ── Manager ───────────────────────────────────────────────────────────────
class BreakpointManager {
public:
    BreakpointManager();
    ~BreakpointManager();

    BreakpointManager(const BreakpointManager&) = delete;
    BreakpointManager& operator=(const BreakpointManager&) = delete;

    // ── mutators ──────────────────────────────────────────────
    uint32_t Add(const std::string& filePath, uint32_t line,
                 BreakpointKind kind = BreakpointKind::Line);
    uint32_t AddFunction(const std::string& functionName);
    bool     Remove(uint32_t id);
    void     RemoveAll(const std::string& filePath);
    void     Clear();

    // ── properties ────────────────────────────────────────────
    bool SetEnabled(uint32_t id, bool enabled);
    bool SetCondition(uint32_t id, const std::string& condition);
    bool SetHitCountTarget(uint32_t id, uint32_t target);
    bool SetLabel(uint32_t id, const std::string& label);

    // ── hit reporting ─────────────────────────────────────────
    /// Called by the debugger when execution stops at a breakpoint.
    /// Returns false if the breakpoint's condition / hit-count target
    /// means the program should continue without pausing.
    bool Hit(uint32_t id);

    // ── queries ───────────────────────────────────────────────
    const Breakpoint*               Find(uint32_t id) const;
    std::vector<Breakpoint>         All() const;
    std::vector<Breakpoint>         InFile(const std::string& filePath) const;
    std::optional<Breakpoint>       AtLine(const std::string& filePath,
                                           uint32_t line) const;
    size_t                          Count() const;
    size_t                          EnabledCount() const;

    // ── persistence ───────────────────────────────────────────
    std::string ExportJSON() const;
    bool        ImportJSON(const std::string& json);

    // ── callbacks ─────────────────────────────────────────────
    using HitCb     = std::function<void(const Breakpoint&)>;
    using ChangedCb = std::function<void()>;
    void OnBreakpointHit(HitCb cb);
    void OnBreakpointChanged(ChangedCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
