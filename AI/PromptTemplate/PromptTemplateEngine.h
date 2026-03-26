#pragma once
/**
 * @file PromptTemplateEngine.h
 * @brief Reusable, parameterised prompt templates for AI agents.
 *
 * Templates are plain text with `{{variable}}` placeholders.  They support
 * optional conditional blocks (`{{#if var}}…{{/if}}`), list iteration
 * (`{{#each list}}…{{/each}}`), and nested templates via `{{>partial}}`.
 *
 * Typical usage:
 * @code
 *   PromptTemplateEngine tpl;
 *   tpl.Init();
 *   tpl.RegisterTemplate("code_review",
 *       "Review the following {{language}} code:\n\n{{code}}\n\n"
 *       "Focus on: {{focus}}");
 *   std::string prompt = tpl.Render("code_review", {
 *       {"language", "C++"},
 *       {"code", sourceText},
 *       {"focus", "memory safety"}
 *   });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace AI {

// ── Variable map ──────────────────────────────────────────────────────────────

using TemplateVars = std::unordered_map<std::string, std::string>;

// ── A registered template ─────────────────────────────────────────────────────

struct PromptTemplate {
    std::string id;
    std::string source;           ///< raw template text with {{placeholders}}
    std::string description;
    std::vector<std::string> requiredVars;
};

// ── Render result ─────────────────────────────────────────────────────────────

struct PromptRenderResult {
    bool        succeeded{false};
    std::string text;             ///< fully rendered prompt
    std::string errorMessage;
};

// ── PromptTemplateEngine ──────────────────────────────────────────────────────

class PromptTemplateEngine {
public:
    PromptTemplateEngine();
    ~PromptTemplateEngine();

    void Init();
    void Shutdown();

    // ── Template registry ─────────────────────────────────────────────────────

    void RegisterTemplate(const std::string& id, const std::string& source,
                          const std::string& description = "");

    bool LoadTemplatesFromFile(const std::string& jsonPath);
    bool SaveTemplatesToFile(const std::string& jsonPath) const;

    bool HasTemplate(const std::string& id) const;
    PromptTemplate GetTemplate(const std::string& id) const;
    std::vector<PromptTemplate> AllTemplates() const;

    // ── Rendering ─────────────────────────────────────────────────────────────

    /// Render a registered template with variables.
    PromptRenderResult Render(const std::string& templateId,
                              const TemplateVars& vars = {}) const;

    /// Render a raw template string directly (without registering).
    PromptRenderResult RenderRaw(const std::string& source,
                                 const TemplateVars& vars = {}) const;

    // ── Partials ──────────────────────────────────────────────────────────────

    void RegisterPartial(const std::string& name, const std::string& source);
    bool HasPartial(const std::string& name) const;

    // ── Helpers / filters ─────────────────────────────────────────────────────

    /// Register a named transform applied via `{{var|filterName}}`.
    void RegisterFilter(const std::string& name,
                        std::function<std::string(const std::string&)> fn);

    // ── Validation ────────────────────────────────────────────────────────────

    /// Check all required vars are present.
    bool Validate(const std::string& templateId, const TemplateVars& vars,
                  std::vector<std::string>& missingVars) const;

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace AI
