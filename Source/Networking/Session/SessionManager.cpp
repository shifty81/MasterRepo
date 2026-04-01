#include "Networking/Session/SessionManager.h"

namespace NF {

bool SessionManager::Connect(const std::string& host, uint16_t port) {
    if (m_State == SessionState::Connected) return false;

    m_State        = SessionState::Connecting;
    m_ConnectTimer = 0.f;

    if (m_Socket.Connect(host, port)) {
        m_State = SessionState::Connected;
        return true;
    }

    m_State = SessionState::Disconnected;
    return false;
}

void SessionManager::Disconnect() {
    m_Socket.Close();
    m_State = SessionState::Disconnected;
    m_SendQueue.clear();
}

SessionState SessionManager::GetState() const noexcept {
    return m_State;
}

void SessionManager::Send(const std::vector<uint8_t>& data) {
    if (m_State != SessionState::Connected) return;
    m_SendQueue.insert(m_SendQueue.end(), data.begin(), data.end());
}

void SessionManager::Update(float /*dt*/) {
    if (m_State != SessionState::Connected) return;

    if (!m_SendQueue.empty()) {
        m_Socket.Send(m_SendQueue.data(), m_SendQueue.size());
        m_SendQueue.clear();
    }
}

} // namespace NF
