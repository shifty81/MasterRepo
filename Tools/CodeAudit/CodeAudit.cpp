#include "Tools/CodeAudit/CodeAudit.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;
namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

CodeAudit::CodeAudit(CodeAuditConfig cfg)
    : m_cfg(std::move(cfg))
{}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void CodeAudit::Scan()
{
    m_repos.clear();
    m_symbolIndex.clear();

    // Scan MasterRepo first
    if (!m_cfg.masterRepoRoot.empty())
        m_repos.push_back(ScanRepo("MasterRepo", m_cfg.masterRepoRoot, true));

    // Scan each external repo
    for (const auto& [label, root] : m_cfg.repoPaths)
        m_repos.push_back(ScanRepo(label, root, false));
}

void CodeAudit::Analyse()
{
    m_findings.clear();
    DetectDuplicates();

    for (const auto& repo : m_repos) {
        if (repo.label == "MasterRepo") continue;
        DetectMigrationCandidates(repo);
    }
}

void CodeAudit::Run()
{
    Scan();
    Analyse();
}

const std::vector<RepoSummary>& CodeAudit::GetRepoSummaries() const
{
    return m_repos;
}

const std::vector<AuditEntry>& CodeAudit::GetFindings() const
{
    return m_findings;
}

std::vector<AuditEntry> CodeAudit::GetFindingsByType(AuditFinding type) const
{
    std::vector<AuditEntry> result;
    for (const auto& f : m_findings)
        if (f.finding == type) result.push_back(f);
    return result;
}

std::vector<AuditEntry> CodeAudit::GetMigrationCandidates() const
{
    return GetFindingsByType(AuditFinding::MigrationCandidate);
}

// ─────────────────────────────────────────────────────────────────────────────
// Scanning
// ─────────────────────────────────────────────────────────────────────────────

bool CodeAudit::IsExcluded(const std::string& rel) const
{
    for (const auto& pat : m_cfg.excludePatterns)
        if (rel.find(pat) != std::string::npos) return true;
    return false;
}

RepoSummary CodeAudit::ScanRepo(const std::string& label,
                                 const std::string& root,
                                 bool /*isMaster*/)
{
    RepoSummary summary;
    summary.label    = label;
    summary.rootPath = root;

    if (!fs::exists(root)) return summary;

    auto it_end = fs::end(fs::recursive_directory_iterator(root));
    auto it     = fs::begin(fs::recursive_directory_iterator(root));

    for (; it != it_end; ++it) {
        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        bool ok = false;
        for (const auto& e : m_cfg.extensions)
            if (e == ext) { ok = true; break; }
        if (!ok) continue;

        std::string rel = fs::relative(entry.path(), root).string();
        if (IsExcluded(rel)) continue;

        ++summary.fileCount;
        ScanFile(entry.path().string(), rel, summary);
    }

    return summary;
}

