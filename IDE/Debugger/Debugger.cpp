#include "IDE/Debugger/Debugger.h"
#include <algorithm>

namespace IDE {

void Debugger::changeState(DebugState s) {
    m_state = s;
    if (m_stateFn) m_stateFn(s);
}

bool Debugger::Launch(const std::string& /*executable*/,
                       const std::vector<std::string>& /*args*/) {
    // Real implementation would spawn the process and attach a DAP adapter.
    // Stub: transition to Running.
    m_callStack.clear(); m_locals.clear();
    changeState(DebugState::Running);
    return true;
}

bool Debugger::Attach(uint32_t /*processID*/) {
    changeState(DebugState::Running);
    return true;
}

void Debugger::Stop() {
    m_callStack.clear(); m_locals.clear();
    m_pausedFile.clear(); m_pausedLine = 0;
    changeState(DebugState::Stopped);
}

void Debugger::Continue() {
    if (m_state == DebugState::Paused) changeState(DebugState::Running);
}

void Debugger::Pause() {
    if (m_state == DebugState::Running) changeState(DebugState::Paused);
}

void Debugger::StepOver() {
    // Stub — real impl sends DAP "next" request
    changeState(DebugState::Running);
}

void Debugger::StepInto() { changeState(DebugState::Running); }
void Debugger::StepOut()  { changeState(DebugState::Running); }

// ──────────────────────────────────────────────────────────────
// Breakpoints
// ──────────────────────────────────────────────────────────────

uint32_t Debugger::AddBreakpoint(const std::string& filePath, uint32_t line,
                                  const std::string& condition) {
    BPInfo bp;
    bp.id        = m_nextBPID++;
    bp.filePath  = filePath;
    bp.line      = line;
    bp.enabled   = true;
    bp.verified  = true; // assume verified for now
    bp.condition = condition;
    m_breakpoints.push_back(std::move(bp));
    return m_breakpoints.back().id;
}

void Debugger::RemoveBreakpoint(uint32_t id) {
    m_breakpoints.erase(std::remove_if(m_breakpoints.begin(), m_breakpoints.end(),
        [id](const BPInfo& b){ return b.id == id; }), m_breakpoints.end());
}

void Debugger::EnableBreakpoint(uint32_t id, bool enabled) {
    for (auto& bp : m_breakpoints)
        if (bp.id == id) { bp.enabled = enabled; return; }
}

void Debugger::ClearBreakpoints() { m_breakpoints.clear(); }

// ──────────────────────────────────────────────────────────────
// Watches
// ──────────────────────────────────────────────────────────────

void Debugger::AddWatch(const std::string& expression) {
    DebugVariable w;
    w.name  = expression;
    w.value = IsPaused() ? Evaluate(expression) : "<not paused>";
    m_watches.push_back(std::move(w));
}

void Debugger::RemoveWatch(size_t index) {
    if (index < m_watches.size()) m_watches.erase(m_watches.begin() + (ptrdiff_t)index);
}

// ──────────────────────────────────────────────────────────────
// Evaluation
// ──────────────────────────────────────────────────────────────

void Debugger::SetEvalCallback(EvalFn fn)         { m_evalFn  = std::move(fn); }
void Debugger::SetBreakpointHitCallback(HitFn fn) { m_hitFn   = std::move(fn); }
void Debugger::SetStateChangeCallback(StateFn fn) { m_stateFn = std::move(fn); }
void Debugger::SetOutputCallback(OutputFn fn)     { m_outputFn = std::move(fn); }

std::string Debugger::Evaluate(const std::string& expression) const {
    if (m_evalFn) return m_evalFn(expression);
    return "<no eval backend>";
}

// ──────────────────────────────────────────────────────────────
// Simulation (for testing / integration)
// ──────────────────────────────────────────────────────────────

void Debugger::SimulateBreakpointHit(const std::string& filePath, uint32_t line,
                                      const std::vector<StackFrame>& stack,
                                      const std::vector<DebugVariable>& locals) {
    m_pausedFile = filePath;
    m_pausedLine = line;
    m_callStack  = stack;
    m_locals     = locals;

    // Refresh watches
    for (auto& w : m_watches)
        w.value = Evaluate(w.name);

    changeState(DebugState::Paused);

    // Fire callback for matching breakpoint
    if (m_hitFn) {
        for (const auto& bp : m_breakpoints)
            if (bp.filePath == filePath && bp.line == line && bp.enabled) {
                m_hitFn(bp, stack.empty() ? StackFrame{} : stack.front());
                break;
            }
    }
}

void Debugger::DeliverOutput(const std::string& text) {
    if (m_outputFn) m_outputFn(text);
}

} // namespace IDE
