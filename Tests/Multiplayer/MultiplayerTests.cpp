#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "Networking/Protocol/NetMessage.h"
#include "Networking/Channel/NetChannel.h"
#include "Networking/Transport/Socket.h"
#include "Game/Net/NetReplicator.h"
#include "Game/Net/GameServer.h"
#include "Game/Net/GameClient.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

#include "Game/App/Orchestrator.h"

using namespace NF;
using namespace NF::Game;
using Catch::Approx;

// Helper: get the port number assigned to a listening socket.
static uint16_t GetListenPort(Socket& listener) {
    struct sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    int rc = getsockname(listener.GetFD(),
                reinterpret_cast<struct sockaddr*>(&addr), &len);
    if (rc != 0) return 0;
    return ntohs(addr.sin_port);
}

// ============================================================================
// NetMessage — protocol layer
// ============================================================================

TEST_CASE("NetMessage: header size is 8 bytes", "[Multiplayer]") {
    REQUIRE(sizeof(NetMessageHeader) == 8);
}

TEST_CASE("NetMessage: Seal produces correct header + payload", "[Multiplayer]") {
    NetMessage msg(NetMessageType::ClientHello);
    msg.WriteString("TestPlayer");
    msg.Seal();

    REQUIRE(msg.GetType() == NetMessageType::ClientHello);
    REQUIRE(msg.GetPayloadSize() > 0);
    REQUIRE(msg.GetData().size() == sizeof(NetMessageHeader) + msg.GetPayloadSize());
}

TEST_CASE("NetMessage: round-trip from bytes", "[Multiplayer]") {
    // Build a message.
    auto original = MakeServerWelcome(42);

    // Reconstruct from raw bytes.
    NetMessage restored(original.GetData().data(), original.GetData().size());

    REQUIRE(restored.GetType() == NetMessageType::ServerWelcome);
    REQUIRE(restored.GetPayloadSize() == sizeof(uint32_t));

    auto ar = restored.BeginRead();
    uint32_t clientId = 0;
    ar.Serialize(clientId);
    REQUIRE(clientId == 42);
}

TEST_CASE("NetMessage: ClientHello carries player name", "[Multiplayer]") {
    auto msg = MakeClientHello("Nova");
    NetMessage restored(msg.GetData().data(), msg.GetData().size());

    auto ar = restored.BeginRead();
    uint32_t len = 0;
    ar.Serialize(len);
    REQUIRE(len == 4);
}

TEST_CASE("NetMessage: ServerPing round-trip", "[Multiplayer]") {
    auto ping = MakeServerPing(99);
    NetMessage restored(ping.GetData().data(), ping.GetData().size());
    REQUIRE(restored.GetType() == NetMessageType::ServerPing);

    auto ar = restored.BeginRead();
    uint32_t seq = 0;
    ar.Serialize(seq);
    REQUIRE(seq == 99);
}

TEST_CASE("NetMessage: empty payload", "[Multiplayer]") {
    NetMessage msg(NetMessageType::ClientDisconnect);
    msg.Seal();

    REQUIRE(msg.GetPayloadSize() == 0);
    REQUIRE(msg.GetData().size() == sizeof(NetMessageHeader));
}

// ============================================================================
// NetChannel — framing layer
// ============================================================================

TEST_CASE("NetChannel: queue and flush without socket does not crash", "[Multiplayer]") {
    NetChannel channel;
    auto msg = MakeServerPing(1);
    channel.QueueSend(msg);

    // No socket attached — flush should return -1.
    REQUIRE(channel.Flush() == -1);
}

TEST_CASE("NetChannel: attached to listening socket reports connected", "[Multiplayer]") {
    Socket listener;
    REQUIRE(listener.Listen(0)); // OS picks a free port
    REQUIRE(listener.IsConnected());

    NetChannel channel;
    channel.Attach(&listener);

    REQUIRE(channel.IsConnected());
    listener.Close();
}

TEST_CASE("NetChannel: BytesSent starts at zero", "[Multiplayer]") {
    NetChannel channel;
    REQUIRE(channel.BytesSent() == 0);
    REQUIRE(channel.BytesReceived() == 0);
}

TEST_CASE("NetChannel: ReceivedCount starts at zero", "[Multiplayer]") {
    NetChannel channel;
    REQUIRE(channel.ReceivedCount() == 0);
}

