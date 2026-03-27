#pragma once
/**
 * @file GameModeManager.h
 * @brief Game-mode rules engine: match setup, win/lose conditions, team management.
 *
 * Features:
 *   - Register named game-mode definitions (Deathmatch, CTF, Survival, …)
 *   - Session lifecycle: Waiting → Countdown → Running → PostGame
 *   - Team setup with configurable size, colour, friendly-fire
 *   - Score/objective tracking (kills, flag captures, survival time, points)
 *   - Win condition evaluation (score limit, time limit, last-team-standing, custom)
 *   - Respawn rules per mode (instant, delay, spectator, no-respawn)
 *   - Per-mode loadout restrictions and spawn point pools
 *   - Callback-driven results; no direct UI dependency
 *
 * Typical usage:
 * @code
 *   GameModeManager gm;
 *   gm.Init();
 *   gm.RegisterMode({"deathmatch", 10, WinCondition::ScoreLimit, {8,0,0}});
 *   gm.StartSession("deathmatch", 8);
 *   gm.AddPlayer(playerId, "TeamA");
 *   gm.RecordEvent({playerId, GameEvent::Kill, targetId});
 *   gm.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class WinCondition : uint8_t {
    ScoreLimit=0, TimeLimit, LastTeamStanding, ObjectiveCapture, Custom
};

enum class RespawnRule : uint8_t {
    Instant=0, FixedDelay, SpectateUntilRound, NoRespawn
};

struct TeamDesc {
    std::string id;
    float       colour[3]{1,1,1};
    uint32_t    maxPlayers{4};
    bool        friendlyFire{false};
};

struct GameModeDesc {
    std::string       id;
    uint32_t          scoreLimit{10};
    float             timeLimitSeconds{300.f};
    WinCondition      winCondition{WinCondition::ScoreLimit};
    RespawnRule       respawnRule{RespawnRule::FixedDelay};
    float             respawnDelay{5.f};
    std::vector<TeamDesc> teams;
    bool              allowSpectators{true};
    float             countdownSeconds{3.f};
};

enum class MatchPhase : uint8_t { Waiting=0, Countdown, Running, PostGame };

enum class GameEventType : uint8_t {
    Kill=0, Death, Assist, FlagCapture, FlagReturn, ObjectivePoint, Custom
};

struct GameEventRecord {
    uint32_t      playerId{0};
    GameEventType type{GameEventType::Kill};
    uint32_t      targetId{0};   ///< optional second actor
    float         value{1.f};    ///< e.g. point value for custom events
};

struct PlayerSessionData {
    uint32_t      playerId{0};
    std::string   teamId;
    int32_t       score{0};
    uint32_t      kills{0};
    uint32_t      deaths{0};
    uint32_t      assists{0};
    bool          alive{true};
    float         respawnTimer{0.f};
};

struct MatchResult {
    std::string   winnerTeamId;   ///< "" = draw
    std::string   modeName;
    float         duration{0.f};
    std::vector<PlayerSessionData> finalStats;
};

class GameModeManager {
public:
    GameModeManager();
    ~GameModeManager();

    void Init();
    void Shutdown();
    void Tick(float dt);

    // Mode registry
    void  RegisterMode(const GameModeDesc& desc);
    void  UnregisterMode(const std::string& modeId);
    bool  HasMode(const std::string& modeId) const;

    // Session control
    bool  StartSession(const std::string& modeId, uint32_t maxPlayers=8);
    void  EndSession();
    void  ForceEndSession(const std::string& reasonTeamId="");
    MatchPhase GetPhase() const;
    float      GetElapsedTime() const;
    float      GetRemainingTime() const;

    // Player management
    bool  AddPlayer(uint32_t playerId, const std::string& teamId="");
    void  RemovePlayer(uint32_t playerId);
    bool  RespawnPlayer(uint32_t playerId, const float spawnPos[3]);
    const PlayerSessionData* GetPlayerData(uint32_t playerId) const;

    // Events
    void  RecordEvent(const GameEventRecord& evt);

    // Score queries
    int32_t GetTeamScore(const std::string& teamId) const;
    int32_t GetPlayerScore(uint32_t playerId) const;

    // Callbacks
    using PhaseCb     = std::function<void(MatchPhase)>;
    using ResultCb    = std::function<void(const MatchResult&)>;
    using RespawnCb   = std::function<void(uint32_t playerId, float delay)>;
    void SetOnPhaseChange(PhaseCb cb);
    void SetOnMatchEnd(ResultCb cb);
    void SetOnRespawnQueued(RespawnCb cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
