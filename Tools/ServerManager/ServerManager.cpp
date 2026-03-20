#include "Tools/ServerManager/ServerManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <filesystem>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// ServerConfig — migrated from atlas::ServerConfig
// ──────────────────────────────────────────────────────────────

bool ServerConfig::LoadFromFile(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line.find("//") == 0) continue;
        size_t col = line.find(':');
        if (col == std::string::npos) continue;
        auto trim = [](std::string s) {
            s.erase(0, s.find_first_not_of(" \t\""));
            s.erase(s.find_last_not_of(" \t\",") + 1);
            return s;
        };
        std::string key = trim(line.substr(0, col));
        std::string val = trim(line.substr(col + 1));
        if      (key == "host")                    host               = val;
        else if (key == "port")                    port               = (uint16_t)std::stoi(val);
        else if (key == "max_connections")         maxConnections     = std::stoi(val);
        else if (key == "server_name")             serverName         = val;
        else if (key == "server_description")      serverDescription  = val;
        else if (key == "persistent_world")        persistentWorld    = (val == "true");
        else if (key == "auto_save")               autoSave           = (val == "true");
        else if (key == "save_interval_seconds")   saveIntervalSecs   = std::stoi(val);
        else if (key == "use_whitelist")           useWhitelist       = (val == "true");
        else if (key == "public_server")           publicServer       = (val == "true");
        else if (key == "password")                password           = val;
        else if (key == "use_steam")               useSteam           = (val == "true");
        else if (key == "steam_app_id")            steamAppID         = (uint32_t)std::stoul(val);
        else if (key == "steam_authentication")    steamAuthentication = (val == "true");
        else if (key == "steam_server_browser")    steamServerBrowser  = (val == "true");
        else if (key == "tick_rate")               tickRate           = std::stof(val);
        else if (key == "max_entities")            maxEntities        = std::stoi(val);
        else if (key == "data_path")               dataPath           = val;
        else if (key == "save_path")               savePath           = val;
        else if (key == "log_path")                logPath            = val;
    }
    return true;
}

bool ServerConfig::SaveToFile(const std::string& filePath) const {
    std::ofstream f(filePath);
    if (!f) return false;
    f << "{\n"
      << "  \"host\": \""               << host               << "\",\n"
      << "  \"port\": "                 << port               << ",\n"
      << "  \"max_connections\": "      << maxConnections     << ",\n"
      << "  \"server_name\": \""        << serverName         << "\",\n"
      << "  \"server_description\": \"" << serverDescription  << "\",\n"
      << "  \"persistent_world\": "     << (persistentWorld ? "true":"false") << ",\n"
      << "  \"auto_save\": "            << (autoSave ? "true":"false")        << ",\n"
      << "  \"save_interval_seconds\": "<< saveIntervalSecs   << ",\n"
      << "  \"use_whitelist\": "        << (useWhitelist  ? "true":"false")   << ",\n"
      << "  \"public_server\": "        << (publicServer  ? "true":"false")   << ",\n"
      << "  \"password\": \""           << password           << "\",\n"
      << "  \"use_steam\": "            << (useSteam ? "true":"false")        << ",\n"
      << "  \"steam_app_id\": "         << steamAppID         << ",\n"
      << "  \"tick_rate\": "            << tickRate           << ",\n"
      << "  \"max_entities\": "         << maxEntities        << ",\n"
      << "  \"data_path\": \""          << dataPath           << "\",\n"
      << "  \"save_path\": \""          << savePath           << "\",\n"
      << "  \"log_path\": \""           << logPath            << "\"\n"
      << "}\n";
    return true;
}

// ──────────────────────────────────────────────────────────────
// ServerManager
// ──────────────────────────────────────────────────────────────

std::string ServerManager::CurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return os.str();
}

void ServerManager::SetStatusCallback(StatusFn fn) { m_statusFn = std::move(fn); }
void ServerManager::SetOutputCallback(OutputFn fn) { m_outputFn = std::move(fn); }

void ServerManager::changeStatus(ServerInstance& inst, ServerStatus s) {
    inst.status = s;
    if (m_statusFn) m_statusFn(inst.id, s);
}

