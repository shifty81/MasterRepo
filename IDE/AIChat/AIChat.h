#pragma once
#include <string>
#include <vector>
#include <functional>
#include <deque>
#include <cstdint>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Chat message
// ──────────────────────────────────────────────────────────────

enum class ChatRole { User, Assistant, System };

struct ChatMessage {
    ChatRole    role      = ChatRole::User;
    std::string content;
    std::string timestamp; // ISO-8601 string, filled on insertion
};

// ──────────────────────────────────────────────────────────────
// AI Chat Panel — connects to the AI agent system
// ──────────────────────────────────────────────────────────────

class AIChat {
public:
    // Callback invoked when the user sends a message.
    // Implementations should call AppendMessage() with the response.
    using SendFn = std::function<void(const std::string& userMessage, AIChat& chat)>;

    void SetSendCallback(SendFn fn);

    // Send a message from the user
    void SendMessage(const std::string& text);

    // Append a message (called by backend / send callback)
    void AppendMessage(ChatRole role, const std::string& content);

    // Clear the conversation history
    void ClearHistory();

    // Access the conversation
    const std::deque<ChatMessage>& Messages() const { return m_messages; }
    size_t MessageCount() const { return m_messages.size(); }

    // System prompt shown to the AI on first message
    void SetSystemPrompt(const std::string& prompt);
    const std::string& SystemPrompt() const { return m_systemPrompt; }

    // Pending indicator
    bool IsPending() const { return m_pending; }

    // Notification when a new message arrives
    using MessageCallback = std::function<void(const ChatMessage&)>;
    void SetMessageCallback(MessageCallback cb);

    // Maximum number of messages to keep in history
    void SetMaxHistory(size_t max) { m_maxHistory = max; }

    // Export history as markdown text
    std::string ExportMarkdown() const;

private:
    std::deque<ChatMessage> m_messages;
    std::string             m_systemPrompt;
    SendFn                  m_sendFn;
    MessageCallback         m_msgCb;
    bool                    m_pending   = false;
    size_t                  m_maxHistory = 500;

    static std::string CurrentTimestamp();
};

} // namespace IDE
