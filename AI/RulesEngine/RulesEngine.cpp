#include "AI/RulesEngine/RulesEngine.h"
#include <algorithm>
#include <sstream>

namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// RulesContext
// ─────────────────────────────────────────────────────────────────────────────

void RulesContext::Set(const std::string& key, ContextValue value)
{
    m_data[key] = std::move(value);
}

void RulesContext::SetAll(const std::unordered_map<std::string,ContextValue>& kv)
{
    for (const auto& [k,v] : kv) m_data[k] = v;
}

bool RulesContext::HasKey(const std::string& key) const
{
    return m_data.count(key) > 0;
}

bool RulesContext::GetBool(const std::string& key, bool def) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    if (auto* v = std::get_if<bool>(&it->second)) return *v;
    if (auto* v = std::get_if<int>(&it->second))  return *v != 0;
    return def;
}

int RulesContext::GetInt(const std::string& key, int def) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    if (auto* v = std::get_if<int>(&it->second))   return *v;
    if (auto* v = std::get_if<float>(&it->second)) return (int)*v;
    if (auto* v = std::get_if<bool>(&it->second))  return *v ? 1 : 0;
    return def;
}

float RulesContext::GetFloat(const std::string& key, float def) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    if (auto* v = std::get_if<float>(&it->second)) return *v;
    if (auto* v = std::get_if<int>(&it->second))   return (float)*v;
    if (auto* v = std::get_if<bool>(&it->second))  return *v ? 1.f : 0.f;
    return def;
}

std::string RulesContext::GetString(const std::string& key,
                                     const std::string& def) const
{
    auto it = m_data.find(key);
    if (it == m_data.end()) return def;
    if (auto* v = std::get_if<std::string>(&it->second)) return *v;
    return def;
}

const std::unordered_map<std::string,ContextValue>& RulesContext::All() const
{
    return m_data;
}

void RulesContext::Clear() { m_data.clear(); }

// ─────────────────────────────────────────────────────────────────────────────
// EvalResult helpers
// ─────────────────────────────────────────────────────────────────────────────

int EvalResult::CountBySeverity(RuleSeverity s) const
{
    int n = 0;
    for (const auto& v : violations)
        if (v.severity == s) ++n;
    return n;
}

bool EvalResult::HasCritical() const
{
    return CountBySeverity(RuleSeverity::Critical) > 0;
}

bool EvalResult::HasErrors() const
{
    return CountBySeverity(RuleSeverity::Error) > 0;
}

