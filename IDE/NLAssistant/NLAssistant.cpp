#include "IDE/NLAssistant/NLAssistant.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace IDE {

// ── configuration ──────────────────────────────────────────────

void NLAssistant::RegisterAction(const std::string& actionName, ActionHandler handler) {
    m_handlers[actionName].handler = std::move(handler);
}

void NLAssistant::AddAlias(const std::string& phrase, const std::string& actionName) {
    m_aliases[Lowercase(phrase)] = actionName;
}

void NLAssistant::SetResponseCallback(ResponseCallback cb) {
    m_responseCb = std::move(cb);
}

void NLAssistant::SetContext(const std::string& key, const std::string& value) {
    m_context[key] = value;
}

void NLAssistant::ClearContext() { m_context.clear(); }

// ── help ───────────────────────────────────────────────────────

void NLAssistant::RegisterHelp(const std::string& actionName, const std::string& helpText) {
    m_handlers[actionName].helpText = helpText;
}

std::string NLAssistant::GetHelp(const std::string& actionName) const {
    auto it = m_handlers.find(actionName);
    return (it != m_handlers.end()) ? it->second.helpText : "";
}

std::string NLAssistant::GetAllHelp() const {
    std::ostringstream oss;
    for (const auto& [name, entry] : m_handlers) {
        oss << name;
        if (!entry.helpText.empty()) oss << "  —  " << entry.helpText;
        oss << "\n";
    }
    return oss.str();
}

std::vector<std::string> NLAssistant::ListActions() const {
    std::vector<std::string> names;
    names.reserve(m_handlers.size());
    for (const auto& [name, _] : m_handlers) names.push_back(name);
    return names;
}

// ── processing ─────────────────────────────────────────────────

ParsedIntent NLAssistant::Submit(const std::string& text) {
    m_history.push_back(text);
    ParsedIntent intent = ParseText(text);
    Dispatch(intent);
    return intent;
}

void NLAssistant::SubmitAsync(const std::string& text) {
    // Synchronous for now — async scheduling deferred to TaskSystem integration
    Submit(text);
}

std::vector<std::string> NLAssistant::GetSuggestions(const std::string& partialText) const {
    std::string lower = Lowercase(partialText);
    std::vector<std::string> results;
    for (const auto& [name, _] : m_handlers)
        if (Lowercase(name).find(lower) != std::string::npos) results.push_back(name);
    for (const auto& [phrase, _] : m_aliases)
        if (phrase.find(lower) != std::string::npos) results.push_back(phrase);
    return results;
}

// ── private helpers ────────────────────────────────────────────

ParsedIntent NLAssistant::ParseText(const std::string& text) const {
    ParsedIntent intent;
    intent.rawText = text;
    std::string lower = Lowercase(text);

    // 1. Direct alias match
    auto aliasIt = m_aliases.find(lower);
    if (aliasIt != m_aliases.end()) {
        intent.action     = aliasIt->second;
        intent.confidence = 1.0f;
        return intent;
    }

    // 2. Partial alias / action match (first token)
    std::istringstream ss(lower);
    std::string first;
    ss >> first;

    // Check if first token matches an alias
    for (const auto& [phrase, action] : m_aliases) {
        if (phrase.rfind(first, 0) == 0 || lower.find(phrase) != std::string::npos) {
            intent.action     = action;
            intent.confidence = 0.8f;
            break;
        }
    }

    // 3. Fall back: first token as action name
    if (intent.action.empty()) {
        if (m_handlers.count(first)) {
            intent.action     = first;
            intent.confidence = 0.9f;
        }
    }

    // 4. Extract "key:value" params from remaining tokens
    std::string tok;
    while (ss >> tok) {
        auto colon = tok.find(':');
        if (colon != std::string::npos) {
            intent.params[tok.substr(0, colon)] = tok.substr(colon + 1);
        } else {
            intent.params["arg" + std::to_string(intent.params.size())] = tok;
        }
    }

    // 5. Inject context as params with lower priority
    for (const auto& [k, v] : m_context)
        intent.params.emplace("ctx_" + k, v);

    return intent;
}

void NLAssistant::Dispatch(const ParsedIntent& intent) {
    if (intent.action.empty()) {
        Respond("Unknown command: " + intent.rawText + ". Type 'help' for a list of commands.");
        return;
    }
    auto it = m_handlers.find(intent.action);
    if (it != m_handlers.end() && it->second.handler) {
        it->second.handler(intent);
    } else {
        Respond("No handler registered for action: " + intent.action);
    }
}

std::string NLAssistant::Respond(const std::string& text) {
    if (m_responseCb) m_responseCb(text);
    return text;
}

std::string NLAssistant::Lowercase(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return r;
}

} // namespace IDE
