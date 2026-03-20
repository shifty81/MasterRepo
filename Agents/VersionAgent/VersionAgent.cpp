#include "Agents/VersionAgent/VersionAgent.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <fstream>

namespace Agents {

static std::string CurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

VersionAgent::VersionAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile)
                  | static_cast<uint8_t>(AIPermission::Execute);
    RegisterDefaultIntents();
}

void VersionAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t VersionAgent::GetPermissions() const { return m_permissions; }
bool VersionAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void VersionAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void VersionAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* VersionAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> VersionAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t VersionAgent::IntentCount() const { return m_intents.size(); }

AIResponse VersionAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus VersionAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  VersionAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& VersionAgent::GetResponseHistory() const { return m_responseHistory; }
size_t VersionAgent::RequestCount() const { return m_requestHistory.size(); }

void VersionAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* VersionAgent::GetLLM() const { return m_llm; }

void VersionAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID  = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
    m_snapshots.clear();
    m_nextSnapshotID = 1;
}

Snapshot VersionAgent::CreateSnapshot(const std::string& description) {
    Snapshot snap;
    snap.id          = m_nextSnapshotID++;
    snap.timestamp   = CurrentTimestamp();
    snap.description = description;
    // Record tracked files from current directory
    for (auto& entry : std::filesystem::recursive_directory_iterator(".",
                            std::filesystem::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            if (ext == ".h" || ext == ".cpp" || ext == ".cmake" || ext == ".txt")
                snap.files.push_back(entry.path().string());
        }
    }
    m_snapshots.push_back(snap);
    return snap;
}

std::vector<Snapshot> VersionAgent::ListSnapshots() const { return m_snapshots; }

bool VersionAgent::Rollback(uint64_t id) {
    for (const auto& snap : m_snapshots) {
        if (snap.id == id) return true; // Actual rollback requires VCS integration
    }
    return false;
}

std::string VersionAgent::GetDiff(uint64_t id) const {
    for (const auto& snap : m_snapshots) {
        if (snap.id == id) {
            std::ostringstream oss;
            oss << "Snapshot " << snap.id << " [" << snap.timestamp << "]: " << snap.description << "\n";
            oss << snap.files.size() << " file(s) tracked\n";
            return oss.str();
        }
    }
    return "Snapshot not found";
}

void VersionAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "create_snapshot",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            std::string desc;
            auto it = req.parameters.find("description");
            if (it != req.parameters.end()) desc = it->second;
            Snapshot snap = CreateSnapshot(desc);
            resp.success = true;
            resp.result  = "Snapshot " + std::to_string(snap.id) + " created at " + snap.timestamp;
            return resp;
        }
    });

    RegisterIntent({
        "list_snapshots",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest&) -> AIResponse {
            AIResponse resp;
            resp.success = true;
            const auto snaps = ListSnapshots();
            std::ostringstream oss;
            oss << snaps.size() << " snapshot(s)";
            resp.result = oss.str();
            for (const auto& s : snaps)
                resp.actions.push_back(std::to_string(s.id) + " [" + s.timestamp + "] " + s.description);
            return resp;
        }
    });

    RegisterIntent({
        "rollback",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("id");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'id' parameter";
                return resp;
            }
            uint64_t id = std::stoull(it->second);
            resp.success = Rollback(id);
            resp.result  = resp.success ? "Rolled back to snapshot " + it->second
                                        : "Snapshot not found";
            return resp;
        }
    });

    RegisterIntent({
        "get_diff",
        static_cast<uint8_t>(AIPermission::ReadFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto it = req.parameters.find("id");
            if (it == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing 'id' parameter";
                return resp;
            }
            uint64_t id = std::stoull(it->second);
            resp.success = true;
            resp.result  = GetDiff(id);
            return resp;
        }
    });
}

} // namespace Agents
