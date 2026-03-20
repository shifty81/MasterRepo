#include "IDE/Console/IDEConsole.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace IDE {

std::string IDEConsole::CurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%H:%M:%S");
    return os.str();
}

void IDEConsole::SetExecuteCallback(ExecuteFn fn) { m_executeFn = std::move(fn); }
void IDEConsole::SetLineCallback(LineCallback cb) { m_lineCb = std::move(cb); }

void IDEConsole::addLine(ConsoleLine line) {
    m_lines.push_back(std::move(line));
    if (m_lines.size() > m_maxLines) m_lines.pop_front();
    if (m_lineCb) m_lineCb(m_lines.back());
}

void IDEConsole::Print(const std::string& text, ConsoleMsgType type) {
    ConsoleLine ln;
    ln.type      = type;
    ln.text      = text;
    ln.timestamp = CurrentTimestamp();
    addLine(std::move(ln));
}

void IDEConsole::Execute(const std::string& command) {
    if (command.empty()) return;

    // Echo the command
    ConsoleLine ln;
    ln.type      = ConsoleMsgType::Command;
    ln.text      = "> " + command;
    ln.timestamp = CurrentTimestamp();
    addLine(std::move(ln));

    // Add to history
    if (m_history.empty() || m_history.back() != command)
        m_history.push_back(command);
    if (m_history.size() > 200) m_history.pop_front();
    m_historyIdx = -1;
    m_input.clear();

    if (m_executeFn) {
        m_executeFn(command, *this);
    } else {
        PrintError("No command handler registered");
    }
}

void IDEConsole::Clear() { m_lines.clear(); }

void IDEConsole::HistoryPrevious() {
    if (m_history.empty()) return;
    if (m_historyIdx < 0) m_historyIdx = (int32_t)m_history.size() - 1;
    else if (m_historyIdx > 0) --m_historyIdx;
    m_input = m_history[(size_t)m_historyIdx];
}

void IDEConsole::HistoryNext() {
    if (m_historyIdx < 0) return;
    ++m_historyIdx;
    if (m_historyIdx >= (int32_t)m_history.size()) {
        m_historyIdx = -1;
        m_input.clear();
    } else {
        m_input = m_history[(size_t)m_historyIdx];
    }
}

std::string IDEConsole::Export() const {
    std::ostringstream os;
    for (const auto& ln : m_lines)
        os << "[" << ln.timestamp << "] " << ln.text << "\n";
    return os.str();
}

} // namespace IDE