void CodeAudit::ScanFile(const std::string& filePath,
                          const std::string& relPath,
                          RepoSummary& repo)
{
    std::ifstream ifs(filePath);
    if (!ifs) return;

    std::string src{std::istreambuf_iterator<char>(ifs),
                    std::istreambuf_iterator<char>{}};
    int lineCount = 0;
    for (char c : src) if (c == '\n') ++lineCount;
    repo.lineCount += lineCount;

    // Namespace extraction — build a simple "last seen namespace" map
    static const std::regex reNs(R"(namespace\s+(\w+)\s*\{)");
    std::string currentNs;
    for (std::sregex_iterator it(src.begin(), src.end(), reNs), end; it != end; ++it) {
        std::string ns = (*it)[1].str();
        if (std::find(repo.namespaces.begin(), repo.namespaces.end(), ns) == repo.namespaces.end())
            repo.namespaces.push_back(ns);
        currentNs = ns; // use last namespace found in file as default
    }

    // Class/struct extraction
    static const std::regex reCls(R"((?:class|struct)\s+(\w+))");
    int ln = 1;
    std::istringstream iss(src);
    std::string line;
    while (std::getline(iss, line)) {
        // Track namespace declarations per line
        std::smatch nsMatch;
        if (std::regex_search(line, nsMatch, reNs))
            currentNs = nsMatch[1].str();

        std::smatch m;
        if (std::regex_search(line, m, reCls)) {
            SymbolInfo sym;
            sym.name          = m[1].str();
            sym.file          = relPath;
            sym.line          = ln;
            sym.signature     = "class/struct";
            sym.namespaceName = currentNs.empty() ? repo.label : currentNs;
            repo.symbols.push_back(sym);
            ++repo.classCount;

            // Add to global index; store repoLabel in namespaceName when no
            // C++ namespace is known so DetectDuplicates can group by repo.
            SymbolInfo idxSym = sym;
            idxSym.namespaceName = repo.label; // use repo label for cross-repo grouping
            m_symbolIndex[sym.name].push_back(std::move(idxSym));
        }

        // Simple free-function detection: lines matching ReturnType FuncName(
        static const std::regex reFn(
            R"(^\s*(?:static\s+|inline\s+|virtual\s+)?[\w:<>*& ]+\s+(\w+)\s*\()");
        if (std::regex_search(line, m, reFn)) {
            std::string fname = m[1].str();
            // Skip keywords
            static const std::vector<std::string> kw = {
                "if","for","while","switch","return","new","delete","sizeof",
                "class","struct","namespace","using","typedef","template"
            };
            if (std::find(kw.begin(), kw.end(), fname) == kw.end()) {
                SymbolInfo sym;
                sym.name      = fname;
                sym.file      = relPath;
                sym.line      = ln;
                sym.signature = "function";
                repo.symbols.push_back(sym);
                ++repo.functionCount;
                SymbolInfo idxSym = sym;
                idxSym.namespaceName = repo.label; // repo label for cross-repo grouping
                m_symbolIndex[fname].push_back(std::move(idxSym));
            }
        }
        ++ln;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Analysis
// ─────────────────────────────────────────────────────────────────────────────

void CodeAudit::DetectDuplicates()
{
    for (const auto& [name, occurrences] : m_symbolIndex) {
        if (occurrences.size() < 2) continue;

        // Check if the symbol appears in more than one repo label
        std::vector<std::string> repos;
        for (const auto& sym : occurrences)
            if (std::find(repos.begin(), repos.end(), sym.namespaceName) == repos.end())
                repos.push_back(sym.namespaceName);

        if (repos.size() < 2) continue;

        AuditEntry entry;
        entry.finding     = AuditFinding::DuplicateSymbol;
        entry.description = "Symbol '" + name + "' exists in " +
                            std::to_string(repos.size()) + " repos: ";
        for (size_t i = 0; i < repos.size(); ++i) {
            if (i) entry.description += ", ";
            entry.description += repos[i];
        }
        entry.symbols = occurrences;
        entry.suggestedAction = "Review and merge or alias. Keep MasterRepo copy, "
                                "archive external.";
        m_findings.push_back(std::move(entry));
    }
}

void CodeAudit::DetectMigrationCandidates(const RepoSummary& external)
{
    // A symbol is a migration candidate if it does NOT appear in MasterRepo
    const RepoSummary* master = nullptr;
    for (const auto& r : m_repos)
        if (r.label == "MasterRepo") { master = &r; break; }

    for (const auto& sym : external.symbols) {
        // Does MasterRepo already have this symbol?
        bool inMaster = false;
        auto it = m_symbolIndex.find(sym.name);
        if (it != m_symbolIndex.end()) {
            for (const auto& s : it->second)
                if (s.namespaceName == "MasterRepo") { inMaster = true; break; }
        }
        if (inMaster) continue;

        // Skip trivial names
        if (sym.name.size() < 4) continue;
        static const std::vector<std::string> skip = {
            "main","Init","Update","Render","Draw","Get","Set","Is","Has","On"
        };
        if (std::find(skip.begin(), skip.end(), sym.name) != skip.end()) continue;

        AuditEntry entry;
        entry.finding      = AuditFinding::MigrationCandidate;
        entry.sourceRepo   = external.label;
        entry.description  = "'" + sym.name + "' in " + external.label +
                             " has no equivalent in MasterRepo";
        entry.symbols      = {sym};
        entry.targetPath   = "Archive/" + external.label + "/" + sym.file;
        entry.suggestedAction = "Consider integrating '" + sym.name +
                               "' into the appropriate MasterRepo subsystem.";
        m_findings.push_back(std::move(entry));

        if (m_cfg.onMigrationCandidate)
            m_cfg.onMigrationCandidate(entry);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Report
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::string CodeAudit::RenderSummaryTable(const std::vector<RepoSummary>& repos)
{
    std::ostringstream oss;
    oss << "| Repo | Files | Classes | Functions | Lines |\n"
        << "|------|-------|---------|-----------|-------|\n";
    for (const auto& r : repos)
        oss << "| " << r.label << " | " << r.fileCount
            << " | " << r.classCount << " | " << r.functionCount
            << " | " << r.lineCount << " |\n";
    return oss.str();
}

/* static */
std::string CodeAudit::RenderFindingsTable(const std::vector<AuditEntry>& findings)
{
    std::ostringstream oss;
    oss << "| # | Type | Description | Action |\n"
        << "|---|------|-------------|--------|\n";
    int n = 1;
    for (const auto& f : findings) {
        std::string type;
        switch (f.finding) {
            case AuditFinding::DuplicateSymbol:    type = "Duplicate";  break;
            case AuditFinding::ConflictingAPI:     type = "Conflict";   break;
            case AuditFinding::ReusablePattern:    type = "Reusable";   break;
            case AuditFinding::NamespaceCollision: type = "NS Collision"; break;
            case AuditFinding::UnusedFile:         type = "Unused";     break;
            case AuditFinding::MigrationCandidate: type = "Migrate";    break;
        }
        oss << "| " << n++ << " | " << type << " | "
            << f.description << " | " << f.suggestedAction << " |\n";
    }
    return oss.str();
}

std::string CodeAudit::BuildReport() const
{
    std::ostringstream oss;
    oss << "# MasterRepo — Multi-Repo Audit Report\n\n"
        << "*Auto-generated by Atlas CodeAudit (Phase 11)*\n\n"
        << "---\n\n"
        << "## Repository Summaries\n\n"
        << RenderSummaryTable(m_repos) << "\n"
        << "## Findings\n\n"
        << RenderFindingsTable(m_findings) << "\n"
        << "---\n"
        << "*Total files: " << TotalFiles()
        << " | Total symbols: " << TotalSymbols()
        << " | Duplicates: " << DuplicateCount() << "*\n";
    return oss.str();
}

bool CodeAudit::WriteReport() const
{
    if (m_cfg.reportPath.empty()) return false;
    try {
        fs::create_directories(fs::path(m_cfg.reportPath).parent_path());
        std::ofstream ofs(m_cfg.reportPath);
        if (!ofs) return false;
        ofs << BuildReport();
        return true;
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

int CodeAudit::TotalFiles() const
{
    int n = 0;
    for (const auto& r : m_repos) n += r.fileCount;
    return n;
}

int CodeAudit::TotalSymbols() const
{
    int n = 0;
    for (const auto& r : m_repos) n += static_cast<int>(r.symbols.size());
    return n;
}

int CodeAudit::DuplicateCount() const
{
    int n = 0;
    for (const auto& f : m_findings)
        if (f.finding == AuditFinding::DuplicateSymbol) ++n;
    return n;
}

} // namespace Tools