TEST_CASE("NetChannel: PopReceived returns false when empty", "[Multiplayer]") {
    NetChannel channel;
    NetMessage msg;
    REQUIRE_FALSE(channel.PopReceived(msg));
}

// ============================================================================
// NetReplicator — snapshot building and serialisation
// ============================================================================

TEST_CASE("NetReplicator: BuildSnapshot increments sequence", "[Multiplayer]") {
    NetReplicator rep;

    std::vector<NetPlayerState> players;
    std::vector<NetVoxelEdit> edits;

    auto snap1 = rep.BuildSnapshot(1, players, edits);
    auto snap2 = rep.BuildSnapshot(2, players, edits);

    REQUIRE(snap1.SnapshotSeq == 0);
    REQUIRE(snap2.SnapshotSeq == 1);
}

TEST_CASE("NetReplicator: BuildSnapshot consumes pending edits", "[Multiplayer]") {
    NetReplicator rep;

    rep.RecordVoxelEdit(10, 5, 3, 1);
    rep.RecordVoxelEdit(11, 5, 3, 2);
    REQUIRE(rep.GetPendingEdits().size() == 2);

    std::vector<NetPlayerState> players;
    auto& pendingRef = const_cast<std::vector<NetVoxelEdit>&>(rep.GetPendingEdits());
    auto snap = rep.BuildSnapshot(1, players, pendingRef);

    REQUIRE(snap.VoxelEdits.size() == 2);
    REQUIRE(rep.GetPendingEdits().empty());
}

TEST_CASE("NetReplicator: ApplySnapshot rejects stale", "[Multiplayer]") {
    NetReplicator rep;

    NetWorldSnapshot snap1;
    snap1.SnapshotSeq = 5;
    REQUIRE(rep.ApplySnapshot(snap1));

    NetWorldSnapshot snap2;
    snap2.SnapshotSeq = 3; // stale
    REQUIRE_FALSE(rep.ApplySnapshot(snap2));
}

TEST_CASE("NetReplicator: serialize/deserialize snapshot round-trip", "[Multiplayer]") {
    NetWorldSnapshot snapshot;
    snapshot.Tick        = 42;
    snapshot.SnapshotSeq = 7;

    // Add a player.
    NetPlayerState player;
    player.ClientId   = 1;
    player.Position   = {10.f, 20.f, 30.f};
    player.Yaw        = 1.5f;
    player.Pitch      = -0.3f;
    player.Health     = 80.f;
    player.Energy     = 60.f;
    player.Grounded   = true;
    player.ToolSlot   = 2;
    snapshot.Players.push_back(player);

    // Add a voxel edit.
    NetVoxelEdit edit;
    edit.WorldX   = 5;
    edit.WorldY   = 10;
    edit.WorldZ   = 15;
    edit.NewType  = 3;
    edit.Sequence = 1;
    snapshot.VoxelEdits.push_back(edit);

    // Serialize.
    auto bytes = NetReplicator::SerializeSnapshot(snapshot);
    REQUIRE(!bytes.empty());

    // Deserialize.
    auto restored = NetReplicator::DeserializeSnapshot(bytes.data(), bytes.size());

    REQUIRE(restored.Tick == 42);
    REQUIRE(restored.SnapshotSeq == 7);
    REQUIRE(restored.Players.size() == 1);
    REQUIRE(restored.Players[0].ClientId == 1);
    REQUIRE(restored.Players[0].Position.X == Approx(10.f));
    REQUIRE(restored.Players[0].Position.Y == Approx(20.f));
    REQUIRE(restored.Players[0].Position.Z == Approx(30.f));
    REQUIRE(restored.Players[0].Yaw == Approx(1.5f));
    REQUIRE(restored.Players[0].Pitch == Approx(-0.3f));
    REQUIRE(restored.Players[0].Health == Approx(80.f));
    REQUIRE(restored.Players[0].Energy == Approx(60.f));
    REQUIRE(restored.Players[0].Grounded == true);
    REQUIRE(restored.Players[0].ToolSlot == 2);

    REQUIRE(restored.VoxelEdits.size() == 1);
    REQUIRE(restored.VoxelEdits[0].WorldX == 5);
    REQUIRE(restored.VoxelEdits[0].WorldY == 10);
    REQUIRE(restored.VoxelEdits[0].WorldZ == 15);
    REQUIRE(restored.VoxelEdits[0].NewType == 3);
    REQUIRE(restored.VoxelEdits[0].Sequence == 1);
}

