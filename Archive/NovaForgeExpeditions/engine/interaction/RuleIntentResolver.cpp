#include "RuleIntentResolver.h"

namespace atlas::interaction {

void RuleIntentResolver::AddRule(const IntentRule& rule)
{
    rules.push_back(rule);
}

std::optional<Intent> RuleIntentResolver::Resolve(const Utterance& u)
{
    for (const auto& rule : rules) {
        if (std::regex_search(u.text, rule.pattern)) {
            return Intent{ rule.intentName, 1.0f };
        }
    }
    return std::nullopt;
}

} // namespace atlas::interaction
