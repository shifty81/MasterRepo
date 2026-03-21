#include "Runtime/Multiplayer/LobbySystem/LobbySystem.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace Runtime::Multiplayer {

static uint64_t lobbyNowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count());
}

// ── LobbyRoom helpers ─────────────────────────────────────────────────────
uint32_t LobbyRoom::PlayerCount() const {
    uint32_t n = 0;
    for (const auto& s : slots)
        if (s.state == SlotState::Occupied) ++n;
    return n;
}
uint32_t LobbyRoom::ReadyCount() const {
    uint32_t n = 0;
    for (const auto& s : slots)
        if (s.state == SlotState::Occupied && s.isReady) ++n;
    return n;
}
bool LobbyRoom::AllPlayersReady() const {
    uint32_t occ = 0;
    for (const auto& s : slots) if (s.state == SlotState::Occupied) ++occ;
    return occ > 0 && ReadyCount() == occ;
}
bool LobbyRoom::IsFull() const {
    return PlayerCount() >= config.maxPlayers;
}
const PlayerSlot* LobbyRoom::GetSlot(PlayerID pid) const {
    for (const auto& s : slots) if (s.playerId == pid) return &s;
    return nullptr;
}
PlayerSlot* LobbyRoom::GetSlot(PlayerID pid) {
    for (auto& s : slots) if (s.playerId == pid) return &s;
    return nullptr;
}

// ── Impl ─────────────────────────────────────────────────────────────────
struct LobbySystem::Impl {
    std::unordered_map<RoomID, LobbyRoom> rooms;
    RoomID nextRoomId{1};
    std::vector<LobbyEventCb> callbacks;
    float countdownAccum{0.0f};
};

LobbySystem::LobbySystem() : m_impl(new Impl()) {}
LobbySystem::~LobbySystem() { delete m_impl; }

void LobbySystem::emit(const LobbyEvent& ev) {
    for (auto& cb : m_impl->callbacks) cb(ev);
}

RoomID LobbySystem::CreateRoom(const LobbyRoomConfig& cfg) {
    RoomID newId = m_impl->nextRoomId++;
    LobbyRoom room;
    room.id          = newId;
    room.config      = cfg;
    room.createdAtMs = lobbyNowMs();
    // Pre-allocate slots
    room.slots.resize(cfg.maxPlayers + cfg.maxObservers);
    for (uint32_t i = 0; i < room.slots.size(); ++i) {
        room.slots[i].index = i;
        room.slots[i].state = (i < cfg.maxPlayers) ? SlotState::Open : SlotState::Observer;
    }
    m_impl->rooms[newId] = std::move(room);
    return newId;
}

bool LobbySystem::CloseRoom(RoomID id) {
    auto it = m_impl->rooms.find(id);
    if (it == m_impl->rooms.end()) return false;
    emit({LobbyEvent::Type::RoomClosed, id, kInvalidPlayer});
    m_impl->rooms.erase(it);
    return true;
}

bool LobbySystem::HasRoom(RoomID id) const { return m_impl->rooms.count(id) > 0; }

LobbyRoom* LobbySystem::GetRoom(RoomID id) {
    auto it = m_impl->rooms.find(id);
    return it != m_impl->rooms.end() ? &it->second : nullptr;
}
const LobbyRoom* LobbySystem::GetRoom(RoomID id) const {
    auto it = m_impl->rooms.find(id);
    return it != m_impl->rooms.end() ? &it->second : nullptr;
}
size_t LobbySystem::RoomCount() const { return m_impl->rooms.size(); }

std::vector<RoomID> LobbySystem::ListPublicRooms() const {
    std::vector<RoomID> out;
    for (const auto& [id, r] : m_impl->rooms)
        if (r.config.publicVisible) out.push_back(id);
    return out;
}

bool LobbySystem::JoinRoom(RoomID id, PlayerID playerId, const std::string& name,
                             const std::string& password, bool asObserver)
{
    auto* room = GetRoom(id);
    if (!room) return false;
    if (room->phase == RoomPhase::InProgress && !room->config.allowJoinInProgress)
        return false;
    if (room->config.requirePassword && password != room->config.password)
        return false;
    // Find free slot
    for (auto& slot : room->slots) {
        bool wantObserver = asObserver || slot.state == SlotState::Observer;
        if (asObserver && slot.state != SlotState::Observer) continue;
        if (!asObserver && slot.state != SlotState::Open) continue;
        slot.state       = SlotState::Occupied;
        slot.playerId    = playerId;
        slot.displayName = name;
        slot.isReady     = false;
        slot.joinedAtMs  = lobbyNowMs();
        // First player becomes host
        if (room->hostId == kInvalidPlayer && !asObserver) {
            room->hostId    = playerId;
            slot.isHost     = true;
        }
        emit({LobbyEvent::Type::PlayerJoined, id, playerId});
        return true;
    }
    return false; // No free slot
}

