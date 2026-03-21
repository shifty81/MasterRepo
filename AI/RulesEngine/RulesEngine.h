#pragma once
// RulesEngine — AI constraint / rules checking for PCG, code, and content
//
// A lightweight forward-chaining rules engine that evaluates named rules
// against arbitrary context objects (passed as JSON-like key-value maps).
//
// Rules can be:
//   • Structural  — "if module count < 3 then warn MinModules"
//   • Semantic    — "if ship_class == destroyer then require shield_gen"
//   • AI-driven   — delegate evaluation to an LLM (offline) for fuzzy checks
//
// Typical users: PCGPipeline, CodeAudit, Validator, TestGen.

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// Context — the world-state the rules evaluate against
// ─────────────────────────────────────────────────────────────────────────────

using ContextValue = std::variant<bool, int, float, std::string>;

class RulesContext {
public:
    void Set(const std::string& key, ContextValue value);
    void SetAll(const std::unordered_map<std::string,ContextValue>& kv);

    bool   HasKey(const std::string& key) const;
    bool   GetBool(const std::string& key, bool def = false) const;
    int    GetInt(const std::string& key, int def = 0) const;
    float  GetFloat(const std::string& key, float def = 0.f) const;
    std::string GetString(const std::string& key,
                          const std::string& def = "") const;

    const std::unordered_map<std::string,ContextValue>& All() const;
    void Clear();

private:
    std::unordered_map<std::string,ContextValue> m_data;
};

// ─────────────────────────────────────────────────────────────────────────────
// Rule definition
// ─────────────────────────────────────────────────────────────────────────────

enum class RuleSeverity { Info, Warning, Error, Critical };
enum class RuleAction    { Pass, Warn, Fail, Fix };

struct RuleViolation {
    std::string   ruleId;
    RuleSeverity  severity  = RuleSeverity::Warning;
    std::string   message;
    std::string   suggestion;
};

/// A rule is a named predicate plus metadata.
struct Rule {
    std::string  id;
    std::string  description;
    RuleSeverity severity  = RuleSeverity::Warning;
    std::vector<std::string> tags;   // for filtering

    /// Returns true if the rule is satisfied (no violation).
    std::function<bool(const RulesContext&)> condition;

    /// Optional: generate a violation message when condition is false.
    std::function<RuleViolation(const RulesContext&)> violationFn;
};

// ─────────────────────────────────────────────────────────────────────────────
// Rule evaluation result
// ─────────────────────────────────────────────────────────────────────────────

struct EvalResult {
    bool                       passed = true;
    std::vector<RuleViolation> violations;

    int CountBySeverity(RuleSeverity s) const;
    bool HasCritical() const;
    bool HasErrors()   const;
    std::string Summary() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// RulesEngine
// ─────────────────────────────────────────────────────────────────────────────

struct RulesEngineConfig {
    bool stopOnCritical = true;   // halt evaluation after first Critical
    bool runAllRules    = false;  // override stopOnCritical
    // Optional LLM hook for AI-driven rule evaluation
    std::function<bool(const std::string& ruleDesc,
                       const RulesContext& ctx)> llmEvalFn;
};

class RulesEngine {
public:
    explicit RulesEngine(RulesEngineConfig cfg = {});

    // ── Rule management ──────────────────────────────────────────────────────

    void AddRule(Rule rule);
    void RemoveRule(const std::string& id);
    void RemoveByTag(const std::string& tag);
    void ClearRules();
    size_t RuleCount() const;

    const Rule* GetRule(const std::string& id) const;
    std::vector<const Rule*> GetByTag(const std::string& tag) const;

    // ── Evaluation ───────────────────────────────────────────────────────────

    EvalResult Evaluate(const RulesContext& ctx) const;
    EvalResult EvaluateSubset(const RulesContext& ctx,
                               const std::vector<std::string>& ruleIds) const;
    EvalResult EvaluateByTag(const RulesContext& ctx,
                              const std::string& tag) const;

    // ── Built-in rule factories ──────────────────────────────────────────────

    /// "key >= min" rule
    static Rule MakeRangeRule(const std::string& id,
                               const std::string& key,
                               float min, float max,
                               RuleSeverity sev = RuleSeverity::Warning);

    /// "key must equal value" rule
    static Rule MakeEqualRule(const std::string& id,
                               const std::string& key,
                               ContextValue expected,
                               RuleSeverity sev = RuleSeverity::Error);

    /// "key must not be empty string" rule
    static Rule MakeNonEmptyRule(const std::string& id,
                                  const std::string& key,
                                  RuleSeverity sev = RuleSeverity::Warning);

    // ── Reporting ────────────────────────────────────────────────────────────

    std::string BuildMarkdownReport(const EvalResult& result) const;

private:
    RulesEngineConfig       m_cfg;
    std::vector<Rule>       m_rules;

    RuleViolation DefaultViolation(const Rule& rule,
                                    const RulesContext& ctx) const;
};

} // namespace AI