TEST_CASE("NetReplicator: empty snapshot serialization", "[Multiplayer]") {
    NetWorldSnapshot snapshot;
    snapshot.Tick = 0;
    snapshot.SnapshotSeq = 0;

    auto bytes = NetReplicator::SerializeSnapshot(snapshot);
    auto restored = NetReplicator::DeserializeSnapshot(bytes.data(), bytes.size());

    REQUIRE(restored.Tick == 0);
    REQUIRE(restored.Players.empty());
    REQUIRE(restored.VoxelEdits.empty());
}

TEST_CASE("NetReplicator: RecordVoxelEdit assigns sequences", "[Multiplayer]") {
    NetReplicator rep;
    rep.RecordVoxelEdit(1, 2, 3, 4);
    rep.RecordVoxelEdit(5, 6, 7, 8);

    const auto& edits = rep.GetPendingEdits();
    REQUIRE(edits.size() == 2);
    REQUIRE(edits[0].Sequence == 0);
    REQUIRE(edits[1].Sequence == 1);
}

// ============================================================================
// GameServer — authoritative server
// ============================================================================

TEST_CASE("GameServer: Init succeeds with valid world", "[Multiplayer]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    REQUIRE(server.Init(&world, 0));
    REQUIRE(server.IsRunning());
    REQUIRE(server.ClientCount() == 0);

    server.Shutdown();
    REQUIRE_FALSE(server.IsRunning());
}

TEST_CASE("GameServer: Init fails with null world", "[Multiplayer]") {
    GameServer server;
    REQUIRE_FALSE(server.Init(nullptr, 0));
}

TEST_CASE("GameServer: AddLocalClient assigns incrementing ids", "[Multiplayer]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    server.Init(&world, 0);

    uint32_t id1 = server.AddLocalClient("Player1");
    uint32_t id2 = server.AddLocalClient("Player2");

    REQUIRE(id1 == 1);
    REQUIRE(id2 == 2);
    REQUIRE(server.ClientCount() == 2);

    server.Shutdown();
}

TEST_CASE("GameServer: Tick advances tick counter", "[Multiplayer]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    server.Init(&world, 0);
    server.AddLocalClient("Player1");

    REQUIRE(server.GetTick() == 0);

    server.Tick(1.f / 60.f);
    REQUIRE(server.GetTick() == 1);

    server.Tick(1.f / 60.f);
    REQUIRE(server.GetTick() == 2);

    server.Shutdown();
}

TEST_CASE("GameServer: GetClientState returns valid state after tick", "[Multiplayer]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    server.Init(&world, 0);
    uint32_t id = server.AddLocalClient("Player1");

    server.Tick(1.f / 60.f);

    const auto* state = server.GetClientState(id);
    REQUIRE(state != nullptr);
    REQUIRE(state->ClientId == id);

    server.Shutdown();
}

TEST_CASE("GameServer: SubmitLocalInput moves player", "[Multiplayer]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    server.Init(&world, 0);
    uint32_t id = server.AddLocalClient("Player1");

    // Get initial position.
    server.Tick(1.f / 60.f);
    const auto* state0 = server.GetClientState(id);
    float initialZ = state0->Position.Z;

    // Submit forward input for several frames.
    for (int i = 0; i < 30; ++i) {
        NetClientInput input;
        input.ClientId = id;
        input.Forward  = 1.f;
        server.SubmitLocalInput(id, input);
        server.Tick(1.f / 60.f);
    }

    const auto* state1 = server.GetClientState(id);
    // Player should have moved forward (Z axis at yaw=0).
    REQUIRE(state1->Position.Z != Approx(initialZ).margin(0.1f));

    server.Shutdown();
}

TEST_CASE("GameServer: snapshot contains all players", "[Multiplayer]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    server.Init(&world, 0);
    server.AddLocalClient("P1");
    server.AddLocalClient("P2");

    server.Tick(1.f / 60.f);

    const auto& snap = server.GetLastSnapshot();
    REQUIRE(snap.Players.size() == 2);
    REQUIRE(snap.Tick == 1);

    server.Shutdown();
}

