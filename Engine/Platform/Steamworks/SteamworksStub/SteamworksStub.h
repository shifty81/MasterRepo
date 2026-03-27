#pragma once
/**
 * @file SteamworksStub.h
 * @brief Platform-agnostic achievement/leaderboard/stats stub mirroring Steamworks SDK surface.
 *
 * Features:
 *   - Achievement registry: id, name, description, hidden
 *   - Unlock achievement (no-op on non-Steam, fires callback)
 *   - Clear achievement (testing)
 *   - Stats: SetStat(int/float), GetStat, StoreStats
 *   - Leaderboard: FindOrCreate, UploadScore, DownloadEntries
 *   - User identity: GetSteamID (uint64 stub = 0 on non-Steam)
 *   - Overlay: ShowOverlay (no-op)
 *   - DLC: IsDlcInstalled
 *   - IsAvailable(): true only when real Steamworks is linked
 *   - All methods safe to call unconditionally (stub returns gracefully)
 *   - Callbacks via std::function (avoids Steamworks callback macros)
 *
 * Typical usage:
 * @code
 *   SteamworksStub sw;
 *   sw.Init();
 *   sw.RegisterAchievement("ACH_FIRST_KILL","First Blood","Kill an enemy",false);
 *   sw.UnlockAchievement("ACH_FIRST_KILL");
 *   sw.SetStatInt("TotalKills", sw.GetStatInt("TotalKills",0)+1);
 *   sw.StoreStats();
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

struct AchievementDef {
    std::string id;
    std::string name;
    std::string description;
    bool        hidden{false};
};

struct LeaderboardEntry {
    uint64_t    steamId{0};
    std::string name;
    int32_t     rank{0};
    int64_t     score{0};
};

class SteamworksStub {
public:
    SteamworksStub();
    ~SteamworksStub();

    bool Init    ();                ///< returns true if real Steamworks available
    void Shutdown();
    void RunCallbacks();            ///< call each frame (no-op stub)

    bool IsAvailable() const;       ///< false = stub only

    // Identity
    uint64_t    GetSteamID()    const;
    std::string GetPersonaName() const;

    // Achievements
    void RegisterAchievement(const AchievementDef& def);
    void RegisterAchievement(const std::string& id, const std::string& name,
                              const std::string& desc, bool hidden=false);
    bool UnlockAchievement  (const std::string& id);
    bool ClearAchievement   (const std::string& id);
    bool IsAchieved         (const std::string& id) const;
    std::vector<AchievementDef> GetAchievements() const;

    // Stats
    void    SetStatInt  (const std::string& name, int32_t v);
    void    SetStatFloat(const std::string& name, float v);
    int32_t GetStatInt  (const std::string& name, int32_t def=0)  const;
    float   GetStatFloat(const std::string& name, float def=0.f)  const;
    bool    StoreStats  ();

    // Leaderboards
    uint64_t FindOrCreateLeaderboard(const std::string& name);
    bool     UploadScore(uint64_t leaderboardHandle, int64_t score);
    std::vector<LeaderboardEntry> DownloadEntries(uint64_t leaderboardHandle,
                                                   int32_t start, int32_t end);

    // DLC
    bool IsDlcInstalled(uint32_t dlcAppId) const;

    // Overlay
    void ShowOverlay(const std::string& dialog="");

    // Callbacks
    void SetOnAchievementUnlocked(std::function<void(const std::string& id)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
