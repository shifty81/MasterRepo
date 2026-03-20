#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Symbol kinds (matches LSP SymbolKind subset)
// ──────────────────────────────────────────────────────────────

enum class SymbolKind {
    File, Namespace, Class, Method, Property,
    Field, Constructor, Enum, Interface, Function,
    Variable, Constant, String, Number, TypeParameter, Unknown
};

// ──────────────────────────────────────────────────────────────
// Source location
// ──────────────────────────────────────────────────────────────

struct SourceLocation {
    std::string file;
    uint32_t    line   = 0;
    uint32_t    column = 0;
};

// ──────────────────────────────────────────────────────────────
// Symbol entry in the index
// ──────────────────────────────────────────────────────────────

struct Symbol {
    std::string    name;
    SymbolKind     kind       = SymbolKind::Unknown;
    SourceLocation definition;
    std::vector<SourceLocation> references;
    std::string    documentation;   // extracted doc comment
    std::string    signature;       // e.g. function prototype
    std::string    ns;              // containing namespace / class
};

// ──────────────────────────────────────────────────────────────
// Completion item
// ──────────────────────────────────────────────────────────────

struct CompletionItem {
    std::string label;
    std::string detail;
    std::string documentation;
    SymbolKind  kind = SymbolKind::Unknown;
};

// ──────────────────────────────────────────────────────────────
// Diagnostic severity
// ──────────────────────────────────────────────────────────────

enum class DiagSeverity { Error, Warning, Info, Hint };

struct Diagnostic {
    SourceLocation location;
    DiagSeverity   severity = DiagSeverity::Info;
    std::string    message;
    std::string    code;
};

// ──────────────────────────────────────────────────────────────
// CodeIntelligence — LSP-like symbol index, completion & navigation
// ──────────────────────────────────────────────────────────────

class CodeIntelligence {
public:
    // Index management
    void IndexDirectory(const std::string& rootDir,
                        const std::vector<std::string>& extensions = {".cpp",".h",".hpp"});
    void IndexFile(const std::string& filePath);
    void RemoveFile(const std::string& filePath);
    void ClearIndex();

    // Symbol lookup
    std::optional<Symbol>        FindDefinition(const std::string& symbolName) const;
    std::vector<SourceLocation>  FindReferences(const std::string& symbolName) const;
    std::vector<Symbol>          SearchSymbols(const std::string& query,
                                               SymbolKind filter = SymbolKind::Unknown) const;

    // Go-to-definition at a specific file/line/col position
    std::optional<Symbol>        GoToDefinition(const std::string& file,
                                                uint32_t line,
                                                uint32_t col) const;

    // Code completion at a position
    std::vector<CompletionItem>  Complete(const std::string& file,
                                          uint32_t line,
                                          uint32_t col,
                                          const std::string& prefix = "") const;

    // Hover info (symbol under cursor)
    std::optional<Symbol>        Hover(const std::string& file,
                                       uint32_t line,
                                       uint32_t col) const;

    // Diagnostics
    std::vector<Diagnostic>      GetDiagnostics(const std::string& file) const;

    // Statistics
    size_t SymbolCount() const;
    size_t FileCount()   const;

    // Change callback (triggered when index is updated)
    using IndexChangedCallback = std::function<void(const std::string& file)>;
    void OnIndexChanged(IndexChangedCallback cb);

    // Singleton helper
    static CodeIntelligence& Get();

private:
    struct FileEntry {
        std::string               path;
        std::vector<std::string>  symbolNames;
        std::vector<Diagnostic>   diagnostics;
    };

    void ParseFile(const std::string& path);

    std::unordered_map<std::string, Symbol>    m_symbols;   // name → symbol
    std::unordered_map<std::string, FileEntry> m_files;     // path  → entry
    IndexChangedCallback                       m_onChange;
};

} // namespace Core
