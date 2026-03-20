#include "Agents/EditorAgent/EditorAgent.h"
#include <algorithm>
#include <sstream>

namespace Agents {

EditorAgent::EditorAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void EditorAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t EditorAgent::GetPermissions() const { return m_permissions; }
bool EditorAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void EditorAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void EditorAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* EditorAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> EditorAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t EditorAgent::IntentCount() const { return m_intents.size(); }

AIResponse EditorAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus EditorAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  EditorAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& EditorAgent::GetResponseHistory() const { return m_responseHistory; }
size_t EditorAgent::RequestCount() const { return m_requestHistory.size(); }

void EditorAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* EditorAgent::GetLLM() const { return m_llm; }

void EditorAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
    m_openFiles.clear();
}

bool EditorAgent::SendCommand(const EditorCommand& cmd) {
    // Dispatch to registered intent if matching, otherwise record the command.
    auto it = m_intents.find(cmd.command);
    if (it != m_intents.end()) {
        AIRequest req;
        req.intentName  = cmd.command;
        req.parameters  = cmd.args;
        auto resp = ProcessRequest(req);
        return resp.success;
    }
    return false;
}

bool EditorAgent::OpenFile(const std::string& path, int /*line*/) {
    if (std::find(m_openFiles.begin(), m_openFiles.end(), path) == m_openFiles.end())
        m_openFiles.push_back(path);
    return true;
}

bool EditorAgent::NavigateTo(const std::string& /*symbol*/) {
    // Navigation requires a live editor connection; records intent.
    return true;
}

std::vector<std::string> EditorAgent::GetOpenFiles() const { return m_openFiles; }

void EditorAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "open_file",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("path");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'path' parameter";
                return resp;
            }
            int line = 0;
            auto lineIt = req.parameters.find("line");
            if (lineIt != req.parameters.end()) line = std::stoi(lineIt->second);
            resp.success = OpenFile(it->second, line);
            resp.result  = "Opened: " + it->second + (line ? " at line " + lineIt->second : "");
            return resp;
        }
    });

    RegisterIntent({
        "navigate_to",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("symbol");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'symbol' parameter";
                return resp;
            }
            resp.success = NavigateTo(it->second);
            resp.result  = "Navigating to: " + it->second;
            return resp;
        }
    });

    RegisterIntent({
        "run_command",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("command");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'command' parameter";
                return resp;
            }
            EditorCommand cmd;
            cmd.command = it->second;
            cmd.args    = req.parameters;
            cmd.args.erase("command");
            resp.success = SendCommand(cmd);
            resp.result  = "Command dispatched: " + it->second;
            return resp;
        }
    });

    RegisterIntent({
        "get_open_files",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest&) -> AIResponse {
            AIResponse resp;
            resp.success = true;
            resp.actions = GetOpenFiles();
            std::ostringstream oss;
            oss << resp.actions.size() << " file(s) open";
            resp.result = oss.str();
            return resp;
        }
    });
}

} // namespace Agents
