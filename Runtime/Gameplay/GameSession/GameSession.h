#pragma once
/**
 * @file GameSession.h
 * @brief Runtime game session — player registry, state machine, scoring.
 *
 * A GameSession represents one playthrough or multiplayer match.  It tracks:
 *   - Session lifecycle (Lobby → Active → Paused → Ended)
 *   - Connected players with per-player score and status
 *   - Wall-clock and in-game time
 *   - Session-level event callbacks (join, leave, score change, end)
 */

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Runtime {

using PlayerID = uint32_t;

enum class SessionState : uint8_t {
    Lobby,   ///< Waiting for players
    Active,  ///< In progress
    Paused,  ///< Temporarily suspended
    Ended,   ///< Session over
};

enum class PlayerStatus : uint8_t {
    Connected,
    Disconnected,
    Spectating,
    Eliminated,
};

struct PlayerEntry {
    PlayerID    id{0};
    std::string name;
    PlayerStatus status{PlayerStatus::Connected};
    int64_t     score{0};
    uint32_t    kills{0};
    uint32_t    deaths{0};
    float       ping{0.0f};    ///< ms
    float       playTime{0.0f};///< seconds this session
};

struct SessionConfig {
    std::string name{"Session"};
    uint32_t    maxPlayers{16};
    float       timeLimitSec{0.0f};  ///< 0 = no limit
    int64_t     scoreLimit{0};       ///< 0 = no limit
    bool        allowSpectators{true};
    bool        friendlyFire{false};
};

struct SessionStats {
    float    elapsedSec{0.0f};
    uint32_t totalKills{0};
    uint32_t playerCount{0};
    PlayerID leadingPlayer{0};
    int64_t  highScore{0};
};

using SessionEventFn = std::function<void(const std::string& event,
                                          PlayerID playerId)>;

/// GameSession — single-session lifecycle and player state manager.
class GameSession {
public:
    explicit GameSession(const SessionConfig& cfg = {});
    ~GameSession();

    // ── lifecycle ────────────────────────────────────────────
    void Start();
    void Pause();
    void Resume();
    void End();
    SessionState GetState() const;
    bool IsActive() const;

    // ── player management ─────────────────────────────────────
    PlayerID     AddPlayer(const std::string& name);
    bool         RemovePlayer(PlayerID id);
    PlayerEntry* GetPlayer(PlayerID id);
    const PlayerEntry* GetPlayer(PlayerID id) const;
    const std::vector<PlayerEntry>& Players() const;
    uint32_t     PlayerCount() const;

    void SetPlayerStatus(PlayerID id, PlayerStatus status);
    void AddScore(PlayerID id, int64_t delta);
    void RecordKill(PlayerID killerId, PlayerID victimId);

    // ── time ─────────────────────────────────────────────────
    /// Advance the session clock by dt seconds (call each game tick).
    void Tick(float dt);
    float ElapsedSeconds() const;

    // ── stats / query ─────────────────────────────────────────
    SessionStats GetStats() const;
    /// Sorted leaderboard (highest score first).
    std::vector<const PlayerEntry*> Leaderboard() const;

    // ── callbacks ─────────────────────────────────────────────
    void OnSessionEvent(SessionEventFn fn);

    // ── config ────────────────────────────────────────────────
    const SessionConfig& Config() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};

    void fireEvent(const std::string& event, PlayerID pid = 0);
    bool checkEndConditions();
};

} // namespace Runtime
