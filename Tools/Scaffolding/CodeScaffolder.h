#pragma once
// CodeScaffolder — Code scaffolding pipeline (Blueprint: dev_tools/code_scaffolding_pipeline)
//
// Generates boilerplate code, CMakeLists, headers, and implementation stubs
// from a declarative description.  Used by:
//   • SelfBuildAgent — when autonomously adding new modules
//   • Editor NL Assistant — "add new system" command
//   • CLI tool for manual scaffolding
//
// Templates reside in Scripts/Templates/ (or inline defaults).
// Output is written to a sandbox path first (AI Sandbox Rule), then moved
// on confirmation.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Component descriptor
// ─────────────────────────────────────────────────────────────────────────────

enum class ComponentKind {
    CppHeader,        // .h
    CppSource,        // .cpp
    CMakeLists,
    PythonScript,
    LuaScript,
    JsonConfig,
    Markdown,
    ShellScript,
};

struct ComponentSpec {
    ComponentKind kind;
    std::string   relativePath;   // relative to module root
    std::string   templateName;   // name of template to use
    std::unordered_map<std::string,std::string> vars; // substitution variables
};

// ─────────────────────────────────────────────────────────────────────────────
// Module blueprint
// ─────────────────────────────────────────────────────────────────────────────

struct ModuleBlueprint {
    std::string              name;          // e.g. "ShieldSystem"
    std::string              ns;            // C++ namespace, e.g. "Engine::Shield"
    std::string              rootDir;       // e.g. "Engine/Shield"
    std::string              libraryName;   // e.g. "AtlasEngine"
    std::string              description;
    std::vector<std::string> dependencies; // CMake target names
    std::vector<ComponentSpec> components;
};

// ─────────────────────────────────────────────────────────────────────────────
// Scaffolding result
// ─────────────────────────────────────────────────────────────────────────────

struct ScaffoldResult {
    bool passed = true;
    std::vector<std::string> filesCreated;
    std::vector<std::string> filesSkipped;  // already exist
    std::vector<std::string> errors;
    std::string sandboxPath;   // staging area before finalise
};

// ─────────────────────────────────────────────────────────────────────────────
// CodeScaffolderConfig
// ─────────────────────────────────────────────────────────────────────────────

struct CodeScaffolderConfig {
    std::string  repoRoot;
    std::string  sandboxRoot  = "/tmp/atlas_scaffold_sandbox";
    std::string  templateRoot;  // defaults to <repoRoot>/Scripts/Templates
    bool         dryRun       = false;   // generate but do not write
    bool         overwrite    = false;   // overwrite existing files
    // Called before each file is written; return false to skip
    std::function<bool(const std::string& path)> confirmFn;
};

// ─────────────────────────────────────────────────────────────────────────────
// Template engine (minimal ${VAR} substitution)
// ─────────────────────────────────────────────────────────────────────────────

class TemplateEngine {
public:
    explicit TemplateEngine(const std::string& templateRoot = "");

    /// Load a named template file. Returns empty string on failure.
    std::string Load(const std::string& name) const;

    /// Substitute ${KEY} tokens.
    static std::string Render(const std::string& tmpl,
                               const std::unordered_map<std::string,std::string>& vars);

    /// Register an inline template (no file needed).
    void RegisterInline(const std::string& name, const std::string& content);

private:
    std::string m_root;
    std::unordered_map<std::string,std::string> m_inline;
};

// ─────────────────────────────────────────────────────────────────────────────
// CodeScaffolder
// ─────────────────────────────────────────────────────────────────────────────

class CodeScaffolder {
public:
    explicit CodeScaffolder(CodeScaffolderConfig cfg = {});

    // ── High-level API ───────────────────────────────────────────────────────

    /// Scaffold a full module (headers, source, CMakeLists) from a blueprint.
    ScaffoldResult ScaffoldModule(const ModuleBlueprint& bp);

    /// Quick scaffold: creates a .h + .cpp pair for a named class.
    ScaffoldResult ScaffoldClass(const std::string& className,
                                  const std::string& ns,
                                  const std::string& rootDir);

    /// Scaffold a new AI agent (CodeAgent-style boilerplate).
    ScaffoldResult ScaffoldAgent(const std::string& agentName);

    // ── Finalisation ────────────────────────────────────────────────────────

    /// Move sandbox output to the real destination.
    bool Finalise(const ScaffoldResult& result);

    /// Discard sandbox output.
    void Discard(const ScaffoldResult& result);

    // ── Template management ──────────────────────────────────────────────────

    TemplateEngine& GetTemplateEngine();

    // ── Reporting ────────────────────────────────────────────────────────────

    std::string BuildReport(const ScaffoldResult& result) const;

private:
    CodeScaffolderConfig m_cfg;
    TemplateEngine       m_templates;

    std::string SandboxPath(const std::string& rel) const;
    bool WriteFile(const std::string& path, const std::string& content,
                   ScaffoldResult& result) const;

    void RegisterDefaultTemplates();
    ScaffoldResult ScaffoldComponent(const ComponentSpec& comp,
                                      const ModuleBlueprint& bp);
};

} // namespace Tools
