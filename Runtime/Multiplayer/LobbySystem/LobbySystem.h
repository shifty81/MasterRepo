#pragma once
/**
 * @file LobbySystem.h
 * @brief Multiplayer lobby management: rooms, player slots, ready-check, host migration.
 *
 * Design:
 *   - LobbyRoom: named room with configurable max players, password, game-mode tag,
 *     custom metadata, and a per-player slot list.
 *   - PlayerSlot: represents one connected player (local or remote stub).
 *   - LobbySystem: manages multiple rooms; handles join/leave/kick/promote-host;
 *     fires callbacks for lobby changes.
 *   - Host migration: if host leaves, the next-oldest player is promoted automatically.
 *   - Observer mode: spectator slots do not count toward player cap.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Runtime::Multiplayer {

using RoomID  = uint32_t;
using PlayerID= uint32_t;
constexpr RoomID   kInvalidRoom   = 0;
constexpr PlayerID kInvalidPlayer = 0;

// ── Player slot ───────────────────────────────────────────────────────────
enum class SlotState : uint8_t {
    Open,       ///< Not yet filled
    Occupied,   ///< Player connected
    Closed,     ///< Locked by host
    Observer,   ///< Spectator (doesn't count toward cap)
};

struct PlayerSlot {
    uint32_t    index{0};
    SlotState   state{SlotState::Open};
    PlayerID    playerId{kInvalidPlayer};
    std::string displayName;
    bool        isHost{false};
    bool        isReady{false};
    uint32_t    ping{0};         ///< ms
    uint64_t    joinedAtMs{0};
};

// ── Room ──────────────────────────────────────────────────────────────────
struct LobbyRoomConfig {
    std::string name;
    uint32_t    maxPlayers{4};
    uint32_t    maxObservers{4};
    bool        requirePassword{false};
    std::string password;
    std::string gameMode;       ///< e.g. "Cooperative", "Versus"
    bool        allowJoinInProgress{false};
    bool        publicVisible{true};
    std::unordered_map<std::string, std::string> metadata;
};

enum class RoomPhase : uint8_t {
    Lobby,          ///< Waiting for players / ready-check
    Starting,       ///< Countdown before launch
    InProgress,     ///< Game is running
    Finished,
};

struct LobbyRoom {
    RoomID        id{kInvalidRoom};
    LobbyRoomConfig config;
    RoomPhase     phase{RoomPhase::Lobby};
    std::vector<PlayerSlot> slots;
    PlayerID      hostId{kInvalidPlayer};
    uint64_t      createdAtMs{0};
    uint32_t      countdownSeconds{5};

    uint32_t PlayerCount() const;
    uint32_t ReadyCount() const;
    bool     AllPlayersReady() const;
    bool     IsFull() const;
    const PlayerSlot* GetSlot(PlayerID pid) const;
    PlayerSlot*       GetSlot(PlayerID pid);
};

// ── Events ────────────────────────────────────────────────────────────────
struct LobbyEvent {
    enum class Type {
        PlayerJoined, PlayerLeft, PlayerKicked, PlayerReady,
        HostChanged, GameStarting, GameStarted, RoomClosed,
        SlotChanged,
    };
    Type     type;
    RoomID   roomId{kInvalidRoom};
    PlayerID playerId{kInvalidPlayer};
};

using LobbyEventCb = std::function<void(const LobbyEvent&)>;

// ── System ───────────────────────────────────────────────────────────────
class LobbySystem {
public:
    LobbySystem();
    ~LobbySystem();

    // ── room management ───────────────────────────────────────
    RoomID  CreateRoom(const LobbyRoomConfig& cfg);
    bool    CloseRoom(RoomID id);
    bool    HasRoom(RoomID id) const;
    LobbyRoom* GetRoom(RoomID id);
    const LobbyRoom* GetRoom(RoomID id) const;
    size_t RoomCount() const;
    std::vector<RoomID> ListPublicRooms() const;

    // ── player actions ────────────────────────────────────────
    bool JoinRoom(RoomID id, PlayerID playerId, const std::string& name,
                  const std::string& password = "", bool asObserver = false);
    bool LeaveRoom(RoomID id, PlayerID playerId);
    bool KickPlayer(RoomID id, PlayerID hostId, PlayerID targetId);
    bool SetReady(RoomID id, PlayerID playerId, bool ready);

    // ── host actions ──────────────────────────────────────────
    bool CloseSlot(RoomID id, PlayerID hostId, uint32_t slotIndex);
    bool OpenSlot(RoomID id, PlayerID hostId, uint32_t slotIndex);
    bool PromoteHost(RoomID id, PlayerID currentHostId, PlayerID newHostId);
    bool StartGame(RoomID id, PlayerID hostId);

    // ── host migration ────────────────────────────────────────
    /// Called automatically when host disconnects.
    bool MigrateHost(RoomID id);

    // ── state ─────────────────────────────────────────────────
    bool SetPhase(RoomID id, RoomPhase phase);

    // ── callbacks ─────────────────────────────────────────────
    void OnEvent(LobbyEventCb cb);

    // ── tick ─────────────────────────────────────────────────
    void Update(float dt);  ///< Drives countdown timer

private:
    struct Impl;
    Impl* m_impl{nullptr};
    void emit(const LobbyEvent& ev);
};

} // namespace Runtime::Multiplayer
