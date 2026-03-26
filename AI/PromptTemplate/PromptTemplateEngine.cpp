#include "AI/PromptTemplate/PromptTemplateEngine.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace AI {

// ── Simple {{var}} substitution + {{#if var}}…{{/if}} blocks ─────────────────

static std::string ApplyFilters(const std::string& value,
    const std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>>& filters,
    const std::string& filterName) {
    for (auto& [name, fn] : filters)
        if (name == filterName) return fn(value);
    return value;
}

static std::string RenderSource(
    const std::string& src,
    const TemplateVars& vars,
    const std::unordered_map<std::string, std::string>& partials,
    const std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>>& filters) {

    std::string out;
    out.reserve(src.size() * 2);
    size_t pos = 0;

    while (pos < src.size()) {
        size_t open = src.find("{{", pos);
        if (open == std::string::npos) {
            out += src.substr(pos);
            break;
        }
        out += src.substr(pos, open - pos);
        size_t close = src.find("}}", open + 2);
        if (close == std::string::npos) { out += src.substr(open); break; }

        std::string tag = src.substr(open + 2, close - open - 2);
        // Trim whitespace
        size_t ts = tag.find_first_not_of(" \t");
        size_t te = tag.find_last_not_of(" \t");
        if (ts != std::string::npos) tag = tag.substr(ts, te - ts + 1);

        if (tag.empty()) { pos = close + 2; continue; }

        // Partial: {{>name}}
        if (tag[0] == '>') {
            std::string pname = tag.substr(1);
            if (ts = pname.find_first_not_of(" "); ts != std::string::npos)
                pname = pname.substr(ts);
            auto it = partials.find(pname);
            if (it != partials.end())
                out += RenderSource(it->second, vars, partials, filters);
            pos = close + 2;
            continue;
        }

        // Conditional block: {{#if var}}…{{/if}}
        if (tag.substr(0, 4) == "#if ") {
            std::string varName = tag.substr(4);
            bool cond = vars.count(varName) && !vars.at(varName).empty()
                        && vars.at(varName) != "false" && vars.at(varName) != "0";
            std::string endTag = "{{/if}}";
            size_t endPos = src.find(endTag, close + 2);
            if (endPos == std::string::npos) { pos = close + 2; continue; }
            if (cond) {
                std::string inner = src.substr(close + 2, endPos - close - 2);
                out += RenderSource(inner, vars, partials, filters);
            }
            pos = endPos + endTag.size();
            continue;
        }

        // Variable (with optional filter): {{var|filter}}
        std::string filterName;
        std::string varName = tag;
        size_t pipe = tag.find('|');
        if (pipe != std::string::npos) {
            varName    = tag.substr(0, pipe);
            filterName = tag.substr(pipe + 1);
        }

        auto it = vars.find(varName);
        if (it != vars.end()) {
            std::string val = it->second;
            if (!filterName.empty())
                val = ApplyFilters(val, filters, filterName);
            out += val;
        } else {
            // Leave unresolved variables as empty
        }
        pos = close + 2;
    }
    return out;
}

// ── Pimpl ─────────────────────────────────────────────────────────────────────

struct PromptTemplateEngine::Impl {
    std::unordered_map<std::string, PromptTemplate> templates;
    std::unordered_map<std::string, std::string>    partials;
    std::vector<std::pair<std::string, std::function<std::string(const std::string&)>>> filters;
};

PromptTemplateEngine::PromptTemplateEngine() : m_impl(new Impl()) {}
PromptTemplateEngine::~PromptTemplateEngine() { delete m_impl; }
void PromptTemplateEngine::Init()     {}
void PromptTemplateEngine::Shutdown() {}

void PromptTemplateEngine::RegisterTemplate(const std::string& id,
                                            const std::string& source,
                                            const std::string& description) {
    PromptTemplate t;
    t.id          = id;
    t.source      = source;
    t.description = description;
    // Scan for required vars: any {{word}} not starting with # or /
    size_t p = 0;
    while ((p = source.find("{{", p)) != std::string::npos) {
        size_t e = source.find("}}", p);
        if (e == std::string::npos) break;
        std::string tag = source.substr(p + 2, e - p - 2);
        if (!tag.empty() && tag[0] != '#' && tag[0] != '/' && tag[0] != '>')
            t.requiredVars.push_back(tag.substr(0, tag.find('|')));
        p = e + 2;
    }
    // Deduplicate
    std::sort(t.requiredVars.begin(), t.requiredVars.end());
    t.requiredVars.erase(std::unique(t.requiredVars.begin(), t.requiredVars.end()),
                         t.requiredVars.end());
    m_impl->templates[id] = std::move(t);
}

bool PromptTemplateEngine::LoadTemplatesFromFile(const std::string& path) {
    std::ifstream f(path);
    return f.is_open(); // full impl delegates to Core/Serialization
}

bool PromptTemplateEngine::SaveTemplatesToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    for (auto& [id, t] : m_impl->templates)
        f << "{\"id\":\"" << id << "\",\"source\":\"(omitted)\"}\n";
    return true;
}

bool PromptTemplateEngine::HasTemplate(const std::string& id) const {
    return m_impl->templates.count(id) > 0;
}

PromptTemplate PromptTemplateEngine::GetTemplate(const std::string& id) const {
    auto it = m_impl->templates.find(id);
    return it != m_impl->templates.end() ? it->second : PromptTemplate{};
}

std::vector<PromptTemplate> PromptTemplateEngine::AllTemplates() const {
    std::vector<PromptTemplate> out;
    for (auto& [k, v] : m_impl->templates) out.push_back(v);
    return out;
}

PromptRenderResult PromptTemplateEngine::Render(const std::string& id,
                                                const TemplateVars& vars) const {
    auto it = m_impl->templates.find(id);
    if (it == m_impl->templates.end())
        return {false, "", "Template not found: " + id};
    return RenderRaw(it->second.source, vars);
}

PromptRenderResult PromptTemplateEngine::RenderRaw(const std::string& source,
                                                   const TemplateVars& vars) const {
    PromptRenderResult r;
    r.text      = RenderSource(source, vars, m_impl->partials, m_impl->filters);
    r.succeeded = true;
    return r;
}

void PromptTemplateEngine::RegisterPartial(const std::string& name,
                                           const std::string& source) {
    m_impl->partials[name] = source;
}

bool PromptTemplateEngine::HasPartial(const std::string& name) const {
    return m_impl->partials.count(name) > 0;
}

void PromptTemplateEngine::RegisterFilter(const std::string& name,
    std::function<std::string(const std::string&)> fn) {
    m_impl->filters.push_back({name, std::move(fn)});
}

bool PromptTemplateEngine::Validate(const std::string& id,
                                    const TemplateVars& vars,
                                    std::vector<std::string>& missing) const {
    auto it = m_impl->templates.find(id);
    if (it == m_impl->templates.end()) return false;
    missing.clear();
    for (auto& req : it->second.requiredVars)
        if (!vars.count(req)) missing.push_back(req);
    return missing.empty();
}

} // namespace AI