// ============================================================================
// GameClient — client lifecycle
// ============================================================================

TEST_CASE("GameClient: initial state is Disconnected", "[Multiplayer]") {
    GameClient client;
    REQUIRE(client.GetState() == GameClientState::Disconnected);
    REQUIRE_FALSE(client.IsConnected());
    REQUIRE(client.GetClientId() == 0);
}

TEST_CASE("GameClient: Connect to real server succeeds", "[Multiplayer]") {
    // Stand up a server on an ephemeral port.
    GameWorld world;
    world.Initialize("Content");
    GameServer server;
    REQUIRE(server.Init(&world, 0)); // port=0, no listener

    // For a direct-connect test we need a listener.  Use raw Socket.
    Socket listener;
    REQUIRE(listener.Listen(0)); // OS picks free port
    uint16_t port = GetListenPort(listener);
    REQUIRE(port > 0);

    listener.SetNonBlocking(true);

    GameClient client;
    REQUIRE(client.Connect("127.0.0.1", port, "TestPlayer"));
    // Client should be in Connecting (waiting for ServerWelcome).
    REQUIRE(client.GetState() == GameClientState::Connecting);

    client.Disconnect();
    REQUIRE_FALSE(client.IsConnected());
    listener.Close();
    server.Shutdown();
}

TEST_CASE("GameClient: double Connect returns false", "[Multiplayer]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    listener.SetNonBlocking(true);
    uint16_t port = GetListenPort(listener);

    GameClient client;
    client.Connect("127.0.0.1", port, "Player1");
    REQUIRE_FALSE(client.Connect("127.0.0.1", port, "Player2"));

    client.Disconnect();
    listener.Close();
}

TEST_CASE("GameClient: Update does not crash when connected", "[Multiplayer]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    listener.SetNonBlocking(true);
    uint16_t port = GetListenPort(listener);

    GameClient client;
    client.Connect("127.0.0.1", port, "TestPlayer");

    // Update several frames — should not crash.
    for (int i = 0; i < 10; ++i)
        client.Update(1.f / 60.f);

    client.Disconnect();
    listener.Close();
}

TEST_CASE("GameClient: SubmitInput does not crash when connected", "[Multiplayer]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    listener.SetNonBlocking(true);
    uint16_t port = GetListenPort(listener);

    GameClient client;
    client.Connect("127.0.0.1", port, "TestPlayer");

    // Must be Connected (not Connecting) to submit input.
    // Since there's no welcome, SubmitInput silently drops — no crash.
    NetClientInput input;
    input.Forward = 1.f;
    input.Jump    = true;
    client.SubmitInput(input);

    client.Update(1.f / 60.f);
    client.Disconnect();
    listener.Close();
}

TEST_CASE("GameClient: GetLocalPlayerState returns nullptr before snapshot", "[Multiplayer]") {
    GameClient client;
    REQUIRE(client.GetLocalPlayerState() == nullptr);
}

// ============================================================================
// Phase 7 — Platform Socket tests
// ============================================================================

TEST_CASE("Socket: default-constructed is not connected", "[Phase7]") {
    Socket s;
    REQUIRE_FALSE(s.IsConnected());
    REQUIRE(s.GetFD() == -1);
}

TEST_CASE("Socket: Listen on port 0 picks ephemeral port", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    REQUIRE(listener.IsConnected());
    REQUIRE(listener.GetFD() >= 0);
    listener.Close();
    REQUIRE_FALSE(listener.IsConnected());
}

TEST_CASE("Socket: Connect to listener succeeds", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    uint16_t port = GetListenPort(listener);
    listener.SetNonBlocking(true);

    Socket client;
    REQUIRE(client.Connect("127.0.0.1", port));
    REQUIRE(client.IsConnected());

    Socket accepted = listener.Accept();
    REQUIRE(accepted.IsConnected());

    client.Close();
    accepted.Close();
    listener.Close();
}

TEST_CASE("Socket: Send and Receive round-trip", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    uint16_t port = GetListenPort(listener);
    listener.SetNonBlocking(true);

    Socket client;
    REQUIRE(client.Connect("127.0.0.1", port));

    Socket server = listener.Accept();
    REQUIRE(server.IsConnected());

    // Send from client to server.
    const char hello[] = "Hello!";
    int sent = client.Send(hello, sizeof(hello));
    REQUIRE(sent == static_cast<int>(sizeof(hello)));

    // Small delay for data to transit.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    char buf[64]{};
    int received = server.Receive(buf, sizeof(buf));
    REQUIRE(received == static_cast<int>(sizeof(hello)));
    REQUIRE(std::string(buf) == "Hello!");

    client.Close();
    server.Close();
    listener.Close();
}

