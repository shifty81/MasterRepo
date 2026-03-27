#include "Runtime/Gameplay/GameModeManager/GameModeManager.h"
#include <algorithm>
#include <climits>
#include <unordered_map>
#include <vector>

namespace Runtime {

struct GameModeManager::Impl {
    std::unordered_map<std::string, GameModeDesc> modes;

    MatchPhase   phase{MatchPhase::Waiting};
    float        elapsed{0.f};
    float        countdown{0.f};
    GameModeDesc activeMode;
    bool         sessionActive{false};
    bool         wonPending{false};
    std::string  winTeamPending;

    std::unordered_map<uint32_t, PlayerSessionData> players;
    std::unordered_map<std::string, int32_t>        teamScores;

    PhaseCb   onPhaseChange;
    ResultCb  onMatchEnd;
    RespawnCb onRespawnQueued;

    void SetPhase(MatchPhase p) {
        phase = p;
        if (onPhaseChange) onPhaseChange(p);
    }

    void EvaluateWinConditions() {
        if (phase != MatchPhase::Running) return;

        bool won = false;
        std::string winTeam;

        if (activeMode.winCondition == WinCondition::ScoreLimit) {
            for (auto& [tid, score] : teamScores) {
                if (score >= (int32_t)activeMode.scoreLimit) {
                    won = true; winTeam = tid; break;
                }
            }
        }
        else if (activeMode.winCondition == WinCondition::TimeLimit) {
            won = (elapsed >= activeMode.timeLimitSeconds);
            if (won) {
                int32_t best = INT32_MIN;
                for (auto& [tid,s] : teamScores)
                    if (s > best) { best=s; winTeam=tid; }
            }
        }
        else if (activeMode.winCondition == WinCondition::LastTeamStanding) {
            uint32_t aliveTeams = 0;
            for (auto& [tid, score] : teamScores) {
                for (auto& [pid, pd] : players)
                    if (pd.teamId == tid && pd.alive) { aliveTeams++; break; }
            }
            if (aliveTeams <= 1) {
                won = true;
                for (auto& [pid, pd] : players)
                    if (pd.alive) { winTeam = pd.teamId; break; }
            }
        }

        if (won) {
            // signal via flag; called by outer Tick
            wonPending = true;
            winTeamPending = winTeam;
        }
    }
};

GameModeManager::GameModeManager()  : m_impl(new Impl) {}
GameModeManager::~GameModeManager() { Shutdown(); delete m_impl; }

void GameModeManager::Init()     {}
void GameModeManager::Shutdown() { EndSession(); }

void GameModeManager::Tick(float dt)
{
    if (!m_impl->sessionActive) return;

    if (m_impl->phase == MatchPhase::Countdown) {
        m_impl->countdown -= dt;
        if (m_impl->countdown <= 0.f) m_impl->SetPhase(MatchPhase::Running);
    }

    if (m_impl->phase == MatchPhase::Running) {
        m_impl->elapsed += dt;
        // Respawn timers
        for (auto& [pid, pd] : m_impl->players) {
            if (!pd.alive && pd.respawnTimer > 0.f) {
                pd.respawnTimer -= dt;
                if (pd.respawnTimer <= 0.f) pd.respawnTimer = 0.f;
            }
        }
        m_impl->EvaluateWinConditions();
        if (m_impl->wonPending) {
            m_impl->wonPending = false;
            ForceEndSession(m_impl->winTeamPending);
        }
    }
}

void GameModeManager::RegisterMode(const GameModeDesc& desc)  { m_impl->modes[desc.id] = desc; }
void GameModeManager::UnregisterMode(const std::string& id)    { m_impl->modes.erase(id); }
bool GameModeManager::HasMode(const std::string& id) const     { return m_impl->modes.count(id)>0; }

bool GameModeManager::StartSession(const std::string& modeId, uint32_t /*maxPlayers*/)
{
    auto it = m_impl->modes.find(modeId);
    if (it == m_impl->modes.end()) return false;
    m_impl->activeMode   = it->second;
    m_impl->players.clear();
    m_impl->teamScores.clear();
    for (auto& t : m_impl->activeMode.teams) m_impl->teamScores[t.id] = 0;
    m_impl->elapsed      = 0.f;
    m_impl->countdown    = m_impl->activeMode.countdownSeconds;
    m_impl->sessionActive = true;
    m_impl->SetPhase(MatchPhase::Countdown);
    return true;
}

void GameModeManager::EndSession()
{
    if (!m_impl->sessionActive) return;
    m_impl->sessionActive = false;
    m_impl->SetPhase(MatchPhase::Waiting);
}

void GameModeManager::ForceEndSession(const std::string& winTeamId)
{
    if (!m_impl->sessionActive) return;
    m_impl->SetPhase(MatchPhase::PostGame);
    MatchResult res;
    res.winnerTeamId = winTeamId;
    res.modeName     = m_impl->activeMode.id;
    res.duration     = m_impl->elapsed;
    for (auto& [pid,pd] : m_impl->players) res.finalStats.push_back(pd);
    if (m_impl->onMatchEnd) m_impl->onMatchEnd(res);
    m_impl->sessionActive = false;
}

MatchPhase GameModeManager::GetPhase()         const { return m_impl->phase; }
float      GameModeManager::GetElapsedTime()   const { return m_impl->elapsed; }
float      GameModeManager::GetRemainingTime() const
{
    if (m_impl->activeMode.winCondition == WinCondition::TimeLimit)
        return std::max(0.f, m_impl->activeMode.timeLimitSeconds - m_impl->elapsed);
    return 0.f;
}

bool GameModeManager::AddPlayer(uint32_t playerId, const std::string& teamId)
{
    PlayerSessionData pd;
    pd.playerId = playerId;
    pd.teamId   = teamId;
    pd.alive    = true;
    m_impl->players[playerId] = pd;
    return true;
}

void GameModeManager::RemovePlayer(uint32_t playerId)
{
    m_impl->players.erase(playerId);
}

bool GameModeManager::RespawnPlayer(uint32_t playerId, const float /*spawnPos*/[3])
{
    auto it = m_impl->players.find(playerId);
    if (it == m_impl->players.end()) return false;
    it->second.alive = true;
    it->second.respawnTimer = 0.f;
    return true;
}

const PlayerSessionData* GameModeManager::GetPlayerData(uint32_t playerId) const
{
    auto it = m_impl->players.find(playerId);
    return it != m_impl->players.end() ? &it->second : nullptr;
}

void GameModeManager::RecordEvent(const GameEventRecord& evt)
{
    auto pit = m_impl->players.find(evt.playerId);
    if (pit == m_impl->players.end()) return;
    auto& pd = pit->second;
    switch (evt.type) {
        case GameEventType::Kill:
            pd.kills++; pd.score++;
            m_impl->teamScores[pd.teamId]++;
            break;
        case GameEventType::Death:
            pd.deaths++; pd.alive = false;
            pd.respawnTimer = m_impl->activeMode.respawnDelay;
            if (m_impl->onRespawnQueued && m_impl->activeMode.respawnRule != RespawnRule::NoRespawn)
                m_impl->onRespawnQueued(evt.playerId, m_impl->activeMode.respawnDelay);
            break;
        case GameEventType::Assist:
            pd.assists++; break;
        case GameEventType::FlagCapture:
        case GameEventType::ObjectivePoint:
            pd.score += (int32_t)evt.value;
            m_impl->teamScores[pd.teamId] += (int32_t)evt.value;
            break;
        default: break;
    }
}

int32_t GameModeManager::GetTeamScore(const std::string& teamId) const
{
    auto it = m_impl->teamScores.find(teamId);
    return it != m_impl->teamScores.end() ? it->second : 0;
}

int32_t GameModeManager::GetPlayerScore(uint32_t playerId) const
{
    auto it = m_impl->players.find(playerId);
    return it != m_impl->players.end() ? it->second.score : 0;
}

void GameModeManager::SetOnPhaseChange(PhaseCb cb)      { m_impl->onPhaseChange   = cb; }
void GameModeManager::SetOnMatchEnd(ResultCb cb)         { m_impl->onMatchEnd       = cb; }
void GameModeManager::SetOnRespawnQueued(RespawnCb cb)   { m_impl->onRespawnQueued  = cb; }

} // namespace Runtime
