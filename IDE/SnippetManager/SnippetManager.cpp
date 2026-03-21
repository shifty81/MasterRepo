#include "IDE/SnippetManager/SnippetManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cctype>
#include <unordered_map>

namespace IDE {

struct SnippetManager::Impl {
    std::unordered_map<std::string, Snippet> snippets;

    static std::string toLower(const std::string& s) {
        std::string out = s;
        for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return out;
    }

    // Extract ${VAR} names from body
    static std::vector<std::string> extractVars(const std::string& body) {
        std::vector<std::string> vars;
        size_t pos = 0;
        while ((pos = body.find("${", pos)) != std::string::npos) {
            size_t end = body.find('}', pos + 2);
            if (end == std::string::npos) break;
            std::string var = body.substr(pos + 2, end - pos - 2);
            if (!var.empty() &&
                std::find(vars.begin(), vars.end(), var) == vars.end())
                vars.push_back(var);
            pos = end + 1;
        }
        return vars;
    }

    static std::string expand(const std::string& body,
                               const std::map<std::string,std::string>& vars)
    {
        std::string result = body;
        for (const auto& [k, v] : vars) {
            std::string token = "${" + k + "}";
            size_t pos = 0;
            while ((pos = result.find(token, pos)) != std::string::npos) {
                result.replace(pos, token.size(), v);
                pos += v.size();
            }
        }
        return result;
    }
};

SnippetManager::SnippetManager() : m_impl(new Impl()) { LoadBuiltins(); }
SnippetManager::~SnippetManager() { delete m_impl; }

void SnippetManager::Add(Snippet snippet) {
    snippet.variables = Impl::extractVars(snippet.body);
    m_impl->snippets[snippet.id] = std::move(snippet);
}

bool SnippetManager::Remove(const std::string& id) {
    return m_impl->snippets.erase(id) > 0;
}
bool SnippetManager::Has(const std::string& id) const {
    return m_impl->snippets.count(id) > 0;
}
Snippet* SnippetManager::Get(const std::string& id) {
    auto it = m_impl->snippets.find(id);
    return it != m_impl->snippets.end() ? &it->second : nullptr;
}
const Snippet* SnippetManager::Get(const std::string& id) const {
    auto it = m_impl->snippets.find(id);
    return it != m_impl->snippets.end() ? &it->second : nullptr;
}
size_t SnippetManager::Count() const { return m_impl->snippets.size(); }

std::optional<std::string> SnippetManager::Expand(
    const std::string& id,
    const std::map<std::string,std::string>& vars) const
{
    auto it = m_impl->snippets.find(id);
    if (it == m_impl->snippets.end()) return std::nullopt;
    return Impl::expand(it->second.body, vars);
}

std::optional<std::string> SnippetManager::ExpandByPrefix(
    const std::string& prefix,
    const std::string& language,
    const std::map<std::string,std::string>& vars)
{
    for (auto& [id, sn] : m_impl->snippets) {
        if (sn.prefix != prefix) continue;
        if (!language.empty() && !sn.language.empty() && sn.language != language) continue;
        ++sn.useCount;
        return Impl::expand(sn.body, vars);
    }
    return std::nullopt;
}

std::vector<const Snippet*> SnippetManager::FindByTag(const std::string& tag) const {
    std::vector<const Snippet*> out;
    for (const auto& [_,sn] : m_impl->snippets)
        if (std::find(sn.tags.begin(), sn.tags.end(), tag) != sn.tags.end())
            out.push_back(&sn);
    return out;
}

std::vector<const Snippet*> SnippetManager::FindByLanguage(const std::string& lang) const {
    std::vector<const Snippet*> out;
    for (const auto& [_,sn] : m_impl->snippets)
        if (sn.language == lang || lang.empty()) out.push_back(&sn);
    return out;
}

std::vector<const Snippet*> SnippetManager::Search(const std::string& query) const {
    std::vector<const Snippet*> out;
    std::string ql = Impl::toLower(query);
    for (const auto& [_,sn] : m_impl->snippets) {
        bool match =
            Impl::toLower(sn.name).find(ql) != std::string::npos ||
            Impl::toLower(sn.prefix).find(ql) != std::string::npos ||
            Impl::toLower(sn.description).find(ql) != std::string::npos;
        if (!match) for (const auto& t : sn.tags)
            if (Impl::toLower(t).find(ql) != std::string::npos) { match = true; break; }
        if (match) out.push_back(&sn);
    }
    return out;
}

std::vector<const Snippet*> SnippetManager::All() const {
    std::vector<const Snippet*> out;
    for (const auto& [_,sn] : m_impl->snippets) out.push_back(&sn);
    std::sort(out.begin(), out.end(), [](const Snippet* a, const Snippet* b){
        return a->name < b->name;
    });
    return out;
}

bool SnippetManager::LoadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    // Simple line-based format: id|name|language|prefix|tags|body
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        Snippet sn;
        std::string tagStr;
        std::getline(ss, sn.id,          '|');
        std::getline(ss, sn.name,         '|');
        std::getline(ss, sn.language,     '|');
        std::getline(ss, sn.prefix,       '|');
        std::getline(ss, tagStr,          '|');
        std::getline(ss, sn.body);
        // Parse tags (comma-separated)
        std::istringstream ts(tagStr);
        std::string t;
        while (std::getline(ts, t, ',')) if (!t.empty()) sn.tags.push_back(t);
        // Unescape \n in body
        for (size_t i = 0; (i = sn.body.find("\\n", i)) != std::string::npos; i += 1)
            sn.body.replace(i, 2, "\n");
        if (!sn.id.empty()) Add(std::move(sn));
    }
    return true;
}

