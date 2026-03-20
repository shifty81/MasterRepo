#pragma once
#include "Agents/CodeAgent/CodeAgent.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

namespace Agents {

struct ErrorPattern {
    std::string pattern;
    std::string description;
    std::string fix_template;
};

class FixAgent {
public:
    FixAgent();

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
    std::vector<ErrorPattern> AnalyzeError(const std::string& error);
    std::string SuggestFix(const std::string& error);
    bool ApplyFix(const std::string& filePath, const std::string& fix, int line);
    void AddPattern(const ErrorPattern& pattern);

private:
    void RegisterDefaultIntents();
    void LoadBuiltinPatterns();

    uint8_t                                m_permissions   = 0;
    std::map<std::string, AIIntentHandler> m_intents;
    std::vector<AIRequest>                 m_requestHistory;
    std::vector<AIResponse>                m_responseHistory;
    uint64_t                               m_nextRequestID = 1;
    AgentStatus                            m_status        = AgentStatus::Idle;
    AI::ModelManager*                      m_llm           = nullptr;
    std::vector<ErrorPattern>              m_patterns;
};

} // namespace Agents
