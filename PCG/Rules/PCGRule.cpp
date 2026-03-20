#include "PCG/Rules/PCGRule.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace PCG {

// ---- PCGSeed ---------------------------------------------------------------

uint64_t PCGSeed::Next() {
    // xorshift64
    value ^= value << 13;
    value ^= value >> 7;
    value ^= value << 17;
    return value;
}

float PCGSeed::NextFloat() {
    return static_cast<float>(Next() & 0xFFFFFFFFull) / 4294967296.0f;
}

int PCGSeed::NextInt(int max) {
    if (max <= 0) return 0;
    return static_cast<int>(Next() % static_cast<uint64_t>(max));
}

// ---- PCGRuleEngine ---------------------------------------------------------

uint32_t PCGRuleEngine::AddRule(PCGRule rule) {
    rule.id = m_nextID++;
    uint32_t id = rule.id;
    m_rules[id] = std::move(rule);
    return id;
}

void PCGRuleEngine::RemoveRule(uint32_t id) {
    m_rules.erase(id);
}

PCGRule* PCGRuleEngine::GetRule(uint32_t id) {
    auto it = m_rules.find(id);
    return (it != m_rules.end()) ? &it->second : nullptr;
}

std::vector<PCGRule> PCGRuleEngine::GetByType(RuleType type) const {
    std::vector<PCGRule> result;
    for (auto& [id, rule] : m_rules) {
        if (rule.type == type) result.push_back(rule);
    }
    return result;
}

bool PCGRuleEngine::Evaluate(PCGSeed& seed, const std::map<std::string, float>& context) const {
    // Collect enabled rules, sort by priority (descending)
    std::vector<const PCGRule*> active;
    active.reserve(m_rules.size());
    for (auto& [id, rule] : m_rules) {
        if (rule.enabled) active.push_back(&rule);
    }
    std::sort(active.begin(), active.end(),
        [](const PCGRule* a, const PCGRule* b) { return a->priority > b->priority; });

    for (const PCGRule* rule : active) {
        for (const PCGConstraint& c : rule->constraints) {
            auto it = context.find(c.name);
            float val = (it != context.end()) ? it->second : 0.0f;
            if (val < c.minValue || val > c.maxValue) return false;
        }
    }
    return true;
}

size_t PCGRuleEngine::Count() const {
    return m_rules.size();
}

void PCGRuleEngine::EnableAll() {
    for (auto& [id, rule] : m_rules) rule.enabled = true;
}

void PCGRuleEngine::DisableAll() {
    for (auto& [id, rule] : m_rules) rule.enabled = false;
}

bool PCGRuleEngine::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    m_rules.clear();
    m_nextID = 1;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // Simple CSV: name,type_int,priority,enabled
        std::istringstream ss(line);
        std::string token;
        PCGRule rule;
        if (std::getline(ss, token, ',')) rule.name = token;
        if (std::getline(ss, token, ',')) rule.type = static_cast<RuleType>(std::stoi(token));
        if (std::getline(ss, token, ',')) rule.priority = std::stoi(token);
        if (std::getline(ss, token, ',')) rule.enabled = (token == "1");
        AddRule(std::move(rule));
    }
    return true;
}

bool PCGRuleEngine::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "# PCGRuleEngine export\n";
    for (auto& [id, rule] : m_rules) {
        f << rule.name << ','
          << static_cast<int>(rule.type) << ','
          << rule.priority << ','
          << (rule.enabled ? 1 : 0) << '\n';
    }
    return true;
}

} // namespace PCG
