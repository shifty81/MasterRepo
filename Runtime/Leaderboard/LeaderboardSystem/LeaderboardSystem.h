#pragma once
/**
 * @file LeaderboardSystem.h
 * @brief Ranked score leaderboard with local storage, pagination, and tie-breaking.
 *
 * Features:
 *   - SubmitScore(playerId, playerName, score, metadata): insert/update entry
 *   - GetRank(playerId) → uint32_t: 1-based rank of player
 *   - GetPage(pageIndex, pageSize) → LeaderboardEntry[]: paginated rank slice
 *   - GetTopN(n) → LeaderboardEntry[]: top-N entries
 *   - GetEntry(playerId) → LeaderboardEntry*
 *   - GetTotalEntries() → uint32_t
 *   - SetSortDescending(on): true=highest first (default), false=lowest first
 *   - SetTieBreaker(field): "time" / "submission_order" / "alphabetical"
 *   - Save(path) / Load(path) → bool: JSON-like flat serialisation
 *   - Clear()
 *   - SetOnNewTopEntry(n, cb): callback fired when a new top-N entry is set
 *   - SetMaxEntries(n): cap total entries, dropping lowest-ranking excess
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct LeaderboardEntry {
    uint32_t    playerId{0};
    std::string playerName;
    float       score{0};
    uint64_t    submissionOrder{0};
    std::string metadata;
    uint32_t    rank{0};
};

class LeaderboardSystem {
public:
    LeaderboardSystem();
    ~LeaderboardSystem();

    void Init    ();
    void Shutdown();
    void Reset   ();
    void Clear   ();

    // Submission
    void SubmitScore(uint32_t playerId, const std::string& name,
                     float score, const std::string& metadata="");

    // Query
    uint32_t                    GetRank        (uint32_t playerId) const;
    std::vector<LeaderboardEntry> GetTopN       (uint32_t n)       const;
    std::vector<LeaderboardEntry> GetPage       (uint32_t pageIdx, uint32_t pageSize) const;
    const LeaderboardEntry*     GetEntry        (uint32_t playerId) const;
    uint32_t                    GetTotalEntries () const;

    // Config
    void SetSortDescending(bool on);
    void SetTieBreaker    (const std::string& field);
    void SetMaxEntries    (uint32_t n);

    // Persistence
    bool Save(const std::string& path) const;
    bool Load(const std::string& path);

    // Callbacks
    void SetOnNewTopEntry(uint32_t topN, std::function<void(const LeaderboardEntry&)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
