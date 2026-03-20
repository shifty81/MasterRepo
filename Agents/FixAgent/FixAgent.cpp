#include "Agents/FixAgent/FixAgent.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace Agents {

FixAgent::FixAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile);
    LoadBuiltinPatterns();
    RegisterDefaultIntents();
}

void FixAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t FixAgent::GetPermissions() const { return m_permissions; }
bool FixAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void FixAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void FixAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* FixAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> FixAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t FixAgent::IntentCount() const { return m_intents.size(); }

AIResponse FixAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus FixAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  FixAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& FixAgent::GetResponseHistory() const { return m_responseHistory; }
size_t FixAgent::RequestCount() const { return m_requestHistory.size(); }

void FixAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* FixAgent::GetLLM() const { return m_llm; }

void FixAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
    m_patterns.clear();
}

std::vector<ErrorPattern> FixAgent::AnalyzeError(const std::string& error) {
    std::vector<ErrorPattern> matches;
    for (const auto& p : m_patterns) {
        if (error.find(p.pattern) != std::string::npos) matches.push_back(p);
    }
    return matches;
}

std::string FixAgent::SuggestFix(const std::string& error) {
    auto matches = AnalyzeError(error);
    if (matches.empty()) return "No known fix pattern matched for this error.";
    std::ostringstream oss;
    for (const auto& m : matches) {
        oss << "[" << m.description << "] " << m.fix_template << "\n";
    }
    return oss.str();
}

bool FixAgent::ApplyFix(const std::string& filePath, const std::string& fix, int line) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    std::vector<std::string> lines;
    std::string l;
    while (std::getline(in, l)) lines.push_back(l);
    in.close();
    if (line < 1 || line > static_cast<int>(lines.size())) return false;
    lines[static_cast<size_t>(line - 1)] = fix;
    std::ofstream out(filePath);
    if (!out.is_open()) return false;
    for (const auto& row : lines) out << row << "\n";
    return true;
}

void FixAgent::AddPattern(const ErrorPattern& pattern) { m_patterns.push_back(pattern); }

void FixAgent::LoadBuiltinPatterns() {
    m_patterns = {
        { "use of undeclared identifier",
          "Undeclared identifier — check includes and scope",
          "Add the missing #include or forward declaration." },
        { "no member named",
          "Missing member — verify class definition",
          "Check that the member exists in the class and the correct header is included." },
        { "expected ';'",
          "Missing semicolon",
          "Add a semicolon at the end of the statement." },
        { "cannot convert",
          "Type mismatch — implicit conversion not available",
          "Add an explicit cast or change the variable type." },
        { "undefined reference",
          "Linker error — symbol not found",
          "Ensure the translation unit defining the symbol is compiled and linked." },
    };
}

void FixAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "analyze_error",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("error");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'error' parameter";
                return resp;
            }
            auto matches = AnalyzeError(it->second);
            resp.success = true;
            std::ostringstream oss;
            oss << matches.size() << " pattern(s) matched";
            resp.result = oss.str();
            for (const auto& m : matches) resp.actions.push_back(m.description);
            return resp;
        }
    });

    RegisterIntent({
        "suggest_fix",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("error");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'error' parameter";
                return resp;
            }
            resp.success = true;
            resp.result  = SuggestFix(it->second);
            return resp;
        }
    });

    RegisterIntent({
        "apply_fix",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto fileIt = req.parameters.find("file");
            auto fixIt  = req.parameters.find("fix");
            auto lineIt = req.parameters.find("line");
            if (fileIt == req.parameters.end() || fixIt == req.parameters.end() ||
                lineIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing file, fix, or line parameter";
                return resp;
            }
            int line = std::stoi(lineIt->second);
            resp.success = ApplyFix(fileIt->second, fixIt->second, line);
            resp.result  = resp.success ? "Fix applied to " + fileIt->second + ":" + lineIt->second
                                        : "Failed to apply fix";
            return resp;
        }
    });
}

} // namespace Agents