std::string EvalResult::Summary() const
{
    if (passed) return "All rules passed.";
    std::ostringstream oss;
    oss << violations.size() << " violation(s): ";
    int crit  = CountBySeverity(RuleSeverity::Critical);
    int err   = CountBySeverity(RuleSeverity::Error);
    int warn  = CountBySeverity(RuleSeverity::Warning);
    if (crit) oss << crit << " critical, ";
    if (err)  oss << err  << " error(s), ";
    if (warn) oss << warn << " warning(s)";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// RulesEngine
// ─────────────────────────────────────────────────────────────────────────────

RulesEngine::RulesEngine(RulesEngineConfig cfg)
    : m_cfg(std::move(cfg))
{}

void RulesEngine::AddRule(Rule rule)
{
    auto it = std::find_if(m_rules.begin(), m_rules.end(),
                           [&](const Rule& r){ return r.id == rule.id; });
    if (it != m_rules.end()) *it = std::move(rule);
    else                     m_rules.push_back(std::move(rule));
}

void RulesEngine::RemoveRule(const std::string& id)
{
    m_rules.erase(
        std::remove_if(m_rules.begin(), m_rules.end(),
                       [&](const Rule& r){ return r.id == id; }),
        m_rules.end());
}

void RulesEngine::RemoveByTag(const std::string& tag)
{
    m_rules.erase(
        std::remove_if(m_rules.begin(), m_rules.end(),
                       [&](const Rule& r){
                           return std::find(r.tags.begin(), r.tags.end(), tag)
                                  != r.tags.end();
                       }),
        m_rules.end());
}

void   RulesEngine::ClearRules()       { m_rules.clear(); }
size_t RulesEngine::RuleCount() const  { return m_rules.size(); }

const Rule* RulesEngine::GetRule(const std::string& id) const
{
    for (const auto& r : m_rules)
        if (r.id == id) return &r;
    return nullptr;
}

std::vector<const Rule*> RulesEngine::GetByTag(const std::string& tag) const
{
    std::vector<const Rule*> out;
    for (const auto& r : m_rules)
        if (std::find(r.tags.begin(), r.tags.end(), tag) != r.tags.end())
            out.push_back(&r);
    return out;
}

// ── Evaluation ────────────────────────────────────────────────────────────────

RuleViolation RulesEngine::DefaultViolation(const Rule& rule,
                                              const RulesContext&) const
{
    RuleViolation v;
    v.ruleId   = rule.id;
    v.severity = rule.severity;
    v.message  = "Rule violated: " + rule.description;
    return v;
}

EvalResult RulesEngine::Evaluate(const RulesContext& ctx) const
{
    EvalResult result;
    for (const auto& rule : m_rules) {
        if (!rule.condition) continue;

        bool ok = false;
        if (m_cfg.llmEvalFn)
            ok = m_cfg.llmEvalFn(rule.description, ctx);
        else
            ok = rule.condition(ctx);

        if (!ok) {
            RuleViolation v = rule.violationFn
                              ? rule.violationFn(ctx)
                              : DefaultViolation(rule, ctx);
            result.violations.push_back(v);
            if (v.severity == RuleSeverity::Error ||
                v.severity == RuleSeverity::Critical)
                result.passed = false;

            if (m_cfg.stopOnCritical &&
                v.severity == RuleSeverity::Critical &&
                !m_cfg.runAllRules)
                break;
        }
    }
    return result;
}

EvalResult RulesEngine::EvaluateSubset(const RulesContext& ctx,
                                        const std::vector<std::string>& ids) const
{
    EvalResult result;
    for (const auto& id : ids) {
        const Rule* r = GetRule(id);
        if (!r || !r->condition) continue;
        bool ok = r->condition(ctx);
        if (!ok) {
            RuleViolation v = r->violationFn
                              ? r->violationFn(ctx)
                              : DefaultViolation(*r, ctx);
            result.violations.push_back(v);
            if (v.severity >= RuleSeverity::Error) result.passed = false;
        }
    }
    return result;
}

EvalResult RulesEngine::EvaluateByTag(const RulesContext& ctx,
                                       const std::string& tag) const
{
    auto taggedRules = GetByTag(tag);
    std::vector<std::string> ids;
    for (const auto* r : taggedRules) ids.push_back(r->id);
    return EvaluateSubset(ctx, ids);
}

// ── Built-in rule factories ───────────────────────────────────────────────────

/* static */
Rule RulesEngine::MakeRangeRule(const std::string& id,
                                 const std::string& key,
                                 float min, float max,
                                 RuleSeverity sev)
{
    Rule r;
    r.id          = id;
    r.description = key + " in [" + std::to_string(min) + ", " +
                    std::to_string(max) + "]";
    r.severity    = sev;
    r.condition   = [key, min, max](const RulesContext& ctx) {
        float v = ctx.GetFloat(key);
        return v >= min && v <= max;
    };
    r.violationFn = [key, min, max, sev](const RulesContext& ctx) {
        RuleViolation v;
        v.ruleId     = key + "_range";
        v.severity   = sev;
        v.message    = key + " = " + std::to_string(ctx.GetFloat(key)) +
                       " is out of range [" + std::to_string(min) + ", " +
                       std::to_string(max) + "]";
        v.suggestion = "Adjust " + key + " to be within range.";
        return v;
    };
    return r;
}

/* static */
Rule RulesEngine::MakeEqualRule(const std::string& id,
                                 const std::string& key,
                                 ContextValue expected,
                                 RuleSeverity sev)
{
    Rule r;
    r.id          = id;
    r.description = key + " must equal expected value";
    r.severity    = sev;
    r.condition   = [key, expected](const RulesContext& ctx) {
        auto it = ctx.All().find(key);
        if (it == ctx.All().end()) return false;
        return it->second == expected;
    };
    return r;
}

/* static */
Rule RulesEngine::MakeNonEmptyRule(const std::string& id,
                                    const std::string& key,
                                    RuleSeverity sev)
{
    Rule r;
    r.id          = id;
    r.description = key + " must not be empty";
    r.severity    = sev;
    r.condition   = [key](const RulesContext& ctx) {
        return !ctx.GetString(key).empty();
    };
    return r;
}

// ── Reporting ─────────────────────────────────────────────────────────────────

std::string RulesEngine::BuildMarkdownReport(const EvalResult& result) const
{
    std::ostringstream oss;
    oss << "# Rules Engine Report\n\n"
        << "**Result:** " << (result.passed ? "✅ PASS" : "❌ FAIL") << "\n"
        << "**Violations:** " << result.violations.size() << "\n\n";

    if (!result.violations.empty()) {
        oss << "| Rule | Severity | Message |\n"
            << "|------|----------|---------|\n";
        for (const auto& v : result.violations) {
            const char* sev =
                (v.severity == RuleSeverity::Critical) ? "Critical" :
                (v.severity == RuleSeverity::Error)    ? "Error"    :
                (v.severity == RuleSeverity::Warning)  ? "Warning"  : "Info";
            oss << "| " << v.ruleId << " | " << sev << " | " << v.message << " |\n";
        }
    }
    return oss.str();
}

} // namespace AI
