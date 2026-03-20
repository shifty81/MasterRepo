#include "Agents/RefactorAgent/RefactorAgent.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <filesystem>

namespace Agents {

RefactorAgent::RefactorAgent() {
    m_permissions = static_cast<uint8_t>(AIPermission::ReadFile)
                  | static_cast<uint8_t>(AIPermission::WriteFile);
    RegisterDefaultIntents();
}

void RefactorAgent::SetPermissions(uint8_t permissions) { m_permissions = permissions; }
uint8_t RefactorAgent::GetPermissions() const { return m_permissions; }
bool RefactorAgent::HasPermission(AIPermission perm) const {
    return (m_permissions & static_cast<uint8_t>(perm)) != 0;
}

void RefactorAgent::RegisterIntent(const AIIntentHandler& handler) { m_intents[handler.name] = handler; }
void RefactorAgent::UnregisterIntent(const std::string& name) { m_intents.erase(name); }
const AIIntentHandler* RefactorAgent::GetIntent(const std::string& name) const {
    auto it = m_intents.find(name);
    if (it == m_intents.end()) return nullptr;
    return &it->second;
}
std::vector<std::string> RefactorAgent::ListIntents() const {
    std::vector<std::string> names;
    names.reserve(m_intents.size());
    for (const auto& kv : m_intents) names.push_back(kv.first);
    return names;
}
size_t RefactorAgent::IntentCount() const { return m_intents.size(); }

AIResponse RefactorAgent::ProcessRequest(const AIRequest& request) {
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

AgentStatus RefactorAgent::GetStatus() const { return m_status; }
const std::vector<AIRequest>&  RefactorAgent::GetRequestHistory()  const { return m_requestHistory; }
const std::vector<AIResponse>& RefactorAgent::GetResponseHistory() const { return m_responseHistory; }
size_t RefactorAgent::RequestCount() const { return m_requestHistory.size(); }

void RefactorAgent::SetLLM(AI::ModelManager* llm) { m_llm = llm; }
AI::ModelManager* RefactorAgent::GetLLM() const { return m_llm; }

void RefactorAgent::Reset() {
    m_permissions = 0;
    m_intents.clear();
    m_requestHistory.clear();
    m_responseHistory.clear();
    m_nextRequestID = 1;
    m_status = AgentStatus::Idle;
    m_llm = nullptr;
}

bool RefactorAgent::RenameSymbol(const std::string& filePath, const std::string& oldName,
                                  const std::string& newName) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();
    // Escape any regex metacharacters in oldName, then match whole-word boundaries
    static const std::string kMeta = R"delim(\.^$*+?{}[]|())delim";
    std::string escaped;
    for (char c : oldName) {
        if (kMeta.find(c) != std::string::npos)
            escaped += '\\';
        escaped += c;
    }
    std::regex re("\\b" + escaped + "\\b");
    std::string updated = std::regex_replace(content, re, newName);
    if (updated == content) return true; // nothing to change
    std::ofstream out(filePath);
    if (!out.is_open()) return false;
    out << updated;
    return true;
}

bool RefactorAgent::ExtractFunction(const std::string& filePath, int startLine, int endLine,
                                     const std::string& funcName) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    in.close();
    if (startLine < 1 || endLine > static_cast<int>(lines.size()) || startLine > endLine)
        return false;
    // Build extracted function body
    std::ostringstream func;
    func << "auto " << funcName << " = [&]() {\n";
    for (int i = startLine - 1; i < endLine; ++i) func << "    " << lines[i] << "\n";
    func << "};\n";
    // Replace extracted lines with call
    lines.erase(lines.begin() + startLine - 1, lines.begin() + endLine);
    lines.insert(lines.begin() + startLine - 1, funcName + "();");
    lines.insert(lines.begin() + startLine - 1, func.str());
    std::ofstream out(filePath);
    if (!out.is_open()) return false;
    for (const auto& l : lines) out << l << "\n";
    return true;
}

