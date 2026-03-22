#pragma once
/**
 * @file GitHistory.h
 * @brief Git commit-history browser: parse log, filter, diff, blame lookups.
 *
 * GitHistory wraps a subset of `git` CLI commands to build an in-process
 * commit history for display in the IDE git panel:
 *
 *   - CommitRecord: SHA, author, email, timestamp, subject, body, changed files.
 *   - LoadHistory(repoPath, maxCommits): runs `git log` and parses the output.
 *   - Filter: search by author, date range, keyword, or affected file path.
 *   - Diff(sha): retrieve the diff for a specific commit.
 *   - BlameLine(file, line): run `git blame` for a single line; returns author+sha.
 *   - Branches: list local and remote refs.
 *   - OnHistoryLoaded: callback fired when the async load completes.
 *
 * All git commands are run synchronously via popen; the caller is responsible
 * for off-thread dispatch if non-blocking UI is required.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace Tools {

// ── Commit record ─────────────────────────────────────────────────────────
struct CommitRecord {
    std::string sha;           ///< Full 40-char SHA
    std::string shortSha;      ///< First 8 chars
    std::string author;
    std::string email;
    int64_t     timestamp{0};  ///< Unix epoch seconds
    std::string subject;       ///< First line of commit message
    std::string body;          ///< Remainder of commit message
    std::vector<std::string> changedFiles;
    int32_t     insertions{0};
    int32_t     deletions{0};
};

// ── Blame line ────────────────────────────────────────────────────────────
struct BlameLine {
    std::string sha;
    std::string author;
    int64_t     timestamp{0};
    uint32_t    origLine{0};
    std::string content;
};

// ── History filter ────────────────────────────────────────────────────────
struct HistoryFilter {
    std::string authorFilter;       ///< Substring match on author name
    std::string keywordFilter;      ///< Substring match on subject/body
    std::string filePathFilter;     ///< Only commits touching this path
    int64_t     since{0};           ///< Unix epoch; 0 = no limit
    int64_t     until{0};           ///< Unix epoch; 0 = no limit
};

// ── History stats ─────────────────────────────────────────────────────────
struct HistoryStats {
    size_t  totalCommits{0};
    size_t  authors{0};
    int64_t firstCommit{0};
    int64_t latestCommit{0};
    int64_t loadTimeMs{0};
};

// ── Browser ───────────────────────────────────────────────────────────────
class GitHistory {
public:
    GitHistory();
    ~GitHistory();

    GitHistory(const GitHistory&) = delete;
    GitHistory& operator=(const GitHistory&) = delete;

    // ── load ──────────────────────────────────────────────────
    bool Load(const std::string& repoPath, uint32_t maxCommits = 500);
    bool IsLoaded() const;
    void Clear();

    // ── access ────────────────────────────────────────────────
    const std::vector<CommitRecord>& AllCommits() const;
    const CommitRecord*              FindBySha(const std::string& sha) const;
    HistoryStats                     Stats() const;

    // ── filter ────────────────────────────────────────────────
    std::vector<CommitRecord> Filter(const HistoryFilter& f) const;

    // ── diff & blame ──────────────────────────────────────────
    std::string  Diff(const std::string& sha) const;
    std::string  DiffRange(const std::string& fromSha,
                           const std::string& toSha) const;
    std::vector<BlameLine> Blame(const std::string& filePath) const;
    BlameLine              BlameLine_(const std::string& filePath,
                                      uint32_t lineNumber) const;

    // ── branches ──────────────────────────────────────────────
    std::vector<std::string> LocalBranches()  const;
    std::vector<std::string> RemoteBranches() const;
    std::string              CurrentBranch()  const;

    // ── callbacks ─────────────────────────────────────────────
    using HistoryLoadedCb = std::function<void(const HistoryStats&)>;
    void OnHistoryLoaded(HistoryLoadedCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Tools
