#include "Game/Net/GameServer.h"
#include "Core/Logging/Log.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

bool GameServer::Init(GameWorld* world, uint16_t port)
{
    if (!world) return false;

    m_World = world;
    m_Port  = port;
    m_Tick  = 0;
    m_NextClientId = 1;
    m_Running = true;

    NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                    "Server initialised on port " + std::to_string(port));
    return true;
}

void GameServer::Shutdown()
{
    if (!m_Running) return;

    m_Clients.clear();
    m_Running = false;
    NF::Logger::Log(NF::LogLevel::Info, "GameServer", "Server shutdown");
}

// ---------------------------------------------------------------------------
// AddLocalClient
// ---------------------------------------------------------------------------

uint32_t GameServer::AddLocalClient(const std::string& playerName)
{
    const uint32_t id = m_NextClientId++;

    auto client = std::make_unique<ConnectedClient>();
    client->ClientId   = id;
    client->PlayerName = playerName;
    client->Connected  = true;

    // Spawn at world spawn point.
    const auto& sp = m_World->GetSpawnPoint();
    client->Movement.SetPosition({sp.Position.X,
                                  sp.Position.Y + 2.f,
                                  sp.Position.Z});

    client->LastState.ClientId = id;
    client->LastState.Position = client->Movement.GetPosition();
    client->LastState.Health   = 100.f;
    client->LastState.Energy   = 100.f;

    m_Clients.emplace(id, std::move(client));

    NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                    "Local client '" + playerName + "' assigned id " + std::to_string(id));
    return id;
}

// ---------------------------------------------------------------------------
// SubmitLocalInput
// ---------------------------------------------------------------------------

void GameServer::SubmitLocalInput(uint32_t clientId, const NetClientInput& input)
{
    auto it = m_Clients.find(clientId);
    if (it == m_Clients.end()) return;
    it->second->LastInput = input;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void GameServer::Tick(float dt)
{
    if (!m_Running) return;

    ++m_Tick;

    // 1. Apply each client's input to their authoritative movement controller.
    for (auto& [id, client] : m_Clients)
    {
        if (!client->Connected) continue;

        auto& inp = client->LastInput;
        client->Movement.SetMoveInput(inp.Forward, inp.Right, inp.Jump, inp.Sprint);

        if (inp.MouseDeltaX != 0.f || inp.MouseDeltaY != 0.f)
            client->Movement.ApplyMouseLook(inp.MouseDeltaX, inp.MouseDeltaY);

        // Clear edge-triggered flags after consumption.
        inp.Jump = false;
        inp.MouseDeltaX = 0.f;
        inp.MouseDeltaY = 0.f;
    }

    // 2. Tick all player movements against the authoritative world.
    for (auto& [id, client] : m_Clients)
    {
        if (!client->Connected) continue;
        client->Movement.Update(dt, m_World->GetChunkMap());
    }

    // 3. Update per-client state snapshots.
    for (auto& [id, client] : m_Clients)
    {
        auto& s = client->LastState;
        s.ClientId = client->ClientId;
        s.Position = client->Movement.GetPosition();
        s.Yaw      = client->Movement.GetYaw();
        s.Pitch    = client->Movement.GetPitch();
        s.Grounded = client->Movement.IsGrounded();
        // Health/Energy are maintained by the rig state (unchanged here).
    }

    // 4. Build the snapshot.
    auto players = GatherPlayerStates();
    auto& pendingEdits = const_cast<std::vector<NetVoxelEdit>&>(m_Replicator.GetPendingEdits());
    m_LastSnapshot = m_Replicator.BuildSnapshot(m_Tick, players, pendingEdits);
}

// ---------------------------------------------------------------------------
// GetClientState
// ---------------------------------------------------------------------------

const NetPlayerState* GameServer::GetClientState(uint32_t clientId) const
{
    auto it = m_Clients.find(clientId);
    if (it == m_Clients.end()) return nullptr;
    return &it->second->LastState;
}

// ---------------------------------------------------------------------------
// GatherPlayerStates
// ---------------------------------------------------------------------------

std::vector<NetPlayerState> GameServer::GatherPlayerStates() const
{
    std::vector<NetPlayerState> states;
    states.reserve(m_Clients.size());
    for (const auto& [id, client] : m_Clients) {
        if (client->Connected)
            states.push_back(client->LastState);
    }
    return states;
}

} // namespace NF::Game