TEST_CASE("Socket: NonBlocking accept returns invalid when no pending", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    listener.SetNonBlocking(true);

    Socket result = listener.Accept();
    REQUIRE_FALSE(result.IsConnected());

    listener.Close();
}

TEST_CASE("Socket: Connect to nothing fails", "[Phase7]") {
    Socket s;
    // Use a high ephemeral port that is almost certainly not listening.
    REQUIRE_FALSE(s.Connect("127.0.0.1", 65534));
    REQUIRE_FALSE(s.IsConnected());
}

TEST_CASE("Socket: move semantics", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    int fd = listener.GetFD();

    Socket moved(std::move(listener));
    REQUIRE(moved.GetFD() == fd);
    REQUIRE(moved.IsConnected());
    REQUIRE_FALSE(listener.IsConnected());
    REQUIRE(listener.GetFD() == -1);

    moved.Close();
}

// ============================================================================
// Phase 7 — NetChannel over real sockets
// ============================================================================

TEST_CASE("NetChannel: send and receive over real socket", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    uint16_t port = GetListenPort(listener);
    listener.SetNonBlocking(true);

    Socket clientSock;
    REQUIRE(clientSock.Connect("127.0.0.1", port));

    Socket serverSock = listener.Accept();
    REQUIRE(serverSock.IsConnected());

    // Non-blocking for poll.
    clientSock.SetNonBlocking(true);
    serverSock.SetNonBlocking(true);

    // Send a message from server to client via NetChannel.
    NetChannel serverCh;
    serverCh.Attach(&serverSock);

    auto welcome = MakeServerWelcome(42);
    serverCh.QueueSend(welcome);
    REQUIRE(serverCh.Flush() >= 0);

    // Small delay for data to transit.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    NetChannel clientCh;
    clientCh.Attach(&clientSock);

    int count = clientCh.Poll();
    REQUIRE(count == 1);

    NetMessage received;
    REQUIRE(clientCh.PopReceived(received));
    REQUIRE(received.GetType() == NetMessageType::ServerWelcome);

    auto ar = received.BeginRead();
    uint32_t id = 0;
    ar.Serialize(id);
    REQUIRE(id == 42);

    clientSock.Close();
    serverSock.Close();
    listener.Close();
}

TEST_CASE("NetChannel: bidirectional message exchange", "[Phase7]") {
    Socket listener;
    REQUIRE(listener.Listen(0));
    uint16_t port = GetListenPort(listener);
    listener.SetNonBlocking(true);

    Socket clientSock;
    REQUIRE(clientSock.Connect("127.0.0.1", port));

    Socket serverSock = listener.Accept();
    clientSock.SetNonBlocking(true);
    serverSock.SetNonBlocking(true);

    NetChannel serverCh, clientCh;
    serverCh.Attach(&serverSock);
    clientCh.Attach(&clientSock);

    // Client sends hello.
    auto hello = MakeClientHello("TestPlayer");
    clientCh.QueueSend(hello);
    clientCh.Flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    REQUIRE(serverCh.Poll() == 1);
    NetMessage helloMsg;
    REQUIRE(serverCh.PopReceived(helloMsg));
    REQUIRE(helloMsg.GetType() == NetMessageType::ClientHello);

    // Server replies with welcome.
    auto welcomeReply = MakeServerWelcome(99);
    serverCh.QueueSend(welcomeReply);
    serverCh.Flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    REQUIRE(clientCh.Poll() == 1);
    NetMessage welcomeMsg;
    REQUIRE(clientCh.PopReceived(welcomeMsg));
    REQUIRE(welcomeMsg.GetType() == NetMessageType::ServerWelcome);

    clientSock.Close();
    serverSock.Close();
    listener.Close();
}

// ============================================================================
// Phase 7 — GameServer real TCP accept + handshake
// ============================================================================