uint32_t ServerManager::AddInstance(const std::string& name, const ServerConfig& cfg) {
    ServerInstance inst;
    inst.id     = m_nextID++;
    inst.name   = name;
    inst.config = cfg;
    inst.status = ServerStatus::Stopped;
    inst.logFile = cfg.logPath + "server_" + std::to_string(inst.id) + ".log";
    m_instances.push_back(std::move(inst));
    return m_instances.back().id;
}

bool ServerManager::RemoveInstance(uint32_t id) {
    auto it = std::find_if(m_instances.begin(), m_instances.end(),
        [id](const ServerInstance& i){ return i.id == id; });
    if (it == m_instances.end()) return false;
    if (it->status == ServerStatus::Running) Stop(id);
    m_instances.erase(it);
    return true;
}

ServerInstance* ServerManager::GetInstance(uint32_t id) {
    for (auto& i : m_instances) if (i.id == id) return &i;
    return nullptr;
}

const ServerInstance* ServerManager::GetInstance(uint32_t id) const {
    for (const auto& i : m_instances) if (i.id == id) return &i;
    return nullptr;
}

bool ServerManager::Start(uint32_t id, const std::string& executable) {
    auto* inst = GetInstance(id);
    if (!inst) return false;
    if (inst->status == ServerStatus::Running) return true;

    changeStatus(*inst, ServerStatus::Starting);
    std::filesystem::create_directories(inst->config.logPath);

    // Build launch command
    std::string exe = executable.empty() ? "MasterRepoServer" : executable;
    std::string cmd = exe
        + " --host " + inst->config.host
        + " --port " + std::to_string(inst->config.port)
        + " >> " + inst->logFile + " 2>&1 &";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        changeStatus(*inst, ServerStatus::Error);
        return false;
    }
    inst->startTime = CurrentTimestamp();
    changeStatus(*inst, ServerStatus::Running);
    return true;
}

bool ServerManager::Stop(uint32_t id) {
    auto* inst = GetInstance(id);
    if (!inst) return false;
    if (inst->status == ServerStatus::Stopped) return true;
    changeStatus(*inst, ServerStatus::Stopping);
    if (inst->pid > 0) {
        std::string cmd = "kill " + std::to_string(inst->pid);
        std::system(cmd.c_str());
        inst->pid = -1;
    }
    changeStatus(*inst, ServerStatus::Stopped);
    return true;
}

bool ServerManager::Restart(uint32_t id, const std::string& executable) {
    Stop(id);
    return Start(id, executable);
}

void ServerManager::StopAll() {
    for (auto& inst : m_instances)
        if (inst.status == ServerStatus::Running) Stop(inst.id);
}

ServerStatus ServerManager::QueryStatus(uint32_t id) const {
    const auto* inst = GetInstance(id);
    return inst ? inst->status : ServerStatus::Error;
}

void ServerManager::RefreshAll() {
    // No-op stub: real impl would ping each server's health endpoint
}

bool ServerManager::SaveConfig(uint32_t id, const std::string& path) const {
    const auto* inst = GetInstance(id);
    if (!inst) return false;
    std::string p = path.empty() ? inst->config.dataPath + "server.cfg" : path;
    return inst->config.SaveToFile(p);
}

bool ServerManager::LoadConfig(uint32_t id, const std::string& path) {
    auto* inst = GetInstance(id);
    if (!inst) return false;
    return inst->config.LoadFromFile(path);
}

std::string ServerManager::ReadLog(uint32_t id, size_t lastNLines) const {
    const auto* inst = GetInstance(id);
    if (!inst) return "";
    std::ifstream f(inst->logFile);
    if (!f) return "";
    std::deque<std::string> lines;
    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(std::move(line));
        if (lines.size() > lastNLines) lines.pop_front();
    }
    std::ostringstream os;
    for (const auto& l : lines) os << l << "\n";
    return os.str();
}

void ServerManager::ClearLog(uint32_t id) {
    const auto* inst = GetInstance(id);
    if (!inst) return;
    std::ofstream f(inst->logFile, std::ios::trunc);
}

} // namespace Tools
