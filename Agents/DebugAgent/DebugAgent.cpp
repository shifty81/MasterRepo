#include "Agents/DebugAgent/DebugAgent.h"
#include <algorithm>
#include <sstream>

namespace Agents {

DebugAgent::DebugAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void DebugAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t DebugAgent::GetPermissions() const { return m_permissions; }
bool DebugAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void DebugAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void DebugAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* DebugAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> DebugAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t DebugAgent::IntentCount() const { return m_intents.size(); }

AIResponse DebugAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus DebugAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  DebugAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& DebugAgent::GetResponseHistory() const { return m_responseHistory; }
size_t DebugAgent::RequestCount() const { return m_requestHistory.size(); }

void DebugAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* DebugAgent::GetLLM() const { return m_llm; }

void DebugAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
    m_breakpoints.clear();
    m_stackTrace.clear();
}

bool DebugAgent::SetBreakpoint(const std::string& file, int line) {
    for (auto& bp : m_breakpoints) {
        if (bp.file == file && bp.line == line) { bp.enabled = true; return true; }
    }
    m_breakpoints.push_back({file, line, true});
    return true;
}

bool DebugAgent::ClearBreakpoint(const std::string& file, int line) {
    auto it = std::remove_if(m_breakpoints.begin(), m_breakpoints.end(),
        [&](const Breakpoint& bp){ return bp.file == file && bp.line == line; });
    if (it == m_breakpoints.end()) return false;
    m_breakpoints.erase(it, m_breakpoints.end());
    return true;
}

std::vector<StackFrame> DebugAgent::GetStackTrace() const { return m_stackTrace; }
const std::vector<Breakpoint>& DebugAgent::GetBreakpoints() const { return m_breakpoints; }

void DebugAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "set_breakpoint",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto fileIt = req.parameters.find("file");
            auto lineIt = req.parameters.find("line");
            if (fileIt == req.parameters.end() || lineIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing file or line parameter";
                return resp;
            }
            int line = std::stoi(lineIt->second);
            resp.success = SetBreakpoint(fileIt->second, line);
            resp.result  = resp.success ? "Breakpoint set at " + fileIt->second + ":" + lineIt->second
                                        : "Failed to set breakpoint";
            return resp;
        }
    });

    RegisterIntent({
        "clear_breakpoint",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto fileIt = req.parameters.find("file");
            auto lineIt = req.parameters.find("line");
            if (fileIt == req.parameters.end() || lineIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing file or line parameter";
                return resp;
            }
            int line = std::stoi(lineIt->second);
            resp.success = ClearBreakpoint(fileIt->second, line);
            resp.result  = resp.success ? "Breakpoint cleared"
                                        : "Breakpoint not found";
            return resp;
        }
    });

    RegisterIntent({
        "get_stack_trace",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest&) -> AIResponse {
            AIResponse resp;
            resp.success = true;
            std::ostringstream oss;
            const auto frames = GetStackTrace();
            for (size_t i = 0; i < frames.size(); ++i) {
                oss << "#" << i << " " << frames[i].function
                    << " at " << frames[i].file << ":" << frames[i].line << "\n";
            }
            resp.result = frames.empty() ? "No stack trace available" : oss.str();
            return resp;
        }
    });

    RegisterIntent({
        "inspect_variable",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("variable");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing variable parameter";
                return resp;
            }
            resp.success = true;
            resp.result  = "Variable '" + it->second + "': inspection requires active debug session";
            return resp;
        }
    });
}

} // namespace Agents
