#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace AI {

// ──────────────────────────────────────────────────────────────
// Help topic
// ──────────────────────────────────────────────────────────────

struct HelpTopic {
    std::string              id;
    std::string              title;
    std::string              summary;    // one-liner shown in listings
    std::string              body;       // full help text / markdown
    std::vector<std::string> tags;       // searchable tags
    std::string              category;   // e.g. "editor", "pcg", "ai", "scripting"
    std::vector<std::string> seeAlso;    // related topic IDs
};

// ──────────────────────────────────────────────────────────────
// ContextHelp — context-aware help system
// Maps the current editor/code context to relevant help topics
// ──────────────────────────────────────────────────────────────

class ContextHelp {
public:
    // Topic management
    void RegisterTopic(const HelpTopic& topic);
    void RegisterTopics(const std::vector<HelpTopic>& topics);
    bool LoadFromFile(const std::string& jsonPath);
    void ClearTopics();

    // Lookup
    const HelpTopic*           GetTopic(const std::string& id) const;
    std::vector<const HelpTopic*> Search(const std::string& query) const;
    std::vector<const HelpTopic*> GetByCategory(const std::string& category) const;
    std::vector<const HelpTopic*> GetByTag(const std::string& tag) const;

    // Context — set the active editor context (symbol, file, selection, etc.)
    void SetContextValue(const std::string& key, const std::string& value);
    void ClearContext();

    // Suggest help topics relevant to the current context
    std::vector<const HelpTopic*> SuggestForContext() const;

    // "What is this?" — single best match for a symbol/keyword
    std::optional<HelpTopic> ExplainSymbol(const std::string& symbol) const;

    // List all categories
    std::vector<std::string> Categories() const;

    // Callback: invoked when help should be shown to the user
    using ShowCallback = std::function<void(const HelpTopic&)>;
    void OnShowTopic(ShowCallback cb);

    // Show a topic (fires callback)
    void Show(const std::string& id);

    // Global singleton
    static ContextHelp& Instance();

private:
    float ScoreTopic(const HelpTopic& topic,
                     const std::unordered_map<std::string,std::string>& ctx) const;

    std::unordered_map<std::string, HelpTopic>     m_topics;
    std::unordered_map<std::string, std::string>   m_context;
    ShowCallback                                   m_showCb;
};

} // namespace AI
