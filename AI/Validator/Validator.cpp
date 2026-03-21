#include "AI/Validator/Validator.h"

#include <algorithm>
#include <array>
#include <chrono>
#ifdef _WIN32
#  include <stdio.h>   // provides _popen / _pclose in global namespace under MSVC
#  define popen  _popen
#  define pclose _pclose
#else
#  include <cstdio>
#endif
#include <ctime>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;
namespace AI {

// ─────────────────────────────────────────────────────────────────────────────
// ValidationReport helpers
// ─────────────────────────────────────────────────────────────────────────────

CheckStatus ValidationReport::OverallStatus() const
{
    CheckStatus worst = CheckStatus::Pass;
    for (const auto& c : checks) {
        if (c.status == CheckStatus::Fail) return CheckStatus::Fail;
        if (c.status == CheckStatus::Warn && worst != CheckStatus::Fail)
            worst = CheckStatus::Warn;
    }
    return worst;
}

int ValidationReport::FailCount() const
{
    int n = 0;
    for (const auto& c : checks)
        if (c.status == CheckStatus::Fail) ++n;
    return n;
}

int ValidationReport::WarnCount() const
{
    int n = 0;
    for (const auto& c : checks)
        if (c.status == CheckStatus::Warn) ++n;
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

Validator::Validator(ValidatorConfig cfg)
    : m_cfg(std::move(cfg))
{}

void Validator::SetProgressCallback(ProgressFn fn)
{
    m_onProgress = std::move(fn);
}

void Validator::NotifyProgress(const std::string& step, float pct)
{
    if (m_onProgress) m_onProgress(step, pct);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal: run shell command and capture output
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::string Validator::RunCommand(const std::string& cmd, int& exitCode,
                                   float& durationSecs)
{
    auto t0 = std::chrono::steady_clock::now();
    std::string output;

    // popen for cross-platform capture
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) { exitCode = -1; return "popen failed"; }

    char buf[256];
    while (fgets(buf, sizeof(buf), pipe))
        output += buf;

    exitCode = pclose(pipe);
    auto t1 = std::chrono::steady_clock::now();
    durationSecs = std::chrono::duration<float>(t1 - t0).count();
    return output;
}

/* static */
std::vector<std::string> Validator::ExtractIssues(const std::string& output)
{
    // Extract lines that look like errors or warnings
    static const std::regex reIssue(
        R"((?:error|warning|note):\s*.+)", std::regex::icase);
    std::vector<std::string> issues;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (std::regex_search(line, reIssue))
            issues.push_back(line);
    }
    // Limit to first 50
    if (issues.size() > 50) issues.resize(50);
    return issues;
}

/* static */
DiffSummary Validator::ParseGitDiff(const std::string& diffOutput,
                                     const std::string& fromRef,
                                     const std::string& toRef)
{
    DiffSummary summary;
    summary.fromRef = fromRef;
    summary.toRef   = toRef;

    std::istringstream iss(diffOutput);
    std::string line;
    FileDiff currentFile;
    bool inFile = false;

    static const std::regex reFile(R"(^diff --git a/(.+) b/)");
    static const std::regex reAdd(R"(^\+[^\+])");
    static const std::regex reDel(R"(^-[^-])");

    while (std::getline(iss, line)) {
        std::smatch m;
        if (std::regex_search(line, m, reFile)) {
            if (inFile) {
                summary.files.push_back(currentFile);
                summary.totalAdded   += currentFile.linesAdded;
                summary.totalRemoved += currentFile.linesRemoved;
            }
            currentFile = FileDiff{};
            currentFile.path = m[1].str();
            // Infer language from file extension
            auto ext = fs::path(currentFile.path).extension().string();
            static const std::unordered_map<std::string,std::string> kLangMap = {
                {".cpp","C++"},{".cxx","C++"},{".cc","C++"},{".c","C"},
                {".h","C++"},{".hpp","C++"},{".hxx","C++"},
                {".py","Python"},{".lua","Lua"},{".sh","Shell"},
                {".json","JSON"},{".md","Markdown"}
            };
            auto langIt = kLangMap.find(ext);
            if (langIt != kLangMap.end())
                currentFile.language = langIt->second;
            else if (currentFile.path.find("CMakeLists") != std::string::npos ||
                     ext == ".cmake")
                currentFile.language = "CMake";
            else
                currentFile.language = ext.empty() ? "?" : ext.substr(1);
            inFile = true;
        } else if (inFile) {
            if (std::regex_match(line, reAdd)) ++currentFile.linesAdded;
            else if (std::regex_match(line, reDel)) ++currentFile.linesRemoved;
        }
    }
    if (inFile) {
        summary.files.push_back(currentFile);
        summary.totalAdded   += currentFile.linesAdded;
        summary.totalRemoved += currentFile.linesRemoved;
    }
    return summary;
}

// ─────────────────────────────────────────────────────────────────────────────
// Individual checks
// ─────────────────────────────────────────────────────────────────────────────

CheckResult Validator::CheckBuild(const std::string& buildType)
{
    CheckResult r;
    r.name = "Build (" + buildType + ")";

    std::string buildDir = m_cfg.buildDir.empty()
                           ? "/tmp/atlas_validator_build"
                           : m_cfg.buildDir;

    std::string configCmd =
        "cmake -B \"" + buildDir + "\" -DCMAKE_BUILD_TYPE=" + buildType +
        " \"" + m_cfg.repoRoot + "\"";
    std::string buildCmd  =
        "cmake --build \"" + buildDir + "\" -- -j" +
        std::to_string(m_cfg.buildJobs);

    int ec1 = 0; float dur1 = 0.f;
    std::string out1 = RunCommand(configCmd, ec1, dur1);

    if (ec1 != 0) {
        r.status  = CheckStatus::Fail;
        r.summary = "CMake configure failed";
        r.rawOutput = out1;
        r.exitCode  = ec1;
        r.issues    = ExtractIssues(out1);
        return r;
    }

    int ec2 = 0; float dur2 = 0.f;
    std::string out2 = RunCommand(buildCmd, ec2, dur2);

    r.exitCode    = ec2;
    r.rawOutput   = out1 + out2;
    r.durationSecs = dur1 + dur2;
    r.issues      = ExtractIssues(out2);

    if (ec2 == 0) {
        r.status  = r.issues.empty() ? CheckStatus::Pass : CheckStatus::Warn;
        r.summary = "Build succeeded" +
                    std::string(r.issues.empty() ? "" : " with warnings");
    } else {
        r.status  = CheckStatus::Fail;
        r.summary = "Build failed (" + std::to_string(r.issues.size()) + " errors)";
    }
    return r;
}

CheckResult Validator::CheckTests()
{
    CheckResult r;
    r.name = "Tests (CTest)";

    std::string buildDir = m_cfg.buildDir.empty()
                           ? "/tmp/atlas_validator_build"
                           : m_cfg.buildDir;

    // Only attempt if CTestTestfile.cmake exists
    if (!fs::exists(buildDir + "/CTestTestfile.cmake")) {
        r.status  = CheckStatus::Skip;
        r.summary = "No CTestTestfile.cmake found — tests not configured";
        return r;
    }

    std::string cmd = "ctest --test-dir \"" + buildDir +
                      "\" --output-on-failure -j" + std::to_string(m_cfg.buildJobs);
    int ec = 0; float dur = 0.f;
    r.rawOutput   = RunCommand(cmd, ec, dur);
    r.exitCode    = ec;
    r.durationSecs = dur;
    r.issues      = ExtractIssues(r.rawOutput);

    if (ec == 0) {
        r.status  = CheckStatus::Pass;
        r.summary = "All tests passed";
    } else {
        r.status  = CheckStatus::Fail;
        r.summary = "Some tests failed";
    }
    return r;
}

CheckResult Validator::CheckLint()
{
    CheckResult r;
    r.name = "Static Analysis (cppcheck)";

    // Check if cppcheck is available
    int ec0 = 0; float d0 = 0.f;
    RunCommand("cppcheck --version", ec0, d0);
    if (ec0 != 0) {
        r.status  = CheckStatus::Skip;
        r.summary = "cppcheck not found";
        return r;
    }

    std::string dirs;
    for (const char* d : {"Core","Engine","Runtime","Editor","AI","Tools"}) {
        std::string p = m_cfg.repoRoot + "/" + d;
        if (fs::exists(p)) dirs += " \"" + p + "\"";
    }

    std::string cmd =
        "cppcheck --enable=warning,performance --std=c++20 "
        "--suppress=missingIncludeSystem " + dirs;

    int ec = 0; float dur = 0.f;
    r.rawOutput   = RunCommand(cmd, ec, dur);
    r.exitCode    = ec;
    r.durationSecs = dur;
    r.issues      = ExtractIssues(r.rawOutput);

    if (r.issues.empty()) {
        r.status  = CheckStatus::Pass;
        r.summary = "No issues found";
    } else {
        r.status  = CheckStatus::Warn;
        r.summary = std::to_string(r.issues.size()) + " issue(s) found";
    }
    return r;
}

CheckResult Validator::CheckDiff(const std::string& baseRef)
{
    CheckResult r;
    r.name = "Diff Analysis (git)";

    std::string cmd = "git -C \"" + m_cfg.repoRoot +
                      "\" diff --stat " + baseRef;
    int ec = 0; float dur = 0.f;
    r.rawOutput   = RunCommand(cmd, ec, dur);
    r.exitCode    = ec;
    r.durationSecs = dur;

    if (ec != 0) {
        r.status  = CheckStatus::Fail;
        r.summary = "git diff failed — is this a git repo?";
        return r;
    }

    // Full diff for file-level analysis
    std::string fullDiffCmd = "git -C \"" + m_cfg.repoRoot +
                              "\" diff " + baseRef;
    int ec2 = 0; float dur2 = 0.f;
    std::string fullDiff = RunCommand(fullDiffCmd, ec2, dur2);

    // Parse embedded in rawOutput; analysis returned via ValidationReport.diff
    r.status  = CheckStatus::Pass;
    r.summary = "Diff parsed successfully";
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Main run pipeline
// ─────────────────────────────────────────────────────────────────────────────

ValidationReport Validator::Run()
{
    ValidationReport report;
    report.repoRoot = m_cfg.repoRoot;

    // Timestamp
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    report.timestamp = buf;
    report.title     = "Atlas Engine — Validation Report";

    float pct = 0.f;
    float step = 1.f / (m_cfg.runBuild + m_cfg.runTests + m_cfg.runLint +
                        m_cfg.runDiffScan + 1);

    if (m_cfg.runBuild) {
        NotifyProgress("Build", pct);
        auto r = CheckBuild(m_cfg.buildType);
        report.checks.push_back(r);
        pct += step;
        if (m_cfg.failFast && r.status == CheckStatus::Fail) goto done;
    }

    if (m_cfg.runTests) {
        NotifyProgress("Tests", pct);
        report.checks.push_back(CheckTests());
        pct += step;
        if (m_cfg.failFast &&
            report.checks.back().status == CheckStatus::Fail) goto done;
    }

    if (m_cfg.runLint) {
        NotifyProgress("Lint", pct);
        report.checks.push_back(CheckLint());
        pct += step;
    }

    if (m_cfg.runDiffScan) {
        NotifyProgress("Diff", pct);
        auto r = CheckDiff(m_cfg.baseRef);
        report.checks.push_back(r);

        // Parse diff into report.diff
        std::string diffCmd = "git -C \"" + m_cfg.repoRoot +
                              "\" diff " + m_cfg.baseRef;
        int ec = 0; float dur = 0.f;
        std::string diffOut = RunCommand(diffCmd, ec, dur);
        report.diff = ParseGitDiff(diffOut, m_cfg.baseRef, "HEAD");
        pct += step;
    }

done:
    // AI summary
    if (m_cfg.llmSummariseFn) {
        NotifyProgress("AI Summary", pct);
        report.aiSummary = m_cfg.llmSummariseFn(RenderMarkdown(report));
    }

    NotifyProgress("Done", 1.0f);

    if (!m_cfg.reportPath.empty())
        WriteReport(report);

    return report;
}

CheckResult Validator::RunCheck(const std::string& name)
{
    if (name == "build")  return CheckBuild(m_cfg.buildType);
    if (name == "tests")  return CheckTests();
    if (name == "lint")   return CheckLint();
    if (name == "diff")   return CheckDiff(m_cfg.baseRef);
    CheckResult r;
    r.name   = name;
    r.status = CheckStatus::Skip;
    r.summary = "Unknown check: " + name;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Report rendering
// ─────────────────────────────────────────────────────────────────────────────

static const char* StatusEmoji(CheckStatus s)
{
    switch (s) {
        case CheckStatus::Pass: return "✅";
        case CheckStatus::Warn: return "⚠️";
        case CheckStatus::Fail: return "❌";
        case CheckStatus::Skip: return "⏭";
    }
    return "?";
}

std::string Validator::RenderMarkdown(const ValidationReport& report) const
{
    std::ostringstream oss;
    oss << "# " << report.title << "\n\n"
        << "**Date:** " << report.timestamp << "\n"
        << "**Repo:** `" << report.repoRoot << "`\n\n";

    // Overall status
    auto overall = report.OverallStatus();
    oss << "## Overall: " << StatusEmoji(overall) << " "
        << (overall == CheckStatus::Pass ? "PASS" :
            overall == CheckStatus::Warn ? "WARN" :
            overall == CheckStatus::Fail ? "FAIL" : "SKIP") << "\n\n";

    // Checks table
    oss << "## Checks\n\n"
        << "| Check | Status | Summary | Duration |\n"
        << "|-------|--------|---------|----------|\n";
    for (const auto& c : report.checks) {
        oss << "| " << c.name << " | " << StatusEmoji(c.status) << " | "
            << c.summary << " | " << (int)c.durationSecs << "s |\n";
    }

    // Diff summary
    if (!report.diff.files.empty()) {
        oss << "\n## Diff Summary\n\n"
            << "Base: `" << report.diff.fromRef << "` → `" << report.diff.toRef << "`\n\n"
            << "+" << report.diff.totalAdded << " / -" << report.diff.totalRemoved
            << " lines across " << report.diff.files.size() << " file(s)\n\n"
            << "| File | Language | +Added | -Removed |\n"
            << "|------|----------|--------|----------|\n";
        for (const auto& f : report.diff.files) {
            oss << "| `" << f.path << "` | " << f.language
                << " | " << f.linesAdded << " | " << f.linesRemoved << " |\n";
        }
    }

    // AI summary
    if (!report.aiSummary.empty()) {
        oss << "\n## AI Summary\n\n" << report.aiSummary << "\n";
    }

    // Issue details
    for (const auto& c : report.checks) {
        if (!c.issues.empty()) {
            oss << "\n### " << c.name << " — Issues\n\n```\n";
            for (const auto& i : c.issues) oss << i << "\n";
            oss << "```\n";
        }
    }

    return oss.str();
}

bool Validator::WriteReport(const ValidationReport& report) const
{
    try {
        fs::create_directories(fs::path(m_cfg.reportPath).parent_path());
        std::ofstream ofs(m_cfg.reportPath);
        if (!ofs) return false;
        ofs << RenderMarkdown(report);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace AI
