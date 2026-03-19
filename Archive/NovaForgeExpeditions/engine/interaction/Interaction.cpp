#include "Interaction.h"
#include <algorithm>
#include <cctype>

namespace atlas::interaction {

static std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

void PatternIntentResolver::AddPattern(const IntentPattern& pattern) {
    m_patterns.push_back(pattern);
}

std::optional<Intent> PatternIntentResolver::Resolve(const Utterance& utterance) {
    std::string lowerText = ToLower(utterance.text);
    for (const auto& pattern : m_patterns) {
        for (const auto& phrase : pattern.phrases) {
            if (lowerText.find(ToLower(phrase)) != std::string::npos) {
                Intent intent;
                intent.name = pattern.intentName;
                intent.confidence = 1.0f;
                intent.source = IntentSource::Text;
                return intent;
            }
        }
    }
    return std::nullopt;
}

void IntentRegistry::Register(const std::string& intentName, Handler handler) {
    m_handlers[intentName] = std::move(handler);
}

bool IntentRegistry::Dispatch(const Intent& intent) const {
    auto it = m_handlers.find(intent.name);
    if (it != m_handlers.end()) {
        it->second(intent);
        return true;
    }
    return false;
}

bool IntentRegistry::HasHandler(const std::string& intentName) const {
    return m_handlers.find(intentName) != m_handlers.end();
}

size_t IntentRegistry::HandlerCount() const {
    return m_handlers.size();
}

} // namespace atlas::interaction
