#include "Tools/Locator/SymbolLocator/SymbolLocator.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <chrono>

namespace Tools {

const char* SymbolKindName(SymbolKind k) {
    switch (k) {
    case SymbolKind::Function:  return "Function";
    case SymbolKind::Class:     return "Class";
    case SymbolKind::Struct:    return "Struct";
    case SymbolKind::Enum:      return "Enum";
    case SymbolKind::Define:    return "Define";
    case SymbolKind::Namespace: return "Namespace";
    case SymbolKind::Variable:  return "Variable";
    default:                    return "Unknown";
    }
}

// ── Fuzzy score ────────────────────────────────────────────────────────────
static float fuzzyScore(const std::string& query, const std::string& target) {
    if (query.empty()) return 1.0f;
    // Simple scoring: exact match > prefix > substring > char-by-char
    std::string tl = target, ql = query;
    auto toLower = [](std::string s){ for (auto& c:s) c=static_cast<char>(::tolower(c)); return s; };
    tl = toLower(tl); ql = toLower(ql);
    if (tl == ql) return 1.0f;
    if (tl.rfind(ql, 0) == 0) return 0.9f;  // prefix
    if (tl.find(ql) != std::string::npos) return 0.7f;  // substring
    // char-by-char subsequence
    size_t qi = 0;
    for (char c : tl) { if (qi<ql.size() && c==ql[qi]) ++qi; }
    return qi == ql.size() ? 0.4f : 0.0f;
}

// ── Regex patterns ─────────────────────────────────────────────────────────
static const std::regex RE_FUNCTION(
    R"(^\s*(?:(?:inline|static|virtual|explicit|constexpr|override|const)\s+)*[\w:<>*&,\s]+\s+(\w+)\s*\()",
    std::regex::optimize);
static const std::regex RE_CLASS(
    R"(^\s*class\s+(\w+))", std::regex::optimize);
static const std::regex RE_STRUCT(
    R"(^\s*struct\s+(\w+))", std::regex::optimize);
static const std::regex RE_ENUM(
    R"(^\s*enum(?:\s+class)?\s+(\w+))", std::regex::optimize);
static const std::regex RE_DEFINE(
    R"(^\s*#\s*define\s+(\w+))", std::regex::optimize);
static const std::regex RE_NS(
    R"(^\s*namespace\s+(\w+))", std::regex::optimize);

static void parseFile(const std::string& path,
                       std::vector<SymbolEntry>& out)
{
    std::ifstream f(path); if (!f.is_open()) return;
    std::string line;
    uint32_t lineNum = 0;
    std::smatch m;
    while (std::getline(f, line)) {
        ++lineNum;
        SymbolEntry e;
        e.filePath  = path;
        e.line      = lineNum;
        e.signature = line;

        if (std::regex_search(line, m, RE_DEFINE)) {
            e.name = m[1]; e.kind = SymbolKind::Define;
        } else if (std::regex_search(line, m, RE_NS)) {
            e.name = m[1]; e.kind = SymbolKind::Namespace;
        } else if (std::regex_search(line, m, RE_CLASS)) {
            e.name = m[1]; e.kind = SymbolKind::Class;
        } else if (std::regex_search(line, m, RE_STRUCT)) {
            e.name = m[1]; e.kind = SymbolKind::Struct;
        } else if (std::regex_search(line, m, RE_ENUM)) {
            e.name = m[1]; e.kind = SymbolKind::Enum;
        } else if (std::regex_search(line, m, RE_FUNCTION)) {
            e.name = m[1]; e.kind = SymbolKind::Function;
        } else {
            continue;
        }
        if (e.name.empty()||e.name=="if"||e.name=="for"||e.name=="while"||
            e.name=="return"||e.name=="switch") continue;
        out.push_back(std::move(e));
    }
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct SymbolLocator::Impl {
    std::vector<std::string>    roots;
    std::vector<std::string>    extensions;
    size_t                      maxFileSize{512*1024};
    std::vector<SymbolEntry>    index;
    bool                        indexed{false};
    SymbolIndexStats            stats;
    std::vector<IndexReadyCb>   readyCbs;
    std::vector<ProgressCb>     progressCbs;
};

SymbolLocator::SymbolLocator() : m_impl(new Impl()) {
    for (const char* e : {".h",".hpp",".c",".cpp",".cxx",".cc",".hxx"})
        m_impl->extensions.push_back(e);
}
SymbolLocator::~SymbolLocator() { delete m_impl; }

void SymbolLocator::AddSearchRoot(const std::string& d) { m_impl->roots.push_back(d); }
void SymbolLocator::AddExtension(const std::string& e)  { m_impl->extensions.push_back(e); }
void SymbolLocator::SetMaxFileSize(size_t b)            { m_impl->maxFileSize = b; }
void SymbolLocator::OnIndexReady(IndexReadyCb cb)       { m_impl->readyCbs.push_back(std::move(cb)); }
void SymbolLocator::OnProgress(ProgressCb cb)           { m_impl->progressCbs.push_back(std::move(cb)); }
bool SymbolLocator::IsIndexed() const                   { return m_impl->indexed; }
SymbolIndexStats SymbolLocator::Stats() const           { return m_impl->stats; }

void SymbolLocator::ClearIndex() {
    m_impl->index.clear();
    m_impl->indexed = false;
    m_impl->stats = {};
}

void SymbolLocator::BuildIndex() {
    namespace fs = std::filesystem;
    m_impl->index.clear();
    auto t0 = std::chrono::steady_clock::now();
    std::vector<std::string> files;
    for (const auto& root : m_impl->roots) {
        if (!fs::exists(root)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(root,
             fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            if (entry.file_size() > m_impl->maxFileSize) continue;
            std::string ext = entry.path().extension().string();
            for (const auto& e : m_impl->extensions)
                if (e == ext) { files.push_back(entry.path().string()); break; }
        }
    }
    size_t total = files.size(), done = 0;
    for (const auto& f : files) {
        parseFile(f, m_impl->index);
        ++done;
        for (auto& cb : m_impl->progressCbs) cb(done, total);
    }
    auto t1 = std::chrono::steady_clock::now();
    m_impl->stats.filesIndexed = total;
    m_impl->stats.symbolCount  = m_impl->index.size();
    m_impl->stats.indexTimeMs  =
        std::chrono::duration<double,std::milli>(t1-t0).count();
    m_impl->indexed = true;
    for (auto& cb : m_impl->readyCbs) cb(m_impl->stats);
}

void SymbolLocator::IncrementalReindex(const std::string& filePath) {
    // Remove old entries for this file, re-parse
    m_impl->index.erase(
        std::remove_if(m_impl->index.begin(), m_impl->index.end(),
            [&filePath](const SymbolEntry& e){ return e.filePath==filePath; }),
        m_impl->index.end());
    parseFile(filePath, m_impl->index);
    m_impl->stats.symbolCount = m_impl->index.size();
}

std::vector<SymbolEntry> SymbolLocator::Search(const std::string& query, size_t maxResults) const {
    std::vector<SymbolEntry> out;
    for (auto e : m_impl->index) {
        e.score = fuzzyScore(query, e.name);
        if (e.score > 0.0f) out.push_back(e);
    }
    std::sort(out.begin(), out.end(),
        [](const SymbolEntry& a, const SymbolEntry& b){ return a.score > b.score; });
    if (out.size() > maxResults) out.resize(maxResults);
    return out;
}

SymbolEntry SymbolLocator::GoToDefinition(const std::string& name) const {
    // Prefer exact name match, then class/function priority
    SymbolEntry best;
    float bestScore = -1;
    for (const auto& e : m_impl->index) {
        float s = (e.name == name) ? 1.0f : fuzzyScore(name, e.name);
        if (s > bestScore) { bestScore = s; best = e; }
    }
    return best;
}

std::vector<SymbolEntry> SymbolLocator::SymbolsInFile(const std::string& filePath) const {
    std::vector<SymbolEntry> out;
    for (const auto& e : m_impl->index) if (e.filePath==filePath) out.push_back(e);
    return out;
}

std::vector<SymbolEntry> SymbolLocator::ByKind(SymbolKind kind) const {
    std::vector<SymbolEntry> out;
    for (const auto& e : m_impl->index) if (e.kind==kind) out.push_back(e);
    return out;
}

} // namespace Tools
