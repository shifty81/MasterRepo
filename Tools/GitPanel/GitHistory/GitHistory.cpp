#include "Tools/GitPanel/GitHistory/GitHistory.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace Tools {

// ── popen helper ─────────────────────────────────────────────────────────
static std::string runCommand(const std::string& cmd) {
    std::string output;
#if defined(_WIN32)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return {};
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
#if defined(_WIN32)
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    return output;
}

// ── Simple record parsing ─────────────────────────────────────────────────
static std::vector<CommitRecord> parseLog(const std::string& raw) {
    std::vector<CommitRecord> records;
    std::istringstream ss(raw);
    std::string line;
    CommitRecord cur;
    bool inBody = false;

    auto flush = [&]() {
        if (!cur.sha.empty()) {
            if (cur.shortSha.empty() && cur.sha.size() >= 8)
                cur.shortSha = cur.sha.substr(0, 8);
            records.push_back(cur);
            cur = CommitRecord{};
            inBody = false;
        }
    };

    while (std::getline(ss, line)) {
        if (line.rfind("COMMIT:", 0) == 0) {
            flush();
            cur.sha = line.substr(7);
            if (cur.sha.size() >= 8) cur.shortSha = cur.sha.substr(0, 8);
        } else if (line.rfind("AUTHOR:", 0) == 0) {
            cur.author = line.substr(7);
        } else if (line.rfind("EMAIL:", 0) == 0) {
            cur.email = line.substr(6);
        } else if (line.rfind("TIME:", 0) == 0) {
            try { cur.timestamp = std::stoll(line.substr(5)); } catch (...) {}
        } else if (line.rfind("SUBJECT:", 0) == 0) {
            cur.subject = line.substr(8);
            inBody = false;
        } else if (line.rfind("BODY:", 0) == 0) {
            cur.body = line.substr(5);
            inBody = true;
        } else if (inBody) {
            if (!cur.body.empty()) cur.body += '\n';
            cur.body += line;
        } else if (line.rfind("FILE:", 0) == 0) {
            cur.changedFiles.push_back(line.substr(5));
        }
    }
    flush();
    return records;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct GitHistory::Impl {
    std::string              repoPath;
    std::vector<CommitRecord> commits;
    bool                     loaded{false};
    std::vector<HistoryLoadedCb> callbacks;

    std::string git(const std::string& args) const {
        std::string cmd = "git -C \"" + repoPath + "\" " + args + " 2>/dev/null";
        return runCommand(cmd);
    }
};

GitHistory::GitHistory() : m_impl(new Impl()) {}
GitHistory::~GitHistory() { delete m_impl; }

bool GitHistory::Load(const std::string& repoPath, uint32_t maxCommits) {
    m_impl->repoPath = repoPath;
    m_impl->commits.clear();
    m_impl->loaded = false;

    auto t0 = std::chrono::steady_clock::now();

    std::string fmt =
        "--pretty=format:"
        "COMMIT:%H%nAUTHOR:%an%nEMAIL:%ae%nTIME:%at%nSUBJECT:%s%nBODY:%b"
        "%n---END---";
    std::string maxStr = std::to_string(maxCommits);
    std::string raw = m_impl->git("log -n " + maxStr + " " + fmt);

    m_impl->commits = parseLog(raw);
    m_impl->loaded  = !m_impl->commits.empty();

    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    HistoryStats stats;
    stats.totalCommits = m_impl->commits.size();
    std::unordered_set<std::string> authors;
    for (const auto& c : m_impl->commits) authors.insert(c.author);
    stats.authors     = authors.size();
    stats.loadTimeMs  = static_cast<int64_t>(ms);
    if (!m_impl->commits.empty()) {
        stats.latestCommit = m_impl->commits.front().timestamp;
        stats.firstCommit  = m_impl->commits.back().timestamp;
    }
    for (auto& cb : m_impl->callbacks) cb(stats);
    return m_impl->loaded;
}

bool GitHistory::IsLoaded() const { return m_impl->loaded; }

void GitHistory::Clear() {
    m_impl->commits.clear();
    m_impl->loaded = false;
}

const std::vector<CommitRecord>& GitHistory::AllCommits() const {
    return m_impl->commits;
}

const CommitRecord* GitHistory::FindBySha(const std::string& sha) const {
    for (const auto& c : m_impl->commits)
        if (c.sha == sha || c.shortSha == sha) return &c;
    return nullptr;
}

HistoryStats GitHistory::Stats() const {
    HistoryStats s;
    s.totalCommits = m_impl->commits.size();
    std::unordered_set<std::string> authors;
    for (const auto& c : m_impl->commits) authors.insert(c.author);
    s.authors = authors.size();
    if (!m_impl->commits.empty()) {
        s.latestCommit = m_impl->commits.front().timestamp;
        s.firstCommit  = m_impl->commits.back().timestamp;
    }
    return s;
}

std::vector<CommitRecord> GitHistory::Filter(const HistoryFilter& f) const {
    std::vector<CommitRecord> out;
    for (const auto& c : m_impl->commits) {
        if (!f.authorFilter.empty() &&
            c.author.find(f.authorFilter) == std::string::npos) continue;
        if (!f.keywordFilter.empty() &&
            c.subject.find(f.keywordFilter) == std::string::npos &&
            c.body.find(f.keywordFilter) == std::string::npos) continue;
        if (f.since > 0 && c.timestamp < f.since) continue;
        if (f.until > 0 && c.timestamp > f.until) continue;
        if (!f.filePathFilter.empty()) {
            bool found = false;
            for (const auto& file : c.changedFiles)
                if (file.find(f.filePathFilter) != std::string::npos) { found = true; break; }
            if (!found) continue;
        }
        out.push_back(c);
    }
    return out;
}

std::string GitHistory::Diff(const std::string& sha) const {
    return m_impl->git("show " + sha);
}

std::string GitHistory::DiffRange(const std::string& fromSha,
                                   const std::string& toSha) const {
    return m_impl->git("diff " + fromSha + " " + toSha);
}

std::vector<BlameLine> GitHistory::Blame(const std::string& filePath) const {
    std::vector<BlameLine> lines;
    std::string raw = m_impl->git(
        "blame --porcelain \"" + filePath + "\"");
    std::istringstream ss(raw);
    std::string line;
    BlameLine cur;
    bool hasSha = false;
    while (std::getline(ss, line)) {
        if (line.size() >= 40 && std::isxdigit(static_cast<unsigned char>(line[0]))) {
            if (hasSha) lines.push_back(cur);
            cur = BlameLine{};
            cur.sha = line.substr(0, 40);
            hasSha = true;
        } else if (line.rfind("author ", 0) == 0) {
            cur.author = line.substr(7);
        } else if (line.rfind("author-time ", 0) == 0) {
            try { cur.timestamp = std::stoll(line.substr(12)); } catch (...) {}
        } else if (!line.empty() && line[0] == '\t') {
            cur.content = line.substr(1);
        }
    }
    if (hasSha) lines.push_back(cur);
    return lines;
}

BlameLine GitHistory::BlameLine_(const std::string& filePath,
                                  uint32_t lineNumber) const {
    auto all = Blame(filePath);
    if (lineNumber > 0 && lineNumber <= all.size())
        return all[lineNumber - 1];
    return {};
}

std::vector<std::string> GitHistory::LocalBranches() const {
    std::vector<std::string> branches;
    std::string raw = m_impl->git("branch --format=%(refname:short)");
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line))
        if (!line.empty()) branches.push_back(line);
    return branches;
}

std::vector<std::string> GitHistory::RemoteBranches() const {
    std::vector<std::string> branches;
    std::string raw = m_impl->git("branch -r --format=%(refname:short)");
    std::istringstream ss(raw);
    std::string line;
    while (std::getline(ss, line))
        if (!line.empty()) branches.push_back(line);
    return branches;
}

std::string GitHistory::CurrentBranch() const {
    std::string b = m_impl->git("rev-parse --abbrev-ref HEAD");
    if (!b.empty() && b.back() == '\n') b.pop_back();
    return b;
}

void GitHistory::OnHistoryLoaded(HistoryLoadedCb cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

} // namespace Tools
