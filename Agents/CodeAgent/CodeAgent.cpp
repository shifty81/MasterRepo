#include "Agents/CodeAgent/CodeAgent.h"

namespace Agents {

void CodeAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t CodeAgent::GetPermissions() const { return m_permissions; }
bool CodeAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void CodeAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void CodeAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* CodeAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> CodeAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t CodeAgent::IntentCount() const { return m_intents.size(); }

AIResponse CodeAgent::ProcessRequest(const AIRequest& request) {
    m_status = AgentStatus::Planning;
    AIRequest req = request;
    req.requestID = m_nextRequestID++;
    m_requestHistory.push_back(req);

    AIResponse response;
    response.requestID = req.requestID;

    auto it = m_intents.find(req.intentName);
    if (it == m_intents.end()) {
        response.success = false;
        response.errorMessage = "Unknown intent: " + req.intentName;
        m_status = AgentStatus::Error;
        m_responseHistory.push_back(response);
        return response;
    }

    const auto& handler = it->second;
    if ((handler.requiredPermissions & m_permissions) != handler.requiredPermissions) {
        response.success = false;
        response.errorMessage = "Insufficient permissions for intent: " + req.intentName;
        m_status = AgentStatus::Error;
        m_responseHistory.push_back(response);
        return response;
    }

    m_status = AgentStatus::Executing;
    response = handler.handler(req);
    response.requestID = req.requestID;
    m_status = AgentStatus::Idle;
    m_responseHistory.push_back(response);
    return response;
}

AgentStatus CodeAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  CodeAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& CodeAgent::GetResponseHistory() const { return m_responseHistory; }
size_t CodeAgent::RequestCount() const { return m_requestHistory.size(); }

void CodeAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* CodeAgent::GetLLM() const { return m_llm; }

void CodeAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
}

} // namespace Agents
