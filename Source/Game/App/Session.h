#pragma once
#include <cstdint>
#include <string>

namespace NF::Game {

/// @brief Connection states for the game session lifecycle.
enum class SessionState : uint8_t {
    Disconnected  = 0, ///< No active session.
    Connecting    = 1, ///< Handshake in progress.
    Connected     = 2, ///< Session is live.
    Disconnecting = 3, ///< Teardown in progress.
};

/// @brief Tracks the active game session and its connection state.
///
/// Bootstrap drives the state machine via the On*() callbacks; gameplay
/// code queries @c IsConnected() before performing session-authenticated
/// operations.
class Session {
public:
    Session() = default;

    // -------------------------------------------------------------------------
    // State queries
    // -------------------------------------------------------------------------

    /// @brief Returns the current connection state.
    [[nodiscard]] SessionState GetState()       const noexcept;

    /// @brief Returns true when the session is in the Connected state.
    [[nodiscard]] bool         IsConnected()    const noexcept;

    /// @brief Returns the active session token (empty when not connected).
    [[nodiscard]] const std::string& GetToken() const noexcept;

    // -------------------------------------------------------------------------
    // State transitions (called by Bootstrap)
    // -------------------------------------------------------------------------

    /// @brief Transition to Connecting and clear any previous token.
    void OnConnecting();

    /// @brief Transition to Connected and store @p token.
    /// @param token Session token received from the host service.
    void OnConnected(std::string token);

    /// @brief Transition to Disconnecting.
    void OnDisconnecting();

    /// @brief Transition to Disconnected and clear the token.
    void OnDisconnected();

    /// @brief Reset to Disconnected without triggering callbacks.
    void Reset();

private:
    SessionState m_State = SessionState::Disconnected;
    std::string  m_Token;
};

} // namespace NF::Game
