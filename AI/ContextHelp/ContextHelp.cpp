#include "AI/ContextHelp/ContextHelp.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>

namespace AI {

// ── topic management ───────────────────────────────────────────

void ContextHelp::RegisterTopic(const HelpTopic& topic) {
    m_topics[topic.id] = topic;
}

void ContextHelp::RegisterTopics(const std::vector<HelpTopic>& topics) {
    for (const auto& t : topics) RegisterTopic(t);
}

bool ContextHelp::LoadFromFile(const std::string& jsonPath) {
    // Minimal plain-text format:
    // [id]
    // title=...
    // category=...
    // summary=...
    // tags=tag1,tag2
    // body=...
    // (blank line terminates entry)
    std::ifstream f(jsonPath);
    if (!f.is_open()) return false;

    HelpTopic cur;
    std::string line;
    bool inEntry = false;

    auto flush = [&]() {
        if (inEntry && !cur.id.empty()) RegisterTopic(cur);
        cur = {};
        inEntry = false;
    };

    while (std::getline(f, line)) {
        if (!line.empty() && line.front() == '[' && line.back() == ']') {
            flush();
            cur.id = line.substr(1, line.size()-2);
            inEntry = true;
        } else if (inEntry) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq+1);
            if      (key == "title")    cur.title    = val;
            else if (key == "category") cur.category = val;
            else if (key == "summary")  cur.summary  = val;
            else if (key == "body")     cur.body     = val;
            else if (key == "tags") {
                std::istringstream ss(val);
                std::string tag;
                while (std::getline(ss, tag, ','))
                    if (!tag.empty()) cur.tags.push_back(tag);
            } else if (key == "see") {
                std::istringstream ss(val);
                std::string ref;
                while (std::getline(ss, ref, ','))
                    if (!ref.empty()) cur.seeAlso.push_back(ref);
            }
        }
    }
    flush();
    return true;
}

void ContextHelp::ClearTopics() { m_topics.clear(); }

// ── lookup ─────────────────────────────────────────────────────

const HelpTopic* ContextHelp::GetTopic(const std::string& id) const {
    auto it = m_topics.find(id);
    return (it != m_topics.end()) ? &it->second : nullptr;
}

std::vector<const HelpTopic*> ContextHelp::Search(const std::string& query) const {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    std::vector<const HelpTopic*> results;
    for (const auto& [id, topic] : m_topics) {
        auto contains = [&](const std::string& s) {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            return lower.find(q) != std::string::npos;
        };
        if (contains(topic.title) || contains(topic.summary) || contains(topic.body) ||
            contains(topic.id)) {
            results.push_back(&topic);
        }
    }
    return results;
}

std::vector<const HelpTopic*> ContextHelp::GetByCategory(const std::string& category) const {
    std::vector<const HelpTopic*> results;
    for (const auto& [_, t] : m_topics)
        if (t.category == category) results.push_back(&t);
    return results;
}

std::vector<const HelpTopic*> ContextHelp::GetByTag(const std::string& tag) const {
    std::vector<const HelpTopic*> results;
    for (const auto& [_, t] : m_topics)
        for (const auto& tg : t.tags)
            if (tg == tag) { results.push_back(&t); break; }
    return results;
}

// ── context-aware suggestions ──────────────────────────────────

void ContextHelp::SetContextValue(const std::string& key, const std::string& value) {
    m_context[key] = value;
}
void ContextHelp::ClearContext() { m_context.clear(); }

float ContextHelp::ScoreTopic(const HelpTopic& topic,
                               const std::unordered_map<std::string,std::string>& ctx) const {
    float score = 0.0f;
    for (const auto& [k, v] : ctx) {
        std::string lower = v;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (topic.category == k || topic.category == lower) score += 2.0f;
        for (const auto& tag : topic.tags)
            if (tag == lower || tag == k) score += 1.5f;
        if (topic.id.find(lower) != std::string::npos)    score += 1.0f;
        if (topic.title.find(lower) != std::string::npos) score += 0.5f;
    }
    return score;
}

std::vector<const HelpTopic*> ContextHelp::SuggestForContext() const {
    std::vector<std::pair<float, const HelpTopic*>> scored;
    for (const auto& [_, topic] : m_topics) {
        float s = ScoreTopic(topic, m_context);
        if (s > 0.0f) scored.push_back({s, &topic});
    }
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });
    std::vector<const HelpTopic*> results;
    for (const auto& [_, t] : scored) results.push_back(t);
    return results;
}

std::optional<HelpTopic> ContextHelp::ExplainSymbol(const std::string& symbol) const {
    auto results = Search(symbol);
    if (results.empty()) return std::nullopt;
    return *results.front();
}

// ── display ────────────────────────────────────────────────────

std::vector<std::string> ContextHelp::Categories() const {
    std::vector<std::string> cats;
    for (const auto& [_, t] : m_topics) {
        if (std::find(cats.begin(), cats.end(), t.category) == cats.end())
            cats.push_back(t.category);
    }
    return cats;
}

void ContextHelp::OnShowTopic(ShowCallback cb) { m_showCb = std::move(cb); }

void ContextHelp::Show(const std::string& id) {
    auto it = m_topics.find(id);
    if (it != m_topics.end() && m_showCb) m_showCb(it->second);
}

// ── singleton ─────────────────────────────────────────────────

ContextHelp& ContextHelp::Instance() {
    static ContextHelp instance;
    return instance;
}

} // namespace AI
