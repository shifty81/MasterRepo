#pragma once
#include "Agents/CodeAgent/CodeAgent.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

namespace Agents {

struct PCGRequest {
    std::string                       type;   // "geometry", "texture", "world"
    uint32_t                          seed    = 0;
    std::map<std::string,std::string> params;
};

struct PCGResult {
    bool        success     = false;
    std::string outputPath;
    std::string description;
};

class PCGAgent {
public:
    PCGAgent();

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

    // Agent-specific methods
    PCGResult GenerateContent(const PCGRequest& req);
    PCGResult PreviewContent(const PCGRequest& req);
    bool SaveResult(const PCGResult& result, const std::string& path);

private:
    void RegisterDefaultIntents();

    uint8_t                                m_permissions   = 0;
    std::map<std::string, AIIntentHandler> m_intents;
    std::vector<AIRequest>                 m_requestHistory;
    std::vector<AIResponse>                m_responseHistory;
    uint64_t                               m_nextRequestID = 1;
    AgentStatus                            m_status        = AgentStatus::Idle;
    AI::ModelManager*                      m_llm           = nullptr;
};

} // namespace Agents
