#include "AtlasAICore.h"

namespace atlas::ai {

void AtlasAICore::SetPermissions(uint8_t permissions) {
    m_permissions = permissions;
}

uint8_t AtlasAICore::GetPermissions() const {
    return m_permissions;
}

bool AtlasAICore::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void AtlasAICore::RegisterIntent(const AIIntentHandler& handler) {
    m_intents[handler.name] = handler;
}

void AtlasAICore::UnregisterIntent(const std::string& name) {
    m_intents.erase(name);
}

const AIIntentHandler* AtlasAICore::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}

std::vector<std::string> AtlasAICore::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) {
        names.push_back(kv.first);
    }
    return names;
}

size_t AtlasAICore::IntentCount() const {
    return m_intents.size();
}

AIResponse AtlasAICore::ProcessRequest(const AIRequest& request) {
    AIRequest req = request;
    req.requestID = m_nextRequestID++;
    m_requestHistory.push_back(req);

    AIResponse response;
    response.requestID = req.requestID;

    auto it = m_intents.find(req.intentName);
    if (it == m_intents.end()) {
        response.success = false;
        response.errorMessage = "Unknown intent: " + req.intentName;
        m_responseHistory.push_back(response);
        return response;
    }

    const auto& handler = it->second;
    if ((handler.requiredPermissions & m_permissions) != handler.requiredPermissions) {
        response.success = false;
        response.errorMessage = "Insufficient permissions for intent: " + req.intentName;
        m_responseHistory.push_back(response);
        return response;
    }

    response = handler.handler(req);
    response.requestID = req.requestID;
    m_responseHistory.push_back(response);
    return response;
}

const std::vector<AIRequest>& AtlasAICore::RequestHistory() const {
    return m_requestHistory;
}

const std::vector<AIResponse>& AtlasAICore::ResponseHistory() const {
    return m_responseHistory;
}

size_t AtlasAICore::RequestCount() const {
    return m_requestHistory.size();
}

void AtlasAICore::Clear() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
}

} // namespace atlas::ai