bool LobbySystem::LeaveRoom(RoomID id, PlayerID playerId) {
    auto* room = GetRoom(id);
    if (!room) return false;
    auto* slot = room->GetSlot(playerId);
    if (!slot) return false;
    bool wasHost = slot->isHost;
    slot->state       = SlotState::Open;
    slot->playerId    = kInvalidPlayer;
    slot->displayName = {};
    slot->isReady     = false;
    slot->isHost      = false;
    emit({LobbyEvent::Type::PlayerLeft, id, playerId});
    if (wasHost) {
        room->hostId = kInvalidPlayer;
        MigrateHost(id);
    }
    return true;
}

bool LobbySystem::KickPlayer(RoomID id, PlayerID hostId, PlayerID targetId) {
    auto* room = GetRoom(id);
    if (!room || room->hostId != hostId) return false;
    auto* slot = room->GetSlot(targetId);
    if (!slot) return false;
    slot->state    = SlotState::Open;
    slot->playerId = kInvalidPlayer;
    slot->displayName = {};
    slot->isReady  = false;
    emit({LobbyEvent::Type::PlayerKicked, id, targetId});
    return true;
}

bool LobbySystem::SetReady(RoomID id, PlayerID playerId, bool ready) {
    auto* room = GetRoom(id);
    if (!room) return false;
    auto* slot = room->GetSlot(playerId);
    if (!slot || slot->state != SlotState::Occupied) return false;
    slot->isReady = ready;
    emit({LobbyEvent::Type::PlayerReady, id, playerId});
    return true;
}

bool LobbySystem::CloseSlot(RoomID id, PlayerID hostId, uint32_t slotIndex) {
    auto* room = GetRoom(id);
    if (!room || room->hostId != hostId) return false;
    if (slotIndex >= room->slots.size()) return false;
    auto& slot = room->slots[slotIndex];
    if (slot.state == SlotState::Open) slot.state = SlotState::Closed;
    emit({LobbyEvent::Type::SlotChanged, id, kInvalidPlayer});
    return true;
}

bool LobbySystem::OpenSlot(RoomID id, PlayerID hostId, uint32_t slotIndex) {
    auto* room = GetRoom(id);
    if (!room || room->hostId != hostId) return false;
    if (slotIndex >= room->slots.size()) return false;
    auto& slot = room->slots[slotIndex];
    if (slot.state == SlotState::Closed) slot.state = SlotState::Open;
    emit({LobbyEvent::Type::SlotChanged, id, kInvalidPlayer});
    return true;
}

bool LobbySystem::PromoteHost(RoomID id, PlayerID currentHostId, PlayerID newHostId) {
    auto* room = GetRoom(id);
    if (!room || room->hostId != currentHostId) return false;
    auto* oldSlot = room->GetSlot(currentHostId);
    auto* newSlot = room->GetSlot(newHostId);
    if (!newSlot || newSlot->state != SlotState::Occupied) return false;
    if (oldSlot) oldSlot->isHost = false;
    newSlot->isHost = true;
    room->hostId    = newHostId;
    emit({LobbyEvent::Type::HostChanged, id, newHostId});
    return true;
}

bool LobbySystem::MigrateHost(RoomID id) {
    auto* room = GetRoom(id);
    if (!room) return false;
    // Pick oldest player (smallest joinedAtMs)
    PlayerSlot* best = nullptr;
    for (auto& s : room->slots) {
        if (s.state == SlotState::Occupied) {
            if (!best || s.joinedAtMs < best->joinedAtMs) best = &s;
        }
    }
    if (!best) return false;
    best->isHost  = true;
    room->hostId  = best->playerId;
    emit({LobbyEvent::Type::HostChanged, id, best->playerId});
    return true;
}

bool LobbySystem::StartGame(RoomID id, PlayerID hostId) {
    auto* room = GetRoom(id);
    if (!room || room->hostId != hostId) return false;
    room->phase = RoomPhase::Starting;
    emit({LobbyEvent::Type::GameStarting, id, hostId});
    return true;
}

bool LobbySystem::SetPhase(RoomID id, RoomPhase phase) {
    auto* room = GetRoom(id);
    if (!room) return false;
    room->phase = phase;
    if (phase == RoomPhase::InProgress)
        emit({LobbyEvent::Type::GameStarted, id, kInvalidPlayer});
    return true;
}

void LobbySystem::OnEvent(LobbyEventCb cb) { m_impl->callbacks.push_back(std::move(cb)); }

void LobbySystem::Update(float dt) {
    // Drive countdown timers for Starting rooms
    for (auto& [id, room] : m_impl->rooms) {
        if (room.phase != RoomPhase::Starting) continue;
        room.countdownSeconds = (room.countdownSeconds > 0)
            ? room.countdownSeconds - 1
            : 0;
        if (room.countdownSeconds == 0) {
            room.phase = RoomPhase::InProgress;
            emit({LobbyEvent::Type::GameStarted, id, kInvalidPlayer});
        }
    }
}

} // namespace Runtime::Multiplayer
