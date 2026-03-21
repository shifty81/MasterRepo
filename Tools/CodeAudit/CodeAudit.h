#pragma once
// CodeAudit — Phase 11: Multi-Repo Reverse Engineering & Asset Extraction
//
// Analyses a directory tree (local repo clone or extracted archive) for
// reusable patterns, reports duplicates / merge conflicts, and suggests how
// code can be integrated into MasterRepo.
//
// All work is purely static (file scanning + regex analysis) so it runs
// fully offline without any compiler or runtime dependency.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Data model
// ─────────────────────────────────────────────────────────────────────────────

enum class AuditFinding {
    DuplicateSymbol,     // same class/function name in two repos
    ConflictingAPI,      // same name but different signature
    ReusablePattern,     // useful utility with no equivalent in MasterRepo
    NamespaceCollision,  // different namespace for same concept
    UnusedFile,          // file has no inbound include references
    MigrationCandidate,  // strong candidate for import into MasterRepo
};

struct SymbolInfo {
    std::string name;        // class / function name
    std::string signature;   // type signature
    std::string file;        // relative file path
    int         line = 0;
    std::string namespaceName;
};

struct AuditEntry {
    AuditFinding        finding;
    std::string         description;
    std::vector<SymbolInfo> symbols;     // related symbols
    std::string         sourceRepo;      // originating repo label
    std::string         targetPath;      // suggested MasterRepo path
    std::string         suggestedAction; // plain-language recommendation
};

struct RepoSummary {
    std::string         label;           // e.g. "AtlasForge-EveOffline"
    std::string         rootPath;
    int                 fileCount    = 0;
    int                 classCount   = 0;
    int                 functionCount = 0;
    int                 lineCount    = 0;
    std::vector<std::string> namespaces;
    std::vector<SymbolInfo>  symbols;
};

// ─────────────────────────────────────────────────────────────────────────────
// CodeAuditConfig
// ─────────────────────────────────────────────────────────────────────────────

struct CodeAuditConfig {
    // Root of MasterRepo (used to detect what is already integrated)
    std::string masterRepoRoot;
    // External repos to audit — label → path
    std::unordered_map<std::string, std::string> repoPaths;
    // File extensions to include (default: .h .hpp .cpp .cxx)
    std::vector<std::string> extensions = {".h", ".hpp", ".cpp", ".cxx"};
    // Paths / patterns to skip
    std::vector<std::string> excludePatterns = {"Archive/", "External/", "Build"};
    // Output report path (Markdown)
    std::string reportPath = "Docs/API/AUDIT_REPORT.md";
    // Optional: called when a migration candidate is found
    std::function<void(const AuditEntry&)> onMigrationCandidate;
};

// ─────────────────────────────────────────────────────────────────────────────
// CodeAudit
// ─────────────────────────────────────────────────────────────────────────────

class CodeAudit {
public:
    explicit CodeAudit(CodeAuditConfig cfg);

    // ── Analysis ─────────────────────────────────────────────────────────────

    /// Scan all configured repos and MasterRepo.  Populates internal model.
    void Scan();

    /// Run duplicate / conflict / pattern detection after Scan().
    void Analyse();

    /// Scan + Analyse in one call.
    void Run();

    // ── Results ──────────────────────────────────────────────────────────────

    const std::vector<RepoSummary>& GetRepoSummaries() const;
    const std::vector<AuditEntry>&  GetFindings()      const;

    /// Filter findings by type.
    std::vector<AuditEntry> GetFindingsByType(AuditFinding type) const;

    /// Return all migration candidates sorted by score (highest first).
    std::vector<AuditEntry> GetMigrationCandidates() const;

    // ── Report ────────────────────────────────────────────────────────────────

    /// Write a Markdown report to cfg.reportPath.
    bool WriteReport() const;

    /// Return the report as a string (without writing to disk).
    std::string BuildReport() const;

    // ── Statistics ────────────────────────────────────────────────────────────

    int TotalFiles()     const;
    int TotalSymbols()   const;
    int DuplicateCount() const;

private:
    CodeAuditConfig            m_cfg;
    std::vector<RepoSummary>   m_repos;
    std::vector<AuditEntry>    m_findings;

    // Index: symbol name → all occurrences across repos
    std::unordered_map<std::string, std::vector<SymbolInfo>> m_symbolIndex;

    RepoSummary ScanRepo(const std::string& label,
                         const std::string& root,
                         bool isMaster);
    void        ScanFile(const std::string& filePath,
                         const std::string& relPath,
                         RepoSummary& repo);
    void        DetectDuplicates();
    void        DetectMigrationCandidates(const RepoSummary& external);
    bool        IsExcluded(const std::string& rel) const;

    static std::string RenderSummaryTable(const std::vector<RepoSummary>&);
    static std::string RenderFindingsTable(const std::vector<AuditEntry>&);
};

} // namespace Tools
