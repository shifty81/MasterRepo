#include "AI/CodeLearning/CodeLearning.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace AI {

static uint64_t CLNowMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string CodeLearning::DetectLanguage(const std::string& path) const {
    auto ext = std::filesystem::path(path).extension().string();
    if (ext == ".cpp" || ext == ".cxx" || ext == ".cc") return "C++";
    if (ext == ".h"   || ext == ".hpp" || ext == ".hxx") return "C++ Header";
    if (ext == ".c")   return "C";
    if (ext == ".py")  return "Python";
    if (ext == ".js" || ext == ".ts") return "JavaScript";
    if (ext == ".cs")  return "C#";
    if (ext == ".lua") return "Lua";
    return "Unknown";
}

void CodeLearning::ExtractSymbols(CodeFile& file, const std::string& content) {
    std::istringstream ss(content);
    std::string line;
    uint32_t lineNum = 0;
    while (std::getline(ss, line)) {
        ++lineNum;
        if (line.find("class ") != std::string::npos) {
            CodeSymbol sym;
            sym.kind = "class";
            sym.file = file.path;
            sym.line = lineNum;
            auto start = line.find("class ") + 6;
            auto end = line.find_first_of(" :{", start);
            if (end == std::string::npos) end = line.size();
            sym.name = line.substr(start, end - start);
            if (!sym.name.empty()) file.symbols.push_back(sym);
        } else if (line.find("struct ") != std::string::npos) {
            CodeSymbol sym;
            sym.kind = "struct";
            sym.file = file.path;
            sym.line = lineNum;
            auto start = line.find("struct ") + 7;
            auto end = line.find_first_of(" :{", start);
            if (end == std::string::npos) end = line.size();
            sym.name = line.substr(start, end - start);
            if (!sym.name.empty()) file.symbols.push_back(sym);
        } else if (line.find("enum ") != std::string::npos) {
            CodeSymbol sym;
            sym.kind = "enum";
            sym.file = file.path;
            sym.line = lineNum;
            auto start = line.find("enum ") + 5;
            if (line.substr(start, 6) == "class ") start += 6;
            auto end = line.find_first_of(" :{", start);
            if (end == std::string::npos) end = line.size();
            sym.name = line.substr(start, end - start);
            if (!sym.name.empty()) file.symbols.push_back(sym);
        }
    }
}

void CodeLearning::IndexFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    CodeFile file;
    file.path = path;
    file.language = DetectLanguage(path);
    file.lastIndexed = CLNowMs();
    ExtractSymbols(file, content);
    m_files[path] = std::move(file);
}

void CodeLearning::IndexDirectory(const std::string& dir, const std::vector<std::string>& extensions) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return;
    for (auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        for (const auto& e : extensions) {
            if (ext == e) { IndexFile(entry.path().string()); break; }
        }
    }
}

std::vector<CodeSymbol> CodeLearning::Search(const std::string& query) const {
    std::vector<CodeSymbol> results;
    for (const auto& [path, file] : m_files) {
        for (const auto& sym : file.symbols) {
            if (sym.name.find(query) != std::string::npos ||
                sym.signature.find(query) != std::string::npos) {
                results.push_back(sym);
            }
        }
    }
    return results;
}

CodeFile* CodeLearning::GetFile(const std::string& path) {
    auto it = m_files.find(path);
    return it != m_files.end() ? &it->second : nullptr;
}

std::vector<CodeSymbol> CodeLearning::GetSymbols(const std::string& kind) const {
    std::vector<CodeSymbol> results;
    for (const auto& [path, file] : m_files) {
        for (const auto& sym : file.symbols) {
            if (sym.kind == kind) results.push_back(sym);
        }
    }
    return results;
}

size_t CodeLearning::FileCount() const { return m_files.size(); }

size_t CodeLearning::SymbolCount() const {
    size_t n = 0;
    for (const auto& [path, file] : m_files) n += file.symbols.size();
    return n;
}

bool CodeLearning::SaveIndex(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    for (const auto& [fpath, file] : m_files) {
        f << "FILE\t" << fpath << "\t" << file.language << "\n";
        for (const auto& sym : file.symbols) {
            f << "SYM\t" << sym.kind << "\t" << sym.name << "\t" << sym.line << "\n";
        }
    }
    return true;
}

bool CodeLearning::LoadIndex(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    m_files.clear();
    std::string line;
    CodeFile* current = nullptr;
    while (std::getline(f, line)) {
        if (line.substr(0, 5) == "FILE\t") {
            std::string rest = line.substr(5);
            auto tab = rest.find('\t');
            std::string fpath = (tab != std::string::npos) ? rest.substr(0, tab) : rest;
            CodeFile file; file.path = fpath;
            if (tab != std::string::npos) file.language = rest.substr(tab + 1);
            m_files[fpath] = std::move(file);
            current = &m_files[fpath];
        } else if (line.substr(0, 4) == "SYM\t" && current) {
            std::string rest = line.substr(4);
            auto t1 = rest.find('\t');
            auto t2 = (t1 != std::string::npos) ? rest.find('\t', t1 + 1) : std::string::npos;
            if (t1 != std::string::npos && t2 != std::string::npos) {
                CodeSymbol sym;
                sym.kind = rest.substr(0, t1);
                sym.name = rest.substr(t1 + 1, t2 - t1 - 1);
                try { sym.line = static_cast<uint32_t>(std::stoul(rest.substr(t2 + 1))); } catch (...) {}
                sym.file = current->path;
                current->symbols.push_back(sym);
            }
        }
    }
    return true;
}

void CodeLearning::Clear() { m_files.clear(); }

} // namespace AI
