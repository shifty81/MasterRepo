#include "Game/Net/GameClient.h"
#include "Networking/Protocol/NetMessage.h"
#include "Core/Logging/Log.h"
#include <cstring>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Connect / Disconnect
// ---------------------------------------------------------------------------

bool GameClient::Connect(const std::string& host, uint16_t port,
                          const std::string& playerName)
{
    if (m_State != GameClientState::Disconnected) return false;

    m_PlayerName = playerName;
    m_State      = GameClientState::Connecting;

    if (!m_Socket.Connect(host, port)) {
        m_State = GameClientState::Disconnected;
        NF::Logger::Log(NF::LogLevel::Error, "GameClient",
                        "Failed to connect to " + host + ":" + std::to_string(port));
        return false;
    }

    m_Socket.SetNonBlocking(true);
    m_Channel.Attach(&m_Socket);

    // Send ClientHello — stay in Connecting until we receive ServerWelcome.
    auto hello = NF::MakeClientHello(playerName);
    m_Channel.QueueSend(hello);
    m_Channel.Flush();

    NF::Logger::Log(NF::LogLevel::Info, "GameClient",
                    "Connecting to " + host + ":" + std::to_string(port));
    return true;
}

void GameClient::Disconnect()
{
    if (m_State == GameClientState::Disconnected) return;

    // Send disconnect message.
    NF::NetMessage msg(NF::NetMessageType::ClientDisconnect);
    msg.Seal();
    m_Channel.QueueSend(msg);
    m_Channel.Flush();

    m_Socket.Close();
    m_State    = GameClientState::Disconnected;
    m_ClientId = 0;
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "Disconnected");
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void GameClient::Update(float /*dt*/)
{
    if (m_State == GameClientState::Disconnected) return;

    // Flush outbound.
    m_Channel.Flush();

    // Poll inbound.
    m_Channel.Poll();

    // Process received messages.
    NF::NetMessage msg;
    while (m_Channel.PopReceived(msg))
    {
        switch (msg.GetType())
        {
        case NF::NetMessageType::ServerWelcome:
        {
            auto ar = msg.BeginRead();
            ar.Serialize(m_ClientId);
            m_State = GameClientState::Connected;
            NF::Logger::Log(NF::LogLevel::Info, "GameClient",
                            "Assigned client id " + std::to_string(m_ClientId));
            break;
        }

        case NF::NetMessageType::ServerSnapshot:
        {
            auto payload = msg.GetPayload();
            if (!payload.empty()) {
                auto snapshot = NetReplicator::DeserializeSnapshot(
                    payload.data(), payload.size());
                if (m_Replicator.ApplySnapshot(snapshot))
                    m_LastSnapshot = std::move(snapshot);
            }
            break;
        }

        case NF::NetMessageType::ServerPing:
        {
            auto ar = msg.BeginRead();
            uint32_t seq = 0;
            ar.Serialize(seq);
            auto pong = NF::MakeClientPong(seq);
            m_Channel.QueueSend(pong);
            break;
        }

        case NF::NetMessageType::ServerKick:
            Disconnect();
            break;

        default:
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// SubmitInput
// ---------------------------------------------------------------------------

void GameClient::SubmitInput(const NetClientInput& input)
{
    if (m_State != GameClientState::Connected) return;

    NF::NetMessage msg(NF::NetMessageType::ClientInput);

    msg.Write(m_ClientId);
    msg.Write(input.Forward);
    msg.Write(input.Right);
    uint8_t flags = (input.Jump ? 1 : 0) | (input.Sprint ? 2 : 0);
    msg.Write(flags);
    msg.Write(input.MouseDeltaX);
    msg.Write(input.MouseDeltaY);
    msg.Write(m_InputSeq++);
    msg.Seal();

    m_Channel.QueueSend(msg);
}

// ---------------------------------------------------------------------------
// GetLocalPlayerState
// ---------------------------------------------------------------------------

const NetPlayerState* GameClient::GetLocalPlayerState() const
{
    for (const auto& p : m_LastSnapshot.Players) {
        if (p.ClientId == m_ClientId)
            return &p;
    }
    return nullptr;
}

} // namespace NF::Game
