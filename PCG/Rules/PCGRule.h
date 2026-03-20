#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace PCG {

struct PCGConstraint {
    std::string name;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float weight   = 1.0f;
};

struct PCGSeed {
    uint64_t value = 0;

    uint64_t Next();
    float    NextFloat();   // [0, 1]
    int      NextInt(int max);
};

enum class RuleType {
    Placement,
    Shape,
    Connection,
    Fill,
    Filter,
    Transform
};

struct PCGRule {
    uint32_t                    id       = 0;
    std::string                 name;
    RuleType                    type     = RuleType::Placement;
    std::vector<PCGConstraint>  constraints;
    int                         priority = 0;
    bool                        enabled  = true;
};

class PCGRuleEngine {
public:
    uint32_t AddRule(PCGRule rule);
    void     RemoveRule(uint32_t id);
    PCGRule* GetRule(uint32_t id);
    std::vector<PCGRule> GetByType(RuleType type) const;

    // Returns true when all enabled rules pass for the given seed + context.
    bool Evaluate(PCGSeed& seed, const std::map<std::string, float>& context) const;

    size_t Count() const;
    void   EnableAll();
    void   DisableAll();

    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;

private:
    uint32_t m_nextID = 1;
    std::unordered_map<uint32_t, PCGRule> m_rules;
};

} // namespace PCG
