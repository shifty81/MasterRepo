#include "Agents/AssetAgent/AssetAgent.h"
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace Agents {

AssetAgent::AssetAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void AssetAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t AssetAgent::GetPermissions() const { return m_permissions; }
bool AssetAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void AssetAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void AssetAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* AssetAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> AssetAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t AssetAgent::IntentCount() const { return m_intents.size(); }

AIResponse AssetAgent::ProcessRequest(const AIRequest& request) {
    m_status = AgentStatus::Planning;
    AIRequest req = request;
    req.requestID = m_nextRequestID++;
    m_requestHistory.push_back(req);

    AIResponse response;
    response.requestID = req.requestID;

    auto it = m_intents.find(req.intentName);
    if (it == m_intents.end()) {
        response.success      = false;
        response.errorMessage = "Unknown intent: " + req.intentName;
        m_status = AgentStatus::Error;
        m_responseHistory.push_back(response);
        return response;
    }

    const auto& handler = it->second;
    if ((handler.requiredPermissions & m_permissions) != handler.requiredPermissions) {
        response.success      = false;
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

AgentStatus AssetAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  AssetAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& AssetAgent::GetResponseHistory() const { return m_responseHistory; }
size_t AssetAgent::RequestCount() const { return m_requestHistory.size(); }

void AssetAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* AssetAgent::GetLLM() const { return m_llm; }

void AssetAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
    m_assets.clear();
    m_nextAssetID = 1;
}

AssetRecord AssetAgent::ImportAsset(const std::string& path) {
    AssetRecord rec;
    rec.id   = m_nextAssetID++;
    rec.path = path;
    rec.name = std::filesystem::path(path).filename().string();
    rec.type = std::filesystem::path(path).extension().string();
    m_assets.push_back(rec);
    return rec;
}

AssetRecord AssetAgent::GenerateAsset(const AssetRequest& req) {
    AssetRecord rec;
    rec.id   = m_nextAssetID++;
    rec.type = req.type;
    rec.path = req.outputPath;
    rec.name = std::filesystem::path(req.outputPath).filename().string();
    // Generation is deferred to the pipeline; record is stored now.
    m_assets.push_back(rec);
    return rec;
}

bool AssetAgent::TagAsset(uint64_t id, const std::string& tag) {
    for (auto& rec : m_assets) {
        if (rec.id == id) {
            if (std::find(rec.tags.begin(), rec.tags.end(), tag) == rec.tags.end())
                rec.tags.push_back(tag);
            return true;
        }
    }
    return false;
}

std::vector<AssetRecord> AssetAgent::SearchAssets(const std::string& query) const {
    std::vector<AssetRecord> results;
    for (const auto& rec : m_assets) {
        if (rec.name.find(query) != std::string::npos ||
            rec.type.find(query) != std::string::npos ||
            rec.path.find(query) != std::string::npos) {
            results.push_back(rec);
            continue;
        }
        for (const auto& tag : rec.tags) {
            if (tag.find(query) != std::string::npos) { results.push_back(rec); break; }
        }
    }
    return results;
}

void AssetAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "import_asset",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("path");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'path' parameter";
                return resp;
            }
            AssetRecord rec = ImportAsset(it->second);
            resp.success = true;
            resp.result  = "Imported asset id=" + std::to_string(rec.id) + " name=" + rec.name;
            return resp;
        }
    });

    RegisterIntent({
        "generate_asset",
        static_cast<uint8_t>(AIPermission::WriteFile) | static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            AssetRequest ar;
            auto it = req.parameters.find("type");
            if (it != req.parameters.end()) ar.type = it->second;
            it = req.parameters.find("prompt");
            if (it != req.parameters.end()) ar.prompt = it->second;
            it = req.parameters.find("output");
            if (it != req.parameters.end()) ar.outputPath = it->second;
            AssetRecord rec = GenerateAsset(ar);
            resp.success = true;
            resp.result  = "Generated asset id=" + std::to_string(rec.id) + " type=" + rec.type;
            return resp;
        }
    });

    RegisterIntent({
        "tag_asset",
        static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto idIt  = req.parameters.find("id");
            auto tagIt = req.parameters.find("tag");
            if (idIt == req.parameters.end() || tagIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing id or tag parameter";
                return resp;
            }
            uint64_t id = std::stoull(idIt->second);
            resp.success = TagAsset(id, tagIt->second);
            resp.result  = resp.success ? "Tagged asset " + idIt->second
                                        : "Asset not found";
            return resp;
        }
    });

    RegisterIntent({
        "search_assets",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("query");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'query' parameter";
                return resp;
            }
            auto results = SearchAssets(it->second);
            resp.success = true;
            std::ostringstream oss;
            oss << results.size() << " asset(s) found";
            resp.result = oss.str();
            for (const auto& r : results) resp.actions.push_back(r.path);
            return resp;
        }
    });
}

} // namespace Agents
