#pragma once
#include "Agents/CodeAgent/CodeAgent.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cstdint>

namespace Agents {

struct AssetRequest {
    std::string type;
    std::string prompt;
    std::string outputPath;
};

struct AssetRecord {
    uint64_t                 id   = 0;
    std::string              name;
    std::string              type;
    std::string              path;
    std::vector<std::string> tags;
};

class AssetAgent {
public:
    AssetAgent();

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
    AssetRecord ImportAsset(const std::string& path);
    AssetRecord GenerateAsset(const AssetRequest& req);
    bool TagAsset(uint64_t id, const std::string& tag);
    std::vector<AssetRecord> SearchAssets(const std::string& query) const;

private:
    void RegisterDefaultIntents();

    uint8_t                                m_permissions   = 0;
    std::map<std::string, AIIntentHandler> m_intents;
    std::vector<AIRequest>                 m_requestHistory;
    std::vector<AIResponse>                m_responseHistory;
    uint64_t                               m_nextRequestID = 1;
    AgentStatus                            m_status        = AgentStatus::Idle;
    AI::ModelManager*                      m_llm           = nullptr;
    std::vector<AssetRecord>               m_assets;
    uint64_t                               m_nextAssetID   = 1;
};

} // namespace Agents
