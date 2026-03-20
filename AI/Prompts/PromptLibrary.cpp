#include "AI/Prompts/PromptLibrary.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace AI {

PromptLibrary::PromptLibrary() { RegisterBuiltins(); }

void PromptLibrary::Register(const PromptTemplate& tmpl) { m_templates[tmpl.name] = tmpl; }

PromptTemplate* PromptLibrary::Get(const std::string& name) {
    auto it = m_templates.find(name);
    return it != m_templates.end() ? &it->second : nullptr;
}

std::string PromptLibrary::Render(const std::string& name, const std::map<std::string,std::string>& vars) const {
    auto it = m_templates.find(name);
    if (it == m_templates.end()) return "";
    std::string result = it->second.templateText;
    for (const auto& [key, value] : vars) {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.size(), value);
            pos += value.size();
        }
    }
    return result;
}

std::vector<std::string> PromptLibrary::List(const std::string& category) const {
    std::vector<std::string> names;
    for (const auto& [name, tmpl] : m_templates) {
        if (category.empty() || tmpl.category == category) names.push_back(name);
    }
    return names;
}

size_t PromptLibrary::Count() const { return m_templates.size(); }

bool PromptLibrary::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        auto t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        PromptTemplate tmpl;
        tmpl.name = line.substr(0, t1);
        tmpl.id = tmpl.name;
        tmpl.category = line.substr(t1 + 1, t2 - t1 - 1);
        tmpl.templateText = line.substr(t2 + 1);
        m_templates[tmpl.name] = tmpl;
    }
    return true;
}

bool PromptLibrary::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    for (const auto& [name, tmpl] : m_templates) {
        f << tmpl.name << "\t" << tmpl.category << "\t" << tmpl.templateText << "\n";
    }
    return true;
}

void PromptLibrary::RegisterBuiltins() {
    Register({"generate_code", "generate_code", "code",
        "Write {{language}} code that does the following:\n{{description}}\n\nRequirements:\n{{requirements}}",
        {"language", "description", "requirements"}});

    Register({"fix_error", "fix_error", "debug",
        "Fix the following {{language}} compile error:\n\nError: {{error}}\n\nCode:\n{{code}}\n\nProvide the corrected code.",
        {"language", "error", "code"}});

    Register({"explain_code", "explain_code", "docs",
        "Explain what the following {{language}} code does:\n\n{{code}}",
        {"language", "code"}});

    Register({"write_test", "write_test", "testing",
        "Write unit tests for the following {{language}} function:\n\n{{code}}\n\nUse {{framework}} test framework.",
        {"language", "code", "framework"}});

    Register({"refactor", "refactor", "code",
        "Refactor the following {{language}} code to improve {{goal}}:\n\n{{code}}",
        {"language", "goal", "code"}});
}

} // namespace AI
