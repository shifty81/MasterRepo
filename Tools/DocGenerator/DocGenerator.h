#pragma once
// DocGenerator — Phase 10.5: AI-assisted documentation generator
// Scans C++ headers, extracts class/function declarations and doc comments,
// then emits Markdown reference pages (optionally enriched by the AI layer).

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Data model
// ─────────────────────────────────────────────────────────────────────────────

struct ParamDoc {
    std::string type;
    std::string name;
    std::string description; // from @param tag or extracted comment
};

struct FunctionDoc {
    std::string   name;
    std::string   returnType;
    std::string   briefComment;  // first line of block comment
    std::string   fullComment;   // full block comment body
    std::vector<ParamDoc> params;
    bool          isStatic  = false;
    bool          isVirtual = false;
    bool          isConst   = false;
    std::string   signature; // raw declaration line
};

struct FieldDoc {
    std::string type;
    std::string name;
    std::string comment;
};

struct ClassDoc {
    std::string   name;
    std::string   baseClass;
    std::string   briefComment;
    std::string   fullComment;
    std::string   headerPath;   // relative to repo root
    std::string   namespaceName;
    std::vector<FunctionDoc> publicMethods;
    std::vector<FieldDoc>    publicFields;
    std::vector<FunctionDoc> staticMethods;
};

struct FileDoc {
    std::string            filePath;   // relative path from scan root
    std::string            briefComment;
    std::vector<ClassDoc>  classes;
    std::vector<FunctionDoc> freeFunctions;
};

// ─────────────────────────────────────────────────────────────────────────────
// DocGeneratorConfig
// ─────────────────────────────────────────────────────────────────────────────

struct DocGeneratorConfig {
    std::string scanRoot;          // root directory to scan for headers
    std::string outputDir;         // where Markdown files are written
    std::string projectName;       // used in the index page title
    bool        recurse      = true;
    bool        includePrivate = false;
    bool        generateIndex = true; // emit Docs/API/index.md
    // Glob patterns of files to skip (matched against relative path)
    std::vector<std::string> excludePatterns;
    // Optional: hook to enrich a ClassDoc with AI-generated summary
    // Signature: (class_doc) -> enriched_brief (may be empty to skip)
    std::function<std::string(const ClassDoc&)> aiEnrichHook;
};

// ─────────────────────────────────────────────────────────────────────────────
// DocGenerator
// ─────────────────────────────────────────────────────────────────────────────

class DocGenerator {
public:
    explicit DocGenerator(DocGeneratorConfig cfg);

    // ── Scan ─────────────────────────────────────────────────────────────────

    /// Recursively scan all .h/.hpp files under cfg.scanRoot and populate
    /// the internal document model.  Returns number of files parsed.
    int Scan();

    /// Return the parsed file docs (populated after Scan()).
    const std::vector<FileDoc>& GetFileDocs() const { return m_fileDocs; }

    // ── Generate ──────────────────────────────────────────────────────────────

    /// Write one Markdown file per header into cfg.outputDir.
    /// Also writes an index page if cfg.generateIndex == true.
    /// Returns number of files written.
    int Generate();

    // ── Convenience all-in-one ────────────────────────────────────────────────

    /// Scan + Generate in one call.  Returns number of Markdown files written.
    int Run();

    // ── Statistics ────────────────────────────────────────────────────────────
    int ClassCount()    const;
    int FunctionCount() const;

private:
    DocGeneratorConfig       m_cfg;
    std::vector<FileDoc>     m_fileDocs;

    // Parsing helpers
    FileDoc ParseFile(const std::filesystem::path& path);
    void    ParseClassBlock(const std::string& src, size_t openBrace,
                             ClassDoc& cls);
    FunctionDoc ParseFunctionDecl(const std::string& line,
                                  const std::string& comment);

    // Markdown emitters
    std::string RenderFileDoc  (const FileDoc&)     const;
    std::string RenderClassDoc (const ClassDoc&)    const;
    std::string RenderFuncDoc  (const FunctionDoc&) const;
    std::string RenderIndex    ()                   const;

    // Utilities
    static bool  MatchesExclude(const std::string& rel,
                                 const std::vector<std::string>& patterns);
    static std::string StripComment(const std::string& raw);
    static std::string RelativePath(const std::filesystem::path& base,
                                    const std::filesystem::path& full);
};

} // namespace Tools
