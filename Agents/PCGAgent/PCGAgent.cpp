#include "Agents/PCGAgent/PCGAgent.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace Agents {

PCGAgent::PCGAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void PCGAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t PCGAgent::GetPermissions() const { return m_permissions; }
bool PCGAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void PCGAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void PCGAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* PCGAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> PCGAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t PCGAgent::IntentCount() const { return m_intents.size(); }

AIResponse PCGAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus PCGAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  PCGAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& PCGAgent::GetResponseHistory() const { return m_responseHistory; }
size_t PCGAgent::RequestCount() const { return m_requestHistory.size(); }

void PCGAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* PCGAgent::GetLLM() const { return m_llm; }

void PCGAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
}

PCGResult PCGAgent::GenerateContent(const PCGRequest& req) {
    PCGResult result;
    result.outputPath   = req.params.count("output") ? req.params.at("output")
                                                      : "pcg_" + req.type + "_" +
                                                        std::to_string(req.seed) + ".dat";
    result.description  = "Generated " + req.type + " with seed " + std::to_string(req.seed);
    result.success      = true;
    return result;
}

PCGResult PCGAgent::PreviewContent(const PCGRequest& req) {
    PCGResult result = GenerateContent(req);
    result.outputPath   = "preview_" + result.outputPath;
    result.description  = "[Preview] " + result.description;
    return result;
}

bool PCGAgent::SaveResult(const PCGResult& result, const std::string& path) {
    if (!result.success) return false;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << result.description << "\nsource: " << result.outputPath << "\n";
    return true;
}

void PCGAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "generate_geometry",
        static_cast<uint8_t>(AIPermission::WriteFile) | static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            PCGRequest pcgReq;
            pcgReq.type   = "geometry";
            auto it = req.parameters.find("seed");
            if (it != req.parameters.end()) pcgReq.seed = static_cast<uint32_t>(std::stoul(it->second));
            pcgReq.params = req.parameters;
            PCGResult r = GenerateContent(pcgReq);
            resp.success = r.success;
            resp.result  = r.description;
            resp.actions.push_back(r.outputPath);
            return resp;
        }
    });

    RegisterIntent({
        "generate_texture",
        static_cast<uint8_t>(AIPermission::WriteFile) | static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            PCGRequest pcgReq;
            pcgReq.type   = "texture";
            auto it = req.parameters.find("seed");
            if (it != req.parameters.end()) pcgReq.seed = static_cast<uint32_t>(std::stoul(it->second));
            pcgReq.params = req.parameters;
            PCGResult r = GenerateContent(pcgReq);
            resp.success = r.success;
            resp.result  = r.description;
            resp.actions.push_back(r.outputPath);
            return resp;
        }
    });

    RegisterIntent({
        "generate_world",
        static_cast<uint8_t>(AIPermission::WriteFile) | static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            PCGRequest pcgReq;
            pcgReq.type   = "world";
            auto it = req.parameters.find("seed");
            if (it != req.parameters.end()) pcgReq.seed = static_cast<uint32_t>(std::stoul(it->second));
            pcgReq.params = req.parameters;
            PCGResult r = GenerateContent(pcgReq);
            resp.success = r.success;
            resp.result  = r.description;
            resp.actions.push_back(r.outputPath);
            return resp;
        }
    });

    RegisterIntent({
        "preview_content",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            PCGRequest pcgReq;
            auto it = req.parameters.find("type");
            if (it != req.parameters.end()) pcgReq.type = it->second;
            it = req.parameters.find("seed");
            if (it != req.parameters.end()) pcgReq.seed = static_cast<uint32_t>(std::stoul(it->second));
            pcgReq.params = req.parameters;
            PCGResult r = PreviewContent(pcgReq);
            resp.success = r.success;
            resp.result  = r.description;
            resp.actions.push_back(r.outputPath);
            return resp;
        }
    });
}

} // namespace Agents
