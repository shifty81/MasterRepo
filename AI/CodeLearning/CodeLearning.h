#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace AI {

struct CodeSymbol {
    std::string name;
    std::string kind;
    std::string file;
    uint32_t    line      = 0;
    std::string signature;
};

struct CodeFile {
    std::string             path;
    std::string             language;
    std::vector<CodeSymbol> symbols;
    uint64_t                lastIndexed = 0;
};

class CodeLearning {
public:
    void                    IndexFile(const std::string& path);
    void                    IndexDirectory(const std::string& dir, const std::vector<std::string>& extensions);
    std::vector<CodeSymbol> Search(const std::string& query) const;
    CodeFile*               GetFile(const std::string& path);
    std::vector<CodeSymbol> GetSymbols(const std::string& kind) const;
    size_t                  FileCount() const;
    size_t                  SymbolCount() const;
    bool                    SaveIndex(const std::string& path) const;
    bool                    LoadIndex(const std::string& path);
    void                    Clear();

private:
    std::string DetectLanguage(const std::string& path) const;
    void        ExtractSymbols(CodeFile& file, const std::string& content);

    std::unordered_map<std::string, CodeFile> m_files;
};

} // namespace AI
