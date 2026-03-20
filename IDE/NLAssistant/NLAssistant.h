#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

namespace IDE {

// ──────────────────────────────────────────────────────────────
// Parsed intent from a natural-language command
// ──────────────────────────────────────────────────────────────

struct ParsedIntent {
    std::string              action;      // e.g. "open_file", "build", "run_test"
    std::unordered_map<std::string, std::string> params; // named args
    float                    confidence  = 0.0f; // [0,1]
    std::string              rawText;
};

// ──────────────────────────────────────────────────────────────
// NLAssistant — natural language command interface for the editor
// ──────────────────────────────────────────────────────────────

class NLAssistant {
public:
    // Callback executed when an intent is successfully dispatched
    using ActionHandler = std::function<void(const ParsedIntent&)>;
    // Callback for plain text responses (explanations, help, etc.)
    using ResponseCallback = std::function<void(const std::string& text)>;

    // --- Configuration -----------------------------------------------

    // Register an action handler for a specific action name
    void RegisterAction(const std::string& actionName, ActionHandler handler);

    // Add an alias so multiple phrasings map to the same action
    void AddAlias(const std::string& phrase, const std::string& actionName);

    // Set callback for text responses
    void SetResponseCallback(ResponseCallback cb);

    // Provide context information (current file, selection, etc.)
    void SetContext(const std::string& key, const std::string& value);
    void ClearContext();

    // --- Processing --------------------------------------------------

    // Submit a text command from the user; returns parsed intent
    ParsedIntent Submit(const std::string& text);

    // Non-blocking: enqueue and process asynchronously
    void SubmitAsync(const std::string& text);

    // Get suggestions/completions for partial input
    std::vector<std::string> GetSuggestions(const std::string& partialText) const;

    // --- Help --------------------------------------------------------

    // Return help text for a registered action
    void RegisterHelp(const std::string& actionName, const std::string& helpText);
    std::string GetHelp(const std::string& actionName) const;
    std::string GetAllHelp() const;

    // List all registered action names
    std::vector<std::string> ListActions() const;

    // --- History -----------------------------------------------------

    const std::vector<std::string>& History() const { return m_history; }
    void ClearHistory() { m_history.clear(); }

private:
    ParsedIntent  ParseText(const std::string& text) const;
    void          Dispatch(const ParsedIntent& intent);
    std::string   Respond(const std::string& text);
    static std::string Lowercase(const std::string& s);

    struct HandlerEntry {
        ActionHandler handler;
        std::string   helpText;
    };

    std::unordered_map<std::string, HandlerEntry>  m_handlers;
    std::unordered_map<std::string, std::string>   m_aliases;    // phrase → actionName
    std::unordered_map<std::string, std::string>   m_context;
    ResponseCallback                               m_responseCb;
    std::vector<std::string>                       m_history;
};

} // namespace IDE
