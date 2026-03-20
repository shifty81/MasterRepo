#include "Agents/CleanupAgent/CleanupAgent.h"
#include <filesystem>
#include <sstream>
#include <regex>
#include <stdexcept>

namespace Agents {

CleanupAgent::CleanupAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void CleanupAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t CleanupAgent::GetPermissions() const { return m_permissions; }
bool CleanupAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void CleanupAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void CleanupAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* CleanupAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> CleanupAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t CleanupAgent::IntentCount() const { return m_intents.size(); }

AIResponse CleanupAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus CleanupAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  CleanupAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& CleanupAgent::GetResponseHistory() const { return m_responseHistory; }
size_t CleanupAgent::RequestCount() const { return m_requestHistory.size(); }

void CleanupAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* CleanupAgent::GetLLM() const { return m_llm; }

void CleanupAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
    m_rules.clear();
    m_lastReport = CleanupReport{};
}

void CleanupAgent::AddRule(const CleanupRule& rule) { m_rules.push_back(rule); }
const std::vector<CleanupRule>& CleanupAgent::GetRules() const { return m_rules; }

CleanupReport CleanupAgent::RunCleanup(const std::string& rootDir) {
    CleanupReport report;
    namespace fs = std::filesystem;

    for (const auto& rule : m_rules) {
        std::regex re(rule.pattern);
        for (auto& entry : fs::recursive_directory_iterator(rootDir,
                               fs::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            std::string name = entry.path().filename().string();
            if (!std::regex_search(name, re)) continue;
            std::string path = entry.path().string();
            std::error_code ec;
            switch (rule.action) {
                case CleanupAction::Delete:
                    fs::remove(entry.path(), ec);
                    if (!ec) report.deleted.push_back(path);
                    else     report.errors.push_back("Delete failed: " + path);
                    break;
                case CleanupAction::Archive:
                case CleanupAction::Move: {
                    if (rule.targetDir.empty()) { report.errors.push_back("No target dir: " + path); break; }
                    fs::create_directories(rule.targetDir, ec);
                    auto dst = fs::path(rule.targetDir) / entry.path().filename();
                    fs::rename(entry.path(), dst, ec);
                    if (!ec) {
                        if (rule.action == CleanupAction::Archive) report.archived.push_back(path);
                        else                                        report.moved.push_back(path);
                    } else {
                        report.errors.push_back("Move failed: " + path);
                    }
                    break;
                }
            }
            ++report.totalProcessed;
        }
    }
    m_lastReport = report;
    return report;
}

CleanupReport CleanupAgent::GetReport() const { return m_lastReport; }

void CleanupAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "scan_workspace",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string dir = ".";
            auto it = req.parameters.find("dir");
            if (it != req.parameters.end()) dir = it->second;
            std::vector<std::string> found;
            namespace fs = std::filesystem;
            for (auto& entry : fs::recursive_directory_iterator(dir,
                                   fs::directory_options::skip_permission_denied)) {
                if (!entry.is_regular_file()) continue;
                for (const auto& rule : m_rules) {
                    std::regex re(rule.pattern);
                    if (std::regex_search(entry.path().filename().string(), re)) {
                        found.push_back(entry.path().string());
                        break;
                    }
                }
            }
            resp.success = true;
            std::ostringstream oss;
            oss << found.size() << " file(s) matched cleanup rules";
            resp.result  = oss.str();
            resp.actions = found;
            return resp;
        }
    });

    RegisterIntent({
        "cleanup_temp",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string dir = ".";
            auto it = req.parameters.find("dir");
            if (it != req.parameters.end()) dir = it->second;
            // Add a temporary rule for common temp extensions; remove it even if RunCleanup throws
            CleanupRule tmpRule{ R"(\.(tmp|bak|o|obj|pdb)$)", CleanupAction::Delete, "" };
            AddRule(tmpRule);
            CleanupReport report;
            try { report = RunCleanup(dir); } catch (...) { m_rules.pop_back(); throw; }
            m_rules.pop_back();
            resp.success = true;
            std::ostringstream oss;
            oss << "Deleted " << report.deleted.size() << " temp file(s)";
            resp.result  = oss.str();
            resp.actions = report.deleted;
            return resp;
        }
    });

    RegisterIntent({
        "archive_old",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string dir    = ".";
            std::string target = "Archive";
            auto it = req.parameters.find("dir");
            if (it != req.parameters.end()) dir = it->second;
            it = req.parameters.find("target");
            if (it != req.parameters.end()) target = it->second;
            CleanupRule rule{ R"(\.(log|bak)$)", CleanupAction::Archive, target };
            AddRule(rule);
            CleanupReport report;
            try { report = RunCleanup(dir); } catch (...) { m_rules.pop_back(); throw; }
            m_rules.pop_back();
            resp.success = true;
            std::ostringstream oss;
            oss << "Archived " << report.archived.size() << " file(s) to " << target;
            resp.result  = oss.str();
            resp.actions = report.archived;
            return resp;
        }
    });
}

} // namespace Agents