TEST_CASE("GameServer: listen on real port", "[Phase7]") {
    GameWorld world;
    world.Initialize("Content");

    GameServer server;
    // Use port 0 to let OS pick.  We can't do this directly because
    // GameServer::Init(port=0) means "no listener".  Use a port > 0.
    // Pick an ephemeral port that is likely free.
    // We'll use a listener Socket to find a free port, close it, then use it.
    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    REQUIRE(server.Init(&world, port));
    REQUIRE(server.IsRunning());
    REQUIRE(server.GetPort() == port);

    server.Shutdown();
}

TEST_CASE("GameServer: accept remote client and handshake", "[Phase7]") {
    GameWorld world;
    world.Initialize("Content");

    // Find a free port.
    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    GameServer server;
    REQUIRE(server.Init(&world, port));

    // Connect a client.
    GameClient client;
    REQUIRE(client.Connect("127.0.0.1", port, "RemotePlayer"));
    REQUIRE(client.GetState() == GameClientState::Connecting);

    // Server tick accepts the connection and sends welcome.
    server.Tick(1.f / 60.f);

    // Give data time to transit.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Client update processes welcome.
    client.Update(1.f / 60.f);

    REQUIRE(client.GetState() == GameClientState::Connected);
    REQUIRE(client.GetClientId() > 0);

    // Server should now have 1 client.
    REQUIRE(server.ClientCount() == 1);

    client.Disconnect();
    server.Shutdown();
}

TEST_CASE("GameServer: snapshot broadcast to remote client", "[Phase7]") {
    GameWorld world;
    world.Initialize("Content");

    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    GameServer server;
    REQUIRE(server.Init(&world, port));
    server.AddLocalClient("LocalHost"); // add a local player too

    GameClient client;
    REQUIRE(client.Connect("127.0.0.1", port, "RemotePlayer"));

    // Tick server to accept + welcome.
    server.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    client.Update(1.f / 60.f);

    REQUIRE(client.IsConnected());

    // Another server tick builds and broadcasts snapshot.
    server.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    client.Update(1.f / 60.f);

    // Client should have received a snapshot with 2 players (local + remote).
    const auto& snap = client.GetLastSnapshot();
    REQUIRE(snap.Players.size() == 2);

    // Local player state from snapshot.
    const auto* localState = client.GetLocalPlayerState();
    REQUIRE(localState != nullptr);

    client.Disconnect();
    server.Shutdown();
}

// ============================================================================
// Phase 7 — Orchestrator net-mode tests
// ============================================================================

TEST_CASE("Orchestrator: Solo mode backward compat", "[Phase7]") {
    Orchestrator orch;
    REQUIRE(orch.Init(nullptr));
    REQUIRE(orch.IsInitialized());
    REQUIRE(orch.GetNetMode() == NetMode::Solo);
    REQUIRE(orch.GetServer() != nullptr);
    REQUIRE(orch.GetClient() == nullptr);
    REQUIRE(orch.GetLocalClientId() > 0);

    orch.Tick(1.f / 60.f);
    orch.Shutdown();
    REQUIRE_FALSE(orch.IsInitialized());
}

TEST_CASE("Orchestrator: ListenServer mode", "[Phase7]") {
    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    NetParams params;
    params.Mode       = NetMode::ListenServer;
    params.Port       = port;
    params.PlayerName = "Host";

    Orchestrator orch;
    REQUIRE(orch.Init(nullptr, params));
    REQUIRE(orch.GetNetMode() == NetMode::ListenServer);
    REQUIRE(orch.GetServer() != nullptr);
    REQUIRE(orch.GetServer()->IsRunning());
    REQUIRE(orch.GetLocalClientId() > 0);

    orch.Tick(1.f / 60.f);
    orch.Shutdown();
}

TEST_CASE("Orchestrator: Dedicated mode", "[Phase7]") {
    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    NetParams params;
    params.Mode = NetMode::Dedicated;
    params.Port = port;

    Orchestrator orch;
    REQUIRE(orch.Init(nullptr, params));
    REQUIRE(orch.GetNetMode() == NetMode::Dedicated);
    REQUIRE(orch.GetServer() != nullptr);
    REQUIRE(orch.GetLocalClientId() == 0); // no local player in dedicated

    orch.Tick(1.f / 60.f);
    orch.Shutdown();
}