bool SnippetManager::SaveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "# id|name|language|prefix|tags(comma)|body(\\n=newline)\n";
    for (const auto& [_,sn] : m_impl->snippets) {
        std::string body = sn.body;
        for (size_t i = 0; (i = body.find('\n', i)) != std::string::npos; i += 2)
            body.replace(i, 1, "\\n");
        std::string tags;
        for (size_t i = 0; i < sn.tags.size(); ++i) {
            if (i) tags += ',';
            tags += sn.tags[i];
        }
        f << sn.id << '|' << sn.name << '|' << sn.language << '|'
          << sn.prefix << '|' << tags << '|' << body << "\n";
    }
    return true;
}

void SnippetManager::LoadBuiltins() {
    auto add = [&](const char* id, const char* name, const char* lang,
                   const char* prefix, const char* tag, const char* body) {
        Snippet sn;
        sn.id = id; sn.name = name; sn.language = lang;
        sn.prefix = prefix; sn.tags = {tag}; sn.body = body;
        Add(std::move(sn));
    };
    add("cpp_class", "C++ Class", "C++", "cls", "class",
        "class ${ClassName} {\npublic:\n    ${ClassName}();\n    ~${ClassName}();\n};\n");
    add("cpp_for", "Range-based For", "C++", "forr", "loop",
        "for (auto& ${elem} : ${container}) {\n    ${body}\n}\n");
    add("cpp_guard", "Header Guard", "C++", "guard", "header",
        "#pragma once\n");
    add("cpp_ns", "Namespace Block", "C++", "ns", "namespace",
        "namespace ${Name} {\n\n${body}\n\n} // namespace ${Name}\n");
    add("cpp_main", "main function", "C++", "main", "entrypoint",
        "int main(int argc, char* argv[]) {\n    ${body}\n    return 0;\n}\n");
    add("lua_fn", "Lua Function", "Lua", "fn", "function",
        "function ${name}(${params})\n    ${body}\nend\n");
    add("py_class","Python Class","Python","cls","class",
        "class ${ClassName}:\n    def __init__(self):\n        ${body}\n");
}

} // namespace IDE
