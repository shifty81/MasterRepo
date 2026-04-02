#include "Game/Net/GameServer.h"
#include "Networking/Protocol/NetMessage.h"
#include "Core/Logging/Log.h"
#include <cstring>

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

    // If a real port is specified, start listening.
    if (port > 0)
    {
        if (!m_ListenSocket.Listen(port)) {
            NF::Logger::Log(NF::LogLevel::Error, "GameServer",
                            "Failed to listen on port " + std::to_string(port));
            return false;
        }
        m_ListenSocket.SetNonBlocking(true);
        NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                        "Listening on port " + std::to_string(port));
    }

    m_Running = true;
    NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                    "Server initialised on port " + std::to_string(port));
    return true;
}

void GameServer::Shutdown()
{
    if (!m_Running) return;

    m_Clients.clear();
    m_ListenSocket.Close();
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
    client->IsLocal    = true;
    client->Welcomed   = true; // local clients skip handshake

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
// AcceptNewConnections
// ---------------------------------------------------------------------------

void GameServer::AcceptNewConnections()
{
    if (m_Port == 0) return; // no listener

    // Accept up to kMaxAcceptsPerTick connections per tick.
    static constexpr int kMaxAcceptsPerTick = 4;
    for (int i = 0; i < kMaxAcceptsPerTick; ++i)
    {
        NF::Socket incoming = m_ListenSocket.Accept();
        if (!incoming.IsConnected()) break;

        incoming.SetNonBlocking(true);

        const uint32_t id = m_NextClientId++;

        auto client = std::make_unique<ConnectedClient>();
        client->ClientId     = id;
        client->Connected    = true;
        client->IsLocal      = false;
        client->Welcomed     = false;
        client->ClientSocket = std::move(incoming);
        client->Channel.Attach(&client->ClientSocket);

        // Spawn at world spawn point.
        const auto& sp = m_World->GetSpawnPoint();
        client->Movement.SetPosition({sp.Position.X,
                                      sp.Position.Y + 2.f,
                                      sp.Position.Z});

        client->LastState.ClientId = id;
        client->LastState.Position = client->Movement.GetPosition();
        client->LastState.Health   = 100.f;
        client->LastState.Energy   = 100.f;

        NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                        "Remote client connected, assigned id " + std::to_string(id));

        m_Clients.emplace(id, std::move(client));
    }
}

// ---------------------------------------------------------------------------
// ReceiveRemoteInput
// ---------------------------------------------------------------------------

void GameServer::ReceiveRemoteInput()
{
    std::vector<uint32_t> toRemove;

    for (auto& [id, client] : m_Clients)
    {
        if (client->IsLocal || !client->Connected) continue;

        // Poll the channel.
        client->Channel.Poll();

        NF::NetMessage msg;
        while (client->Channel.PopReceived(msg))
        {
            switch (msg.GetType())
            {
            case NF::NetMessageType::ClientHello:
            {
                // Parse player name and send welcome.
                auto ar = msg.BeginRead();
                uint32_t nameLen = 0;
                ar.Serialize(nameLen);

                // Clamp to a reasonable maximum to prevent abuse.
                static constexpr uint32_t kMaxPlayerNameLen = 128;
                if (nameLen > kMaxPlayerNameLen) nameLen = kMaxPlayerNameLen;

                std::string name(nameLen, '\0');
                if (nameLen > 0) {
                    for (uint32_t i = 0; i < nameLen; ++i) {
                        char c = 0;
                        ar.Serialize(c);
                        name[i] = c;
                    }
                }
                client->PlayerName = name;
                client->Welcomed   = true;

                // Reply with ServerWelcome.
                auto welcome = NF::MakeServerWelcome(client->ClientId);
                client->Channel.QueueSend(welcome);
                client->Channel.Flush();

                NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                                "Client " + std::to_string(id) + " hello: " + name);
                break;
            }
            case NF::NetMessageType::ClientInput:
            {
                auto ar = msg.BeginRead();
                uint32_t clientId = 0;
                ar.Serialize(clientId);

                auto& inp = client->LastInput;
                ar.Serialize(inp.Forward);
                ar.Serialize(inp.Right);
                uint8_t flags = 0;
                ar.Serialize(flags);
                inp.Jump   = (flags & 1) != 0;
                inp.Sprint = (flags & 2) != 0;
                ar.Serialize(inp.MouseDeltaX);
                ar.Serialize(inp.MouseDeltaY);
                ar.Serialize(inp.InputSeq);
                break;
            }
            case NF::NetMessageType::ClientDisconnect:
            {
                NF::Logger::Log(NF::LogLevel::Info, "GameServer",
                                "Client " + std::to_string(id) + " disconnected");
                client->Connected = false;
                toRemove.push_back(id);
                break;
            }
            case NF::NetMessageType::ClientPong:
                break; // TODO: track latency
            default:
                break;
            }
        }

        // Check if socket was lost.
        if (!client->ClientSocket.IsConnected()) {
            client->Connected = false;
            toRemove.push_back(id);
        }
    }

    for (uint32_t id : toRemove) m_Clients.erase(id);
}

// ---------------------------------------------------------------------------
// BroadcastSnapshot
// ---------------------------------------------------------------------------

void GameServer::BroadcastSnapshot()
{
    auto data = NetReplicator::SerializeSnapshot(m_LastSnapshot);
    if (data.empty()) return;

    NF::NetMessage msg(NF::NetMessageType::ServerSnapshot);
    // Write raw payload bytes.
    for (uint8_t b : data) msg.Write(b);
    msg.Seal();

    for (auto& [id, client] : m_Clients)
    {
        if (client->IsLocal || !client->Connected || !client->Welcomed) continue;

        client->Channel.QueueSend(msg);
        client->Channel.Flush();
    }
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void GameServer::Tick(float dt)
{
    if (!m_Running) return;

    ++m_Tick;

    // 1. Accept new TCP connections.
    AcceptNewConnections();

    // 2. Receive remote client input.
    ReceiveRemoteInput();

    // 3. Apply each client's input to their authoritative movement controller.
    for (auto& [id, client] : m_Clients)
    {
        if (!client->Connected) continue;

        auto& inp = client->LastInput;
        client->Movement.SetMoveInput(inp.Forward, inp.Right, inp.Jump, inp.Sprint);

        if (inp.MouseDeltaX != 0.f || inp.MouseDeltaY != 0.f)
            client->Movement.ApplyMouseLook(inp.MouseDeltaX, inp.MouseDeltaY);

        inp.Jump = false;
        inp.MouseDeltaX = 0.f;
        inp.MouseDeltaY = 0.f;
    }

    // 4. Tick all player movements against the authoritative world.
    for (auto& [id, client] : m_Clients)
    {
        if (!client->Connected) continue;
        client->Movement.Update(dt, m_World->GetChunkMap());
    }

    // 5. Update per-client state snapshots.
    for (auto& [id, client] : m_Clients)
    {
        auto& s = client->LastState;
        s.ClientId = client->ClientId;
        s.Position = client->Movement.GetPosition();
        s.Yaw      = client->Movement.GetYaw();
        s.Pitch    = client->Movement.GetPitch();
        s.Grounded = client->Movement.IsGrounded();
    }

    // 6. Build the snapshot.
    auto players = GatherPlayerStates();
    auto& pendingEdits = const_cast<std::vector<NetVoxelEdit>&>(m_Replicator.GetPendingEdits());
    m_LastSnapshot = m_Replicator.BuildSnapshot(m_Tick, players, pendingEdits);

    // 7. Broadcast snapshot to remote clients.
    BroadcastSnapshot();
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
