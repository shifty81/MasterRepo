#pragma once
/**
 * @file SessionManager.h
 * @brief Game session lifecycle: create, join, leave, player roster, state, persistence.
 *
 * Features:
 *   - SessionState: Idle, Lobby, Loading, Active, Paused, Ended
 *   - CreateSession(sessionId, maxPlayers) → bool
 *   - DestroySession(sessionId)
 *   - JoinSession(sessionId, playerId, displayName) → bool
 *   - LeaveSession(sessionId, playerId)
 *   - SetState(sessionId, state) / GetState(sessionId) → SessionState
 *   - StartSession(sessionId) / PauseSession(sessionId) / EndSession(sessionId)
 *   - GetPlayerIds(sessionId, out[]) → uint32_t
 *   - GetPlayerCount(sessionId) → uint32_t
 *   - GetDisplayName(sessionId, playerId) → string
 *   - SetPlayerData(sessionId, playerId, key, value) / GetPlayerData(...)
 *   - SetSessionData(sessionId, key, value) / GetSessionData(...)
 *   - SetOnStateChange(sessionId, cb)
 *   - SetOnPlayerJoin(sessionId, cb) / SetOnPlayerLeave(sessionId, cb)
 *   - GetActiveSessions(out[]) → uint32_t
 *   - Reset() / Shutdown()
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

enum class SessionState : uint8_t { Idle, Lobby, Loading, Active, Paused, Ended };

class SessionManager {
public:
    SessionManager();
    ~SessionManager();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Session lifecycle
    bool CreateSession (uint32_t sessionId, uint32_t maxPlayers = 16);
    void DestroySession(uint32_t sessionId);

    // Player management
    bool     JoinSession (uint32_t sessionId, uint32_t playerId,
                          const std::string& displayName);
    void     LeaveSession(uint32_t sessionId, uint32_t playerId);
    uint32_t GetPlayerIds(uint32_t sessionId,
                          std::vector<uint32_t>& out) const;
    uint32_t GetPlayerCount(uint32_t sessionId) const;
    std::string GetDisplayName(uint32_t sessionId, uint32_t playerId) const;

    // State control
    void         SetState     (uint32_t sessionId, SessionState state);
    SessionState GetState     (uint32_t sessionId) const;
    void         StartSession (uint32_t sessionId);
    void         PauseSession (uint32_t sessionId);
    void         EndSession   (uint32_t sessionId);

    // Per-entity KV data
    void        SetPlayerData(uint32_t sessionId, uint32_t playerId,
                              const std::string& key, const std::string& val);
    std::string GetPlayerData(uint32_t sessionId, uint32_t playerId,
                              const std::string& key,
                              const std::string& def = "") const;
    void        SetSessionData(uint32_t sessionId,
                               const std::string& key, const std::string& val);
    std::string GetSessionData(uint32_t sessionId,
                               const std::string& key,
                               const std::string& def = "") const;

    // Callbacks
    void SetOnStateChange (uint32_t sessionId,
                           std::function<void(SessionState prev, SessionState next)> cb);
    void SetOnPlayerJoin  (uint32_t sessionId,
                           std::function<void(uint32_t playerId)> cb);
    void SetOnPlayerLeave (uint32_t sessionId,
                           std::function<void(uint32_t playerId)> cb);

    // Query
    uint32_t GetActiveSessions(std::vector<uint32_t>& out) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
