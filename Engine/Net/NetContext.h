#pragma once
#include "Engine/Network/NetworkManager.h"
#include <string>
#include <functional>
#include <cstdint>

/// Engine/Net/NetContext.h
/// Thin adapter that wires a NetworkManager into the Engine::Core runtime.
///
/// Engine::Core::Engine holds a NetContext member when compiled with
/// ATLAS_ENABLE_NET (always true in this codebase).  The NetContext
/// owns the NetworkManager lifetime and exposes a higher-level API
/// (role-aware host/client helpers, integrated tick callback).

namespace Engine::net {

/// Role this runtime is playing on the network.
enum class NetRole {
    None,          ///< Networking disabled
    ListenServer,  ///< Acts as host + local client
    DedicatedServer, ///< Headless server only
    Client,        ///< Remote client connecting to a server
};

using NetEventCallback = ::Engine::NetEventCallback;
using Packet           = ::Engine::Packet;
using PeerID           = ::Engine::PeerID;

/// NetContext — owned by Engine::Core::Engine.
///
/// Wraps NetworkManager; provides role-aware Start/Stop helpers and an
/// integrated Poll() that can be called once per engine tick.
class NetContext {
public:
    NetContext();
    ~NetContext();

    // ── configuration ─────────────────────────────────────────
    void SetRole(NetRole role);
    NetRole GetRole() const;
    /// Server listen port (used in ListenServer / DedicatedServer roles).
    void SetPort(uint16_t port);
    uint16_t GetPort() const;
    /// Remote host address (used in Client role).
    void SetRemoteHost(const std::string& host);
    std::string GetRemoteHost() const;
    /// Max peers accepted when hosting (default 16).
    void SetMaxPeers(int max);

    // ── lifecycle ─────────────────────────────────────────────
    /// Start networking according to current role.
    bool Start();
    /// Stop and release all network resources.
    void Stop();
    bool IsRunning() const;

    // ── per-tick ──────────────────────────────────────────────
    /// Call once per engine tick; dispatches events to registered callbacks.
    void Poll();

    // ── messaging ─────────────────────────────────────────────
    void Broadcast(const std::vector<uint8_t>& data, bool reliable = true);
    void SendTo(PeerID peer, const std::vector<uint8_t>& data, bool reliable = true);

    // ── callbacks ─────────────────────────────────────────────
    void OnEvent(NetEventCallback cb);

    // ── accessor (for advanced use) ───────────────────────────
    NetworkManager& GetManager();
    const NetworkManager& GetManager() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine::net
