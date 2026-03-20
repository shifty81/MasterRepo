#include "Tools/GitPanel/GitPanel.h"
#include <algorithm>
#include <cstdio>
#include <sstream>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

GitPanel::GitPanel(const std::string& repoRoot)
    : m_repoRoot(repoRoot) {}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string GitPanel::buildCmd(const std::vector<std::string>& args) const {
    std::string cmd = "git -C \"" + m_repoRoot + "\"";
    for (const auto& a : args) {
        cmd += " ";
        // Quote args that contain spaces
        if (a.find(' ') != std::string::npos)
            cmd += "\"" + a + "\"";
        else
            cmd += a;
    }
    return cmd;
}

std::string GitPanel::RunGit(const std::vector<std::string>& args) const {
    std::string cmd = buildCmd(args) + " 2>&1";
#if defined(_WIN32)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return {};
    std::string output;
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe))
        output += buf;
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

GitFileStatus GitPanel::parseStatusLine(const std::string& line) const {
    GitFileStatus s;
    if (line.size() < 3) return s;
    s.indexStatus    = std::string(1, line[0]);
    s.workTreeStatus = std::string(1, line[1]);
    s.path           = line.substr(3);
    // Handle rename: old -> new
    auto arrow = s.path.find(" -> ");
    if (arrow != std::string::npos)
        s.path = s.path.substr(arrow + 4);
    return s;
}

GitCommit GitPanel::parseLogLine(const std::string& line) const {
    GitCommit c;
    // Expected format: hash|shortHash|author|date|subject
    auto split = [&](const std::string& s, char delim) -> std::vector<std::string> {
        std::vector<std::string> parts;
        std::istringstream ss(s);
        std::string token;
        while (std::getline(ss, token, delim))
            parts.push_back(token);
        return parts;
    };

    auto parts = split(line, '|');
    if (parts.size() >= 5) {
        c.hash      = parts[0];
        c.shortHash = parts[1];
        c.author    = parts[2];
        c.date      = parts[3];
        c.message   = parts[4];
        // Re-join if subject had '|' in it
        for (size_t i = 5; i < parts.size(); ++i)
            c.message += "|" + parts[i];
    }
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// Status
// ─────────────────────────────────────────────────────────────────────────────

std::vector<GitFileStatus> GitPanel::GetStatus() const {
    std::string out = RunGit({"status", "--porcelain"});
    std::vector<GitFileStatus> results;
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.size() >= 3)
            results.push_back(parseStatusLine(line));
    }
    return results;
}

bool GitPanel::HasUncommittedChanges() const {
    std::string out = RunGit({"status", "--porcelain"});
    return !out.empty();
}

std::string GitPanel::GetCurrentBranch() const {
    std::string out = RunGit({"rev-parse", "--abbrev-ref", "HEAD"});
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Diff
// ─────────────────────────────────────────────────────────────────────────────

std::string GitPanel::GetDiff(const std::string& file) const {
    if (file.empty()) return RunGit({"diff"});
    return RunGit({"diff", "--", file});
}

std::string GitPanel::GetStagedDiff() const {
    return RunGit({"diff", "--cached"});
}

// ─────────────────────────────────────────────────────────────────────────────
// Staging
// ─────────────────────────────────────────────────────────────────────────────

bool GitPanel::StageFile(const std::string& path) {
    std::string out = RunGit({"add", "--", path});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

bool GitPanel::StageAll() {
    std::string out = RunGit({"add", "-A"});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

bool GitPanel::UnstageFile(const std::string& path) {
    std::string out = RunGit({"reset", "HEAD", "--", path});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

// ─────────────────────────────────────────────────────────────────────────────
// Commit
// ─────────────────────────────────────────────────────────────────────────────

bool GitPanel::Commit(const std::string& message) {
    std::string out = RunGit({"commit", "-m", message});
    return out.find("error") == std::string::npos &&
           out.find("nothing to commit") == std::string::npos;
}

bool GitPanel::AmendCommit(const std::string& message) {
    std::string out = RunGit({"commit", "--amend", "-m", message});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

// ─────────────────────────────────────────────────────────────────────────────
// Branches
// ─────────────────────────────────────────────────────────────────────────────

std::vector<GitBranch> GitPanel::GetBranches() const {
    // %(refname:short)|%(HEAD)|%(upstream:short)
    std::string out = RunGit({"branch", "-a",
                               "--format=%(refname:short)|%(HEAD)|%(upstream:short)"});
    std::vector<GitBranch> branches;
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
        if (line.empty()) continue;
        GitBranch b;
        auto p1 = line.find('|');
        if (p1 == std::string::npos) { b.name = line; branches.push_back(b); continue; }
        b.name = line.substr(0, p1);
        auto p2 = line.find('|', p1 + 1);
        if (p2 != std::string::npos) {
            b.isCurrent     = (line[p1 + 1] == '*');
            b.trackingBranch = line.substr(p2 + 1);
        }
        branches.push_back(b);
    }
    return branches;
}

bool GitPanel::CreateBranch(const std::string& name) {
    std::string out = RunGit({"checkout", "-b", name});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

bool GitPanel::CheckoutBranch(const std::string& name) {
    std::string out = RunGit({"checkout", name});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

bool GitPanel::DeleteBranch(const std::string& name, bool force) {
    std::string flag = force ? "-D" : "-d";
    std::string out = RunGit({"branch", flag, name});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

// ─────────────────────────────────────────────────────────────────────────────
// Log
// ─────────────────────────────────────────────────────────────────────────────

std::vector<GitCommit> GitPanel::GetLog(int maxCount) const {
    std::string n = std::to_string(maxCount);
    std::string out = RunGit({"log", "--format=%H|%h|%an|%ai|%s", "-n", n});
    std::vector<GitCommit> commits;
    std::istringstream ss(out);
    std::string line;
    while (std::getline(ss, line)) {
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
        if (!line.empty())
            commits.push_back(parseLogLine(line));
    }
    return commits;
}

// ─────────────────────────────────────────────────────────────────────────────
// Remote
// ─────────────────────────────────────────────────────────────────────────────

bool GitPanel::Fetch() {
    std::string out = RunGit({"fetch", "--all"});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

bool GitPanel::Pull() {
    std::string out = RunGit({"pull"});
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

bool GitPanel::Push(const std::string& remote, const std::string& branch) {
    std::vector<std::string> args = {"push", remote};
    if (!branch.empty()) args.push_back(branch);
    std::string out = RunGit(args);
    return out.find("error") == std::string::npos &&
           out.find("fatal") == std::string::npos;
}

} // namespace Tools
