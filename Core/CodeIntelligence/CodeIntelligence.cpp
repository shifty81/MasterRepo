#include "Core/CodeIntelligence/CodeIntelligence.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <regex>
#include <algorithm>

namespace fs = std::filesystem;

namespace Core {

// ── Singleton ──────────────────────────────────────────────────

CodeIntelligence& CodeIntelligence::Get() {
    static CodeIntelligence instance;
    return instance;
}

// ── Index management ───────────────────────────────────────────

void CodeIntelligence::IndexDirectory(const std::string& rootDir,
                                       const std::vector<std::string>& extensions) {
    if (!fs::exists(rootDir)) return;
    for (auto& entry : fs::recursive_directory_iterator(rootDir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
            IndexFile(entry.path().string());
        }
    }
}

void CodeIntelligence::IndexFile(const std::string& filePath) {
    ParseFile(filePath);
    if (m_onChange) m_onChange(filePath);
}

void CodeIntelligence::RemoveFile(const std::string& filePath) {
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) return;
    for (const auto& name : fit->second.symbolNames) {
        m_symbols.erase(name);
    }
    m_files.erase(fit);
    if (m_onChange) m_onChange(filePath);
}

void CodeIntelligence::ClearIndex() {
    m_symbols.clear();
    m_files.clear();
}

// ── Symbol lookup ──────────────────────────────────────────────

std::optional<Symbol> CodeIntelligence::FindDefinition(const std::string& symbolName) const {
    auto it = m_symbols.find(symbolName);
    if (it == m_symbols.end()) return std::nullopt;
    return it->second;
}

std::vector<SourceLocation> CodeIntelligence::FindReferences(const std::string& symbolName) const {
    auto it = m_symbols.find(symbolName);
    if (it == m_symbols.end()) return {};
    return it->second.references;
}

std::vector<Symbol> CodeIntelligence::SearchSymbols(const std::string& query,
                                                     SymbolKind filter) const {
    std::vector<Symbol> results;
    for (auto& [name, sym] : m_symbols) {
        if (filter != SymbolKind::Unknown && sym.kind != filter) continue;
        if (name.find(query) != std::string::npos ||
            sym.name.find(query) != std::string::npos) {
            results.push_back(sym);
        }
    }
    return results;
}

std::optional<Symbol> CodeIntelligence::GoToDefinition(const std::string& file,
                                                        uint32_t line,
                                                        uint32_t col) const {
    // Find the symbol whose definition matches this location
    for (auto& [name, sym] : m_symbols) {
        if (sym.definition.file == file &&
            sym.definition.line == line) {
            (void)col;
            return sym;
        }
    }
    return std::nullopt;
}

std::vector<CompletionItem> CodeIntelligence::Complete(const std::string& /*file*/,
                                                        uint32_t /*line*/,
                                                        uint32_t /*col*/,
                                                        const std::string& prefix) const {
    std::vector<CompletionItem> items;
    for (auto& [name, sym] : m_symbols) {
        if (prefix.empty() || name.rfind(prefix, 0) == 0) {
            CompletionItem item;
            item.label         = sym.name;
            item.detail        = sym.signature;
            item.documentation = sym.documentation;
            item.kind          = sym.kind;
            items.push_back(std::move(item));
        }
    }
    // Sort alphabetically
    std::sort(items.begin(), items.end(),
              [](const CompletionItem& a, const CompletionItem& b){
                  return a.label < b.label;
              });
    return items;
}

std::optional<Symbol> CodeIntelligence::Hover(const std::string& file,
                                               uint32_t line,
                                               uint32_t col) const {
    return GoToDefinition(file, line, col);
}

std::vector<Diagnostic> CodeIntelligence::GetDiagnostics(const std::string& file) const {
    auto it = m_files.find(file);
    if (it == m_files.end()) return {};
    return it->second.diagnostics;
}

// ── Statistics ─────────────────────────────────────────────────

size_t CodeIntelligence::SymbolCount() const { return m_symbols.size(); }
size_t CodeIntelligence::FileCount()   const { return m_files.size(); }

void CodeIntelligence::OnIndexChanged(IndexChangedCallback cb) {
    m_onChange = std::move(cb);
}

// ── Parsing ────────────────────────────────────────────────────

void CodeIntelligence::ParseFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    FileEntry entry;
    entry.path = path;

    // Simple regex-based extraction of class/struct/function definitions
    // Regex patterns are `static` so they are compiled only once per process lifetime
    static const std::regex rClass(R"(^\s*(?:class|struct)\s+(\w+))");
    static const std::regex rFunc(R"(^\s*(?:\w[\w\s\*&:<>]*)\s+(\w+)\s*\()");
    static const std::regex rNamespace(R"(^\s*namespace\s+(\w+))");
    static const std::regex rEnum(R"(^\s*enum(?:\s+class)?\s+(\w+))");

    std::string line;
    uint32_t lineNo = 0;
    std::string currentNs;

    while (std::getline(file, line)) {
        ++lineNo;
        std::smatch m;

        SymbolKind kind = SymbolKind::Unknown;
        std::string symbolName;

        if (std::regex_search(line, m, rNamespace)) {
            currentNs   = m[1].str();
            kind        = SymbolKind::Namespace;
            symbolName  = currentNs;
        } else if (std::regex_search(line, m, rClass)) {
            kind       = SymbolKind::Class;
            symbolName = m[1].str();
        } else if (std::regex_search(line, m, rEnum)) {
            kind       = SymbolKind::Enum;
            symbolName = m[1].str();
        } else if (std::regex_search(line, m, rFunc)) {
            kind       = SymbolKind::Function;
            symbolName = m[1].str();
        }

        if (kind != SymbolKind::Unknown && !symbolName.empty()) {
            Symbol sym;
            sym.name            = symbolName;
            sym.kind            = kind;
            sym.ns              = currentNs;
            sym.definition.file = path;
            sym.definition.line = lineNo;
            sym.definition.column = 0;
            sym.signature       = line;

            m_symbols[symbolName] = std::move(sym);
            entry.symbolNames.push_back(symbolName);
        }
    }

    m_files[path] = std::move(entry);
}

} // namespace Core
