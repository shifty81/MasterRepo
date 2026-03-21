#include "Runtime/Gameplay/GameSession/GameSession.h"
#include <algorithm>
#include <chrono>

namespace Runtime {

struct GameSession::Impl {
    SessionConfig             config;
    SessionState              state{SessionState::Lobby};
    std::vector<PlayerEntry>  players;
    PlayerID                  nextId{1};
    float                     elapsed{0.0f};
    std::vector<SessionEventFn> callbacks;
};

GameSession::GameSession(const SessionConfig& cfg)
    : m_impl(new Impl()) {
    m_impl->config = cfg;
}
GameSession::~GameSession() { delete m_impl; }

void GameSession::Start() {
    if (m_impl->state == SessionState::Lobby ||
        m_impl->state == SessionState::Paused) {
        m_impl->state = SessionState::Active;
        fireEvent("session_start");
    }
}

void GameSession::Pause() {
    if (m_impl->state == SessionState::Active) {
        m_impl->state = SessionState::Paused;
        fireEvent("session_pause");
    }
}

void GameSession::Resume() {
    if (m_impl->state == SessionState::Paused) {
        m_impl->state = SessionState::Active;
        fireEvent("session_resume");
    }
}

void GameSession::End() {
    m_impl->state = SessionState::Ended;
    fireEvent("session_end");
}

SessionState GameSession::GetState() const { return m_impl->state; }
bool GameSession::IsActive() const { return m_impl->state == SessionState::Active; }

PlayerID GameSession::AddPlayer(const std::string& name) {
    if (m_impl->players.size() >= m_impl->config.maxPlayers) return 0;
    PlayerEntry e;
    e.id   = m_impl->nextId++;
    e.name = name;
    m_impl->players.push_back(e);
    fireEvent("player_join", e.id);
    return e.id;
}

bool GameSession::RemovePlayer(PlayerID id) {
    auto it = std::remove_if(m_impl->players.begin(), m_impl->players.end(),
                              [id](const PlayerEntry& p){ return p.id == id; });
    if (it == m_impl->players.end()) return false;
    m_impl->players.erase(it, m_impl->players.end());
    fireEvent("player_leave", id);
    return true;
}

PlayerEntry* GameSession::GetPlayer(PlayerID id) {
    for (auto& p : m_impl->players) if (p.id == id) return &p;
    return nullptr;
}
const PlayerEntry* GameSession::GetPlayer(PlayerID id) const {
    for (const auto& p : m_impl->players) if (p.id == id) return &p;
    return nullptr;
}
const std::vector<PlayerEntry>& GameSession::Players() const { return m_impl->players; }
uint32_t GameSession::PlayerCount() const {
    return static_cast<uint32_t>(m_impl->players.size());
}

void GameSession::SetPlayerStatus(PlayerID id, PlayerStatus status) {
    if (auto* p = GetPlayer(id)) {
        p->status = status;
        fireEvent("player_status_changed", id);
    }
}

void GameSession::AddScore(PlayerID id, int64_t delta) {
    if (auto* p = GetPlayer(id)) {
        p->score += delta;
        if (delta > 0) fireEvent("score_added", id);
        checkEndConditions();
    }
}

void GameSession::RecordKill(PlayerID killerId, PlayerID victimId) {
    if (auto* killer = GetPlayer(killerId)) {
        ++killer->kills;
        killer->score += 100;
    }
    if (auto* victim = GetPlayer(victimId)) {
        ++victim->deaths;
        if (!m_impl->config.friendlyFire)
            victim->status = PlayerStatus::Eliminated;
        fireEvent("player_eliminated", victimId);
    }
    checkEndConditions();
}

void GameSession::Tick(float dt) {
    if (m_impl->state != SessionState::Active) return;
    m_impl->elapsed += dt;
    for (auto& p : m_impl->players)
        if (p.status == PlayerStatus::Connected) p.playTime += dt;
    checkEndConditions();
}

float GameSession::ElapsedSeconds() const { return m_impl->elapsed; }

SessionStats GameSession::GetStats() const {
    SessionStats s;
    s.elapsedSec  = m_impl->elapsed;
    s.playerCount = static_cast<uint32_t>(m_impl->players.size());
    for (const auto& p : m_impl->players) {
        s.totalKills += p.kills;
        if (p.score > s.highScore) {
            s.highScore     = p.score;
            s.leadingPlayer = p.id;
        }
    }
    return s;
}

std::vector<const PlayerEntry*> GameSession::Leaderboard() const {
    std::vector<const PlayerEntry*> lb;
    lb.reserve(m_impl->players.size());
    for (const auto& p : m_impl->players) lb.push_back(&p);
    std::sort(lb.begin(), lb.end(),
              [](const PlayerEntry* a, const PlayerEntry* b){
                  return a->score > b->score; });
    return lb;
}

void GameSession::OnSessionEvent(SessionEventFn fn) {
    m_impl->callbacks.push_back(std::move(fn));
}

const SessionConfig& GameSession::Config() const { return m_impl->config; }

void GameSession::fireEvent(const std::string& event, PlayerID pid) {
    for (auto& cb : m_impl->callbacks) cb(event, pid);
}

bool GameSession::checkEndConditions() {
    if (m_impl->state != SessionState::Active) return false;
    const auto& cfg = m_impl->config;
    // Time limit
    if (cfg.timeLimitSec > 0.0f && m_impl->elapsed >= cfg.timeLimitSec) {
        End(); return true;
    }
    // Score limit
    if (cfg.scoreLimit > 0) {
        for (const auto& p : m_impl->players) {
            if (p.score >= cfg.scoreLimit) { End(); return true; }
        }
    }
    return false;
}

} // namespace Runtime
