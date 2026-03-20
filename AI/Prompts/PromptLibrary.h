#pragma once
#include <string>
#include <vector>
#include <map>

namespace AI {

struct PromptTemplate {
    std::string              id;
    std::string              name;
    std::string              category;
    std::string              templateText;
    std::vector<std::string> variables;
};

class PromptLibrary {
public:
    PromptLibrary();

    void              Register(const PromptTemplate& tmpl);
    PromptTemplate*   Get(const std::string& name);
    std::string       Render(const std::string& name, const std::map<std::string,std::string>& vars) const;
    std::vector<std::string> List(const std::string& category = "") const;
    size_t            Count() const;
    bool              LoadFromFile(const std::string& path);
    bool              SaveToFile(const std::string& path) const;

private:
    void RegisterBuiltins();

    std::map<std::string, PromptTemplate> m_templates;
};

} // namespace AI
