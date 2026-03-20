#pragma once
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <cstdint>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Console line
// ──────────────────────────────────────────────────────────────

enum class ConsoleMsgType { Normal, Info, Warning, Error, Command, Output };

struct ConsoleLine {
    ConsoleMsgType  type    = ConsoleMsgType::Normal;
    std::string     text;
    std::string     timestamp;
};

// ──────────────────────────────────────────────────────────────
// IDEConsole — integrated terminal / output panel
// ──────────────────────────────────────────────────────────────

class IDEConsole {
public:
    // Input callback — invoked when user enters a command
    using ExecuteFn = std::function<void(const std::string& cmd, IDEConsole& console)>;
    void SetExecuteCallback(ExecuteFn fn);

    // Submit a command from the input line
    void Execute(const std::string& command);

    // Print output
    void Print(const std::string& text, ConsoleMsgType type = ConsoleMsgType::Normal);
    void PrintInfo(const std::string& text)    { Print(text, ConsoleMsgType::Info); }
    void PrintWarning(const std::string& text) { Print(text, ConsoleMsgType::Warning); }
    void PrintError(const std::string& text)   { Print(text, ConsoleMsgType::Error); }

    // Output from a subprocess (line by line)
    void PrintOutput(const std::string& text)  { Print(text, ConsoleMsgType::Output); }

    // Access lines
    const std::deque<ConsoleLine>& Lines() const { return m_lines; }
    size_t LineCount() const { return m_lines.size(); }

    // Clear
    void Clear();

    // Command history (up/down arrow navigation)
    void HistoryPrevious();
    void HistoryNext();
    const std::string& CurrentInput() const { return m_input; }
    void SetInput(const std::string& text)  { m_input = text; }

    // Max lines buffer
    void SetMaxLines(size_t max) { m_maxLines = max; }

    // Notification callback
    using LineCallback = std::function<void(const ConsoleLine&)>;
    void SetLineCallback(LineCallback cb);

    // Export all output as plain text
    std::string Export() const;

private:
    std::deque<ConsoleLine>  m_lines;
    std::deque<std::string>  m_history;
    std::string              m_input;
    int32_t                  m_historyIdx = -1;
    size_t                   m_maxLines   = 2000;
    ExecuteFn                m_executeFn;
    LineCallback             m_lineCb;

    static std::string CurrentTimestamp();
    void addLine(ConsoleLine line);
};

} // namespace IDE