TEST_CASE("Orchestrator: Client mode connects to listen-server", "[Phase7]") {
    // Start a listen-server orchestrator.
    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    NetParams serverParams;
    serverParams.Mode       = NetMode::ListenServer;
    serverParams.Port       = port;
    serverParams.PlayerName = "Host";

    Orchestrator serverOrch;
    REQUIRE(serverOrch.Init(nullptr, serverParams));

    // Start a client orchestrator.
    NetParams clientParams;
    clientParams.Mode       = NetMode::Client;
    clientParams.Host       = "127.0.0.1";
    clientParams.Port       = port;
    clientParams.PlayerName = "RemoteClient";

    Orchestrator clientOrch;
    REQUIRE(clientOrch.Init(nullptr, clientParams));
    REQUIRE(clientOrch.GetNetMode() == NetMode::Client);
    REQUIRE(clientOrch.GetClient() != nullptr);

    // Tick server to accept and welcome.
    serverOrch.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    clientOrch.Tick(1.f / 60.f);

    REQUIRE(clientOrch.GetClient()->IsConnected());

    clientOrch.Shutdown();
    serverOrch.Shutdown();
}

// ============================================================================
// Phase 7 — Local loopback integration test
// ============================================================================

TEST_CASE("Integration: full loopback server+client handshake+snapshot", "[Phase7]") {
    GameWorld world;
    world.Initialize("Content");

    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    // Start server with a local player.
    GameServer server;
    REQUIRE(server.Init(&world, port));
    uint32_t hostId = server.AddLocalClient("Host");

    // Connect a remote client.
    GameClient client;
    REQUIRE(client.Connect("127.0.0.1", port, "Client1"));

    // --- Handshake ---
    // Server tick #1: accept + hello → welcome.
    server.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    client.Update(1.f / 60.f);

    REQUIRE(client.GetState() == GameClientState::Connected);
    uint32_t clientId = client.GetClientId();
    REQUIRE(clientId > 0);
    REQUIRE(clientId != hostId);

    // --- Input ---
    // Client sends forward movement.
    NetClientInput input;
    input.Forward = 1.f;
    client.SubmitInput(input);
    client.Update(0.f); // flush

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Server tick #2: receive input, apply, build snapshot, broadcast.
    server.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    client.Update(1.f / 60.f);

    // --- Verify snapshot ---
    const auto& snap = client.GetLastSnapshot();
    REQUIRE(snap.Players.size() == 2);

    const auto* localPlayer = client.GetLocalPlayerState();
    REQUIRE(localPlayer != nullptr);
    REQUIRE(localPlayer->ClientId == clientId);

    // --- Cleanup ---
    client.Disconnect();
    server.Shutdown();
}

TEST_CASE("Integration: multiple remote clients", "[Phase7]") {
    GameWorld world;
    world.Initialize("Content");

    Socket probe;
    REQUIRE(probe.Listen(0));
    uint16_t port = GetListenPort(probe);
    probe.Close();

    GameServer server;
    REQUIRE(server.Init(&world, port));

    // Connect 3 clients.
    GameClient c1, c2, c3;
    REQUIRE(c1.Connect("127.0.0.1", port, "Player1"));
    REQUIRE(c2.Connect("127.0.0.1", port, "Player2"));
    REQUIRE(c3.Connect("127.0.0.1", port, "Player3"));

    // Server tick to accept all.
    server.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    c1.Update(1.f / 60.f);
    c2.Update(1.f / 60.f);
    c3.Update(1.f / 60.f);

    REQUIRE(c1.IsConnected());
    REQUIRE(c2.IsConnected());
    REQUIRE(c3.IsConnected());
    REQUIRE(server.ClientCount() == 3);

    // All clients should have unique IDs.
    REQUIRE(c1.GetClientId() != c2.GetClientId());
    REQUIRE(c2.GetClientId() != c3.GetClientId());

    // Another tick to broadcast snapshot.
    server.Tick(1.f / 60.f);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    c1.Update(1.f / 60.f);
    c2.Update(1.f / 60.f);
    c3.Update(1.f / 60.f);

    REQUIRE(c1.GetLastSnapshot().Players.size() == 3);
    REQUIRE(c2.GetLastSnapshot().Players.size() == 3);
    REQUIRE(c3.GetLastSnapshot().Players.size() == 3);

    c1.Disconnect();
    c2.Disconnect();
    c3.Disconnect();
    server.Shutdown();
}
