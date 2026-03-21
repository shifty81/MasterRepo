#pragma once
/**
 * @file SnippetManager.h
 * @brief IDE code snippet library with variable placeholder substitution.
 *
 * Features:
 *   - Snippet: named template text with ${VAR} placeholders
 *   - Tags and language category for filtering
 *   - Expand(name, vars): render snippet with given variable bindings
 *   - Search by tag, language, or substring
 *   - Import/export from JSON-style flat records
 *   - Built-in snippets for common C++ patterns
 */

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace IDE {

// ── Snippet definition ────────────────────────────────────────────────────
struct Snippet {
    std::string              id;
    std::string              name;
    std::string              description;
    std::string              language;    ///< e.g. "C++", "Lua", "Python", ""=any
    std::vector<std::string> tags;
    std::string              body;        ///< Template text; ${VAR} = placeholder
    std::string              prefix;      ///< Short trigger (e.g. "forr", "cls")
    std::vector<std::string> variables;  ///< List of variable names in body
    uint32_t                 useCount{0};
};

// ── Snippet manager ───────────────────────────────────────────────────────
class SnippetManager {
public:
    SnippetManager();
    ~SnippetManager();

    // ── CRUD ─────────────────────────────────────────────────
    void     Add(Snippet snippet);
    bool     Remove(const std::string& id);
    bool     Has(const std::string& id) const;
    Snippet* Get(const std::string& id);
    const Snippet* Get(const std::string& id) const;

    size_t Count() const;

    // ── expansion ─────────────────────────────────────────────
    /// Replace ${VAR} placeholders with supplied values.
    /// Returns nullopt if snippet not found.
    std::optional<std::string> Expand(
        const std::string& id,
        const std::map<std::string, std::string>& vars = {}) const;

    /// Expand by prefix (e.g. user typed "forr<tab>").
    std::optional<std::string> ExpandByPrefix(
        const std::string& prefix,
        const std::string& language = "",
        const std::map<std::string, std::string>& vars = {});

    // ── search ────────────────────────────────────────────────
    std::vector<const Snippet*> FindByTag(const std::string& tag) const;
    std::vector<const Snippet*> FindByLanguage(const std::string& lang) const;
    std::vector<const Snippet*> Search(const std::string& query) const;
    std::vector<const Snippet*> All() const;

    // ── persistence ───────────────────────────────────────────
    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;

    // ── built-in library ──────────────────────────────────────
    void LoadBuiltins();

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace IDE
