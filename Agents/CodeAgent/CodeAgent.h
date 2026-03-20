#pragma once
#include "AI/ModelManager/ModelManager.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

namespace Agents {

enum class AIPermission : uint8_t {
    ReadFile  = 1 << 0,
    WriteFile = 1 << 1,
    Compile   = 1 << 2,
    Execute   = 1 << 3,
    Network   = 1 << 4,
};

enum class AgentStatus : uint8_t {
    Idle,
    Planning,
    Executing,
    Waiting,
    Error,
};

struct AIRequest {
    uint64_t                          requestID   = 0;
    std::string                       intentName;
    std::map<std::string,std::string> parameters;
    std::string                       prompt;
};

struct AIResponse {
    uint64_t                 requestID    = 0;
    bool                     success      = false;
    std::string              result;
    std::string              errorMessage;
    std::vector<std::string> actions;
};

struct AIIntentHandler {
    std::string                                  name;
    uint8_t                                      requiredPermissions = 0;
    std::function<AIResponse(const AIRequest&)>  handler;
};

class CodeAgent {
public:
    void SetPermissions(uint8_t permissions);
    uint8_t GetPermissions() const;
    bool HasPermission(AIPermission perm) const;

    void RegisterIntent(const AIIntentHandler& handler);
    void UnregisterIntent(const std::string& name);
    const AIIntentHandler* GetIntent(const std::string& name) const;
    std::vector<std::string> ListIntents() const;
    size_t IntentCount() const;

    AIResponse ProcessRequest(const AIRequest& request);

    AgentStatus GetStatus() const;
    const std::vector<AIRequest>&  GetRequestHistory() const;
    const std::vector<AIResponse>& GetResponseHistory() const;
    size_t RequestCount() const;

    void SetLLM(AI::ModelManager* llm);
    AI::ModelManager* GetLLM() const;

    void Reset();

private:
    uint8_t                                m_permissions    = 0;
    std::map<std::string, AIIntentHandler> m_intents;
    std::vector<AIRequest>                 m_requestHistory;
    std::vector<AIResponse>                m_responseHistory;
    uint64_t                               m_nextRequestID  = 1;
    AgentStatus                            m_status         = AgentStatus::Idle;
    AI::ModelManager*                      m_llm            = nullptr;
};

} // namespace Agents
