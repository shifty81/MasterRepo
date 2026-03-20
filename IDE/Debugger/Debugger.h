#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <map>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Stack frame (for call stack display)
// ──────────────────────────────────────────────────────────────

struct StackFrame {
    uint32_t    index     = 0;
    std::string function;
    std::string filePath;
    uint32_t    line      = 0;
    std::string module;
};

// ──────────────────────────────────────────────────────────────
// Variable / watch
// ──────────────────────────────────────────────────────────────

struct DebugVariable {
    std::string name;
    std::string type;
    std::string value;
    bool        isExpandable = false;
    std::vector<DebugVariable> children;
};

// ──────────────────────────────────────────────────────────────
// Debug session state
// ──────────────────────────────────────────────────────────────

enum class DebugState { Idle, Running, Paused, Stopped };

// ──────────────────────────────────────────────────────────────
// Debugger — step-through debugging with breakpoints
// ──────────────────────────────────────────────────────────────

class Debugger {
public:
    // Session control
    bool Launch(const std::string& executable, const std::vector<std::string>& args = {});
    bool Attach(uint32_t processID);
    void Stop();

    DebugState GetState() const { return m_state; }
    bool IsRunning() const { return m_state == DebugState::Running; }
    bool IsPaused()  const { return m_state == DebugState::Paused; }

    // Execution control
    void Continue();
    void StepOver();
    void StepInto();
    void StepOut();
    void Pause();

    // Breakpoints
    uint32_t AddBreakpoint(const std::string& filePath, uint32_t line,
                           const std::string& condition = "");
    void     RemoveBreakpoint(uint32_t id);
    void     EnableBreakpoint(uint32_t id, bool enabled);
    void     ClearBreakpoints();

    struct BPInfo {
        uint32_t    id        = 0;
        std::string filePath;
        uint32_t    line      = 0;
        bool        enabled   = true;
        bool        verified  = false;
        std::string condition;
    };
    const std::vector<BPInfo>& Breakpoints() const { return m_breakpoints; }

    // Call stack (valid when Paused)
    const std::vector<StackFrame>& CallStack() const { return m_callStack; }

    // Variables (valid when Paused)
    const std::vector<DebugVariable>& Locals() const  { return m_locals; }
    const std::vector<DebugVariable>& Watches() const { return m_watches; }
    void AddWatch(const std::string& expression);
    void RemoveWatch(size_t index);

    // Expression evaluation (when paused)
    using EvalFn = std::function<std::string(const std::string& expr)>;
    void SetEvalCallback(EvalFn fn);
    std::string Evaluate(const std::string& expression) const;

    // Event callbacks
    using HitFn  = std::function<void(const BPInfo& bp, const StackFrame& frame)>;
    using StateFn = std::function<void(DebugState state)>;
    void SetBreakpointHitCallback(HitFn fn);
    void SetStateChangeCallback(StateFn fn);

    // Simulate hitting a breakpoint (used by integration layer)
    void SimulateBreakpointHit(const std::string& filePath, uint32_t line,
                                const std::vector<StackFrame>& stack,
                                const std::vector<DebugVariable>& locals);

    // Output from the debuggee (stdout/stderr)
    using OutputFn = std::function<void(const std::string& text)>;
    void SetOutputCallback(OutputFn fn);
    void DeliverOutput(const std::string& text);

    // Current pause location
    std::string PausedFile() const { return m_pausedFile; }
    uint32_t    PausedLine() const { return m_pausedLine; }

private:
    DebugState              m_state       = DebugState::Idle;
    std::vector<BPInfo>     m_breakpoints;
    std::vector<StackFrame> m_callStack;
    std::vector<DebugVariable> m_locals;
    std::vector<DebugVariable> m_watches;
    std::string             m_pausedFile;
    uint32_t                m_pausedLine  = 0;
    uint32_t                m_nextBPID    = 1;

    EvalFn    m_evalFn;
    HitFn     m_hitFn;
    StateFn   m_stateFn;
    OutputFn  m_outputFn;

    void changeState(DebugState s);
};

} // namespace IDE