bool RefactorAgent::MoveFile(const std::string& srcPath, const std::string& dstPath) {
    std::error_code ec;
    std::filesystem::rename(srcPath, dstPath, ec);
    return !ec;
}

bool RefactorAgent::UpdateIncludes(const std::string& oldPath, const std::string& newPath) {
    // Updates #include directives in all .h/.cpp files in current directory tree
    std::string pattern = "#include \"" + oldPath + "\"";
    std::string replacement = "#include \"" + newPath + "\"";
    bool anyChanged = false;
    for (auto& entry : std::filesystem::recursive_directory_iterator(".")) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".h" && ext != ".cpp" && ext != ".hpp") continue;
        std::ifstream in(entry.path());
        if (!in.is_open()) continue;
        std::string content((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
        in.close();
        if (content.find(pattern) == std::string::npos) continue;
        std::string updated;
        size_t pos = 0, found;
        while ((found = content.find(pattern, pos)) != std::string::npos) {
            updated += content.substr(pos, found - pos);
            updated += replacement;
            pos = found + pattern.size();
        }
        updated += content.substr(pos);
        std::ofstream out(entry.path());
        if (!out.is_open()) continue;
        out << updated;
        anyChanged = true;
    }
    return anyChanged;
}

void RefactorAgent::RegisterDefaultIntents() {
    RegisterIntent({
        "rename_symbol",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto fileIt    = req.parameters.find("file");
            auto oldIt     = req.parameters.find("old_name");
            auto newIt     = req.parameters.find("new_name");
            if (fileIt == req.parameters.end() || oldIt == req.parameters.end() ||
                newIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing file, old_name, or new_name parameter";
                return resp;
            }
            resp.success = RenameSymbol(fileIt->second, oldIt->second, newIt->second);
            resp.result  = resp.success ? "Renamed '" + oldIt->second + "' to '" + newIt->second + "'"
                                        : "Rename failed";
            return resp;
        }
    });

    RegisterIntent({
        "extract_function",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto fileIt  = req.parameters.find("file");
            auto startIt = req.parameters.find("start_line");
            auto endIt   = req.parameters.find("end_line");
            auto nameIt  = req.parameters.find("func_name");
            if (fileIt == req.parameters.end() || startIt == req.parameters.end() ||
                endIt == req.parameters.end() || nameIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing file, start_line, end_line, or func_name parameter";
                return resp;
            }
            resp.success = ExtractFunction(fileIt->second, std::stoi(startIt->second),
                                           std::stoi(endIt->second), nameIt->second);
            resp.result  = resp.success ? "Function '" + nameIt->second + "' extracted"
                                        : "Extraction failed";
            return resp;
        }
    });

    RegisterIntent({
        "move_file",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto srcIt = req.parameters.find("src");
            auto dstIt = req.parameters.find("dst");
            if (srcIt == req.parameters.end() || dstIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing src or dst parameter";
                return resp;
            }
            resp.success = MoveFile(srcIt->second, dstIt->second);
            resp.result  = resp.success ? "Moved " + srcIt->second + " -> " + dstIt->second
                                        : "Move failed";
            return resp;
        }
    });

    RegisterIntent({
        "update_includes",
        static_cast<uint8_t>(AIPermission::ReadFile) | static_cast<uint8_t>(AIPermission::WriteFile),
        [this](const AIRequest& req) -> AIResponse {
            AIResponse resp;
            auto oldIt = req.parameters.find("old_path");
            auto newIt = req.parameters.find("new_path");
            if (oldIt == req.parameters.end() || newIt == req.parameters.end()) {
                resp.success      = false;
                resp.errorMessage = "Missing old_path or new_path parameter";
                return resp;
            }
            resp.success = UpdateIncludes(oldIt->second, newIt->second);
            resp.result  = resp.success ? "Includes updated from '" + oldIt->second + "' to '" + newIt->second + "'"
                                        : "No matching includes found";
            return resp;
        }
    });
}

} // namespace Agents
