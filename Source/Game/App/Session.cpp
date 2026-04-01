#include "Game/App/Session.h"

namespace NF::Game {

SessionState Session::GetState()    const noexcept { return m_State; }
bool         Session::IsConnected() const noexcept { return m_State == SessionState::Connected; }

const std::string& Session::GetToken() const noexcept { return m_Token; }

void Session::OnConnecting()
{
    m_State = SessionState::Connecting;
    m_Token.clear();
}

void Session::OnConnected(std::string token)
{
    m_Token = std::move(token);
    m_State = SessionState::Connected;
}

void Session::OnDisconnecting()
{
    m_State = SessionState::Disconnecting;
}

void Session::OnDisconnected()
{
    m_State = SessionState::Disconnected;
    m_Token.clear();
}

void Session::Reset()
{
    m_State = SessionState::Disconnected;
    m_Token.clear();
}

} // namespace NF::Game
