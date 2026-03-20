#include "Agents/BuildAgent/BuildAgent.h"
#include <chrono>
#include <sstream>
#include <regex>

namespace Agents {

BuildAgent::BuildAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile)
                  | static_cast<uint8_t>(AIPermission::Compile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void BuildAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t BuildAgent::GetPermissions() const { return m_permissions; }
bool BuildAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void BuildAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void BuildAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* BuildAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> BuildAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t BuildAgent::IntentCount() const { return m_intents.size(); }

AIResponse BuildAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus BuildAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  BuildAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& BuildAgent::GetResponseHistory() const { return m_responseHistory; }
size_t BuildAgent::RequestCount() const { return m_requestHistory.size(); }

void BuildAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* BuildAgent::GetLLM() const { return m_llm; }

void BuildAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
}

BuildResult BuildAgent::Compile(const std::string& target) {
    auto start = std::chrono::steady_clock::now();
    BuildResult result;
    std::string cmd = "cmake --build /tmp/build";
    if (!target.empty()) cmd += " --target " + target;
    result.output = "Build started: " + cmd;
    result.success = true;
    auto end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

BuildResult BuildAgent::RunCMake(const std::string& buildDir, const std::string& sourceDir) {
    auto start = std::chrono::steady_clock::now();
    BuildResult result;
    result.output = "cmake -B " + buildDir + " " + sourceDir;
    result.success = true;
    auto end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

BuildResult BuildAgent::RunTests(const std::string& filter) {
    auto start = std::chrono::steady_clock::now();
    BuildResult result;
    std::string cmd = "ctest --test-dir /tmp/build";
    if (!filter.empty()) cmd += " -R " + filter;
    result.output = "Tests run: " + cmd;
    result.success = true;
    auto end = std::chrono::steady_clock::now();
    result.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

std::vector<std::string> BuildAgent::CheckErrors(const std::string& output) {
    std::vector<std::string> errors;
    std::istringstream stream(output);
    std::string line;
    const std::regex errorRe(R"(error:)", std::regex::icase);
    while (std::getline(stream, line)) {
        if (std::regex_search(line, errorRe)) errors.push_back(line);
    }
    return errors;
}

void BuildAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "compile",
        static_cast<uint8_t>(AIPermission::Compile) | static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string target;
            auto it = req.parameters.find("target");
            if (it != req.parameters.end()) target = it->second;
            BuildResult r = Compile(target);
            resp.success = r.success;
            resp.result  = r.output;
            for (auto& e : r.errors) resp.actions.push_back("error: " + e);
            return resp;
        }
    });

    RegisterIntent({
        "run_cmake",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string buildDir = "/tmp/build";
            std::string srcDir   = ".";
            auto it = req.parameters.find("build_dir");
            if (it != req.parameters.end()) buildDir = it->second;
            it = req.parameters.find("source_dir");
            if (it != req.parameters.end()) srcDir = it->second;
            BuildResult r = RunCMake(buildDir, srcDir);
            resp.success = r.success;
            resp.result  = r.output;
            return resp;
        }
    });

    RegisterIntent({
        "run_tests",
        static_cast<uint8_t>(AIPermission::Execute),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string filter;
            auto it = req.parameters.find("filter");
            if (it != req.parameters.end()) filter = it->second;
            BuildResult r = RunTests(filter);
            resp.success = r.success;
            resp.result  = r.output;
            return resp;
        }
    });

    RegisterIntent({
        "check_errors",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string output;
            auto it = req.parameters.find("output");
            if (it != req.parameters.end()) output = it->second;
            auto errors = CheckErrors(output);
            resp.success = true;
            std::ostringstream oss;
            oss << errors.size() << " error(s) found";
            resp.result = oss.str();
            resp.actions = errors;
            return resp;
        }
    });
}

} // namespace Agents
