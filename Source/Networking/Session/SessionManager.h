#pragma once
#include "Networking/Transport/Socket.h"
#include <string>
#include <vector>
#include <cstdint>

namespace NF {

/// @brief Lifecycle states of a network session.
enum class SessionState {
    Idle,         ///< No connection attempted.
    Connecting,   ///< Connection in progress.
    Connected,    ///< Active connection.
    Disconnected  ///< Connection was closed or lost.
};

/// @brief Manages a single client network session.
class SessionManager {
public:
    /// @brief Begin connecting to a remote host.
    /// @return True if the connection attempt was initiated.
    bool Connect(const std::string& host, uint16_t port);

    /// @brief Gracefully close the current session.
    void Disconnect();

    /// @brief Current session state.
    [[nodiscard]] SessionState GetState() const noexcept;

    /// @brief Queue data to be sent on the next Update.
    void Send(const std::vector<uint8_t>& data);

    /// @brief Pump the session: flush queued data, handle timeouts.
    /// @param dt  Time delta in seconds.
    void Update(float dt);

private:
    Socket       m_Socket;
    SessionState m_State{SessionState::Idle};
    float        m_ConnectTimer{0.f};

    std::vector<uint8_t> m_SendQueue;
};

} // namespace NF
