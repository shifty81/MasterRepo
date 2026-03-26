#pragma once
/**
 * @file LeaderboardSystem.h
 * @brief Local-first leaderboard with optional cloud backend.
 *
 * Supports multiple named boards (e.g. "global", "weekly", "friends"),
 * score submission with player metadata, rank queries, and JSON persistence.
 *
 * Typical usage:
 * @code
 *   LeaderboardSystem lb;
 *   lb.Init("Data/leaderboards.json");
 *   lb.CreateBoard("global", ScoreOrder::Descending);
 *   lb.SubmitScore("global", {"player_42", "Commander", 98500});
 *   auto top10 = lb.GetTopScores("global", 10);
 *   uint32_t rank = lb.GetRank("global", "player_42");
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

// ── Sort order ────────────────────────────────────────────────────────────────

enum class ScoreOrder : uint8_t { Descending = 0, Ascending = 1 };

// ── A single leaderboard entry ────────────────────────────────────────────────

struct LeaderboardEntry {
    std::string playerId;
    std::string displayName;
    int64_t     score{0};
    uint64_t    timestampMs{0};
    std::string metadata;   ///< JSON string for extra data (ship type, level, etc.)
    uint32_t    rank{0};    ///< 1-based rank (filled in by queries)
};

// ── Board metadata ────────────────────────────────────────────────────────────

struct LeaderboardInfo {
    std::string name;
    ScoreOrder  order{ScoreOrder::Descending};
    uint32_t    maxEntries{1000};  ///< prune to this size after each submit
    uint32_t    entryCount{0};
};

// ── LeaderboardSystem ─────────────────────────────────────────────────────────

class LeaderboardSystem {
public:
    LeaderboardSystem();
    ~LeaderboardSystem();

    void Init(const std::string& persistPath = "Data/leaderboards.json");
    void Shutdown();

    // ── Board management ──────────────────────────────────────────────────────

    void     CreateBoard(const std::string& name,
                         ScoreOrder order = ScoreOrder::Descending,
                         uint32_t maxEntries = 1000);
    void     DeleteBoard(const std::string& name);
    bool     HasBoard(const std::string& name) const;
    LeaderboardInfo GetBoardInfo(const std::string& name) const;
    std::vector<LeaderboardInfo> AllBoards() const;

    // ── Score submission ──────────────────────────────────────────────────────

    void     SubmitScore(const std::string& board, const LeaderboardEntry& entry);

    /// Update only if the new score is better (per the board's sort order).
    bool     UpdateBestScore(const std::string& board,
                             const LeaderboardEntry& entry);

    // ── Queries ───────────────────────────────────────────────────────────────

    std::vector<LeaderboardEntry> GetTopScores(const std::string& board,
                                               uint32_t count = 10) const;

    std::vector<LeaderboardEntry> GetAroundPlayer(const std::string& board,
                                                  const std::string& playerId,
                                                  uint32_t radius = 5) const;

    uint32_t         GetRank(const std::string& board,
                             const std::string& playerId) const;

    LeaderboardEntry GetPlayerEntry(const std::string& board,
                                    const std::string& playerId) const;

    // ── Persistence ───────────────────────────────────────────────────────────

    void Save() const;
    void Load();
    void ResetBoard(const std::string& name);

    // ── Callbacks ─────────────────────────────────────────────────────────────

    void OnNewTopScore(std::function<void(const std::string& board,
                                          const LeaderboardEntry&)> cb);
    void OnRankChanged(std::function<void(const std::string& board,
                                          const std::string& playerId,
                                          uint32_t newRank)> cb);

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace Runtime
