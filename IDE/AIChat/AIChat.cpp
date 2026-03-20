#include "IDE/AIChat/AIChat.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace IDE {

void AIChat::SetSendCallback(SendFn fn) { m_sendFn = std::move(fn); }

void AIChat::SetMessageCallback(MessageCallback cb) { m_msgCb = std::move(cb); }

void AIChat::SetSystemPrompt(const std::string& prompt) { m_systemPrompt = prompt; }

void AIChat::ClearHistory() { m_messages.clear(); m_pending = false; }

std::string AIChat::CurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return os.str();
}

void AIChat::AppendMessage(ChatRole role, const std::string& content) {
    ChatMessage msg;
    msg.role      = role;
    msg.content   = content;
    msg.timestamp = CurrentTimestamp();
    m_messages.push_back(std::move(msg));
    if (m_messages.size() > m_maxHistory)
        m_messages.pop_front();
    if (role == ChatRole::Assistant) m_pending = false;
    if (m_msgCb) m_msgCb(m_messages.back());
}

void AIChat::SendMessage(const std::string& text) {
    if (text.empty()) return;
    AppendMessage(ChatRole::User, text);
    m_pending = true;
    if (m_sendFn) {
        m_sendFn(text, *this);
    } else {
        // No backend connected — echo a placeholder
        AppendMessage(ChatRole::Assistant, "[No AI backend connected]");
    }
}

std::string AIChat::ExportMarkdown() const {
    std::ostringstream os;
    os << "# AI Chat Export\n\n";
    for (const auto& msg : m_messages) {
        const char* label =
            (msg.role == ChatRole::User)      ? "**You**" :
            (msg.role == ChatRole::Assistant) ? "**AI**"  : "*System*";
        os << label << " — " << msg.timestamp << "\n\n"
           << msg.content << "\n\n---\n\n";
    }
    return os.str();
}

} // namespace IDE
