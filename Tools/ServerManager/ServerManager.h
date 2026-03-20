#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Server configuration — migrated from atlas::ServerConfig
// ──────────────────────────────────────────────────────────────

struct ServerConfig {
    std::string host               = "0.0.0.0";
    uint16_t    port               = 7777;
    int32_t     maxConnections     = 100;
    std::string serverName         = "MasterRepo Server";
    std::string serverDescription;
    bool        persistentWorld    = true;
    bool        autoSave           = true;
    int32_t     saveIntervalSecs   = 300;
    bool        useWhitelist       = false;
    bool        publicServer       = false;
    std::string password;
    bool        useSteam           = false;
    uint32_t    steamAppID         = 0;
    bool        steamAuthentication = false;
    bool        steamServerBrowser = false;
    float       tickRate           = 20.0f;
    int32_t     maxEntities        = 10000;
    std::string dataPath           = "data/";
    std::string savePath           = "saves/";
    std::string logPath            = "logs/";

    bool LoadFromFile(const std::string& filePath);
    bool SaveToFile(const std::string& filePath) const;
};

// ──────────────────────────────────────────────────────────────
// Server instance status
// ──────────────────────────────────────────────────────────────

enum class ServerStatus { Stopped, Starting, Running, Stopping, Error };

struct ServerInstance {
    uint32_t      id          = 0;
    std::string   name;
    ServerConfig  config;
    ServerStatus  status      = ServerStatus::Stopped;
    int32_t       pid         = -1;
    std::string   logFile;
    uint32_t      playerCount = 0;
    std::string   startTime;
};

// ──────────────────────────────────────────────────────────────
// ServerManager — game server deployment and monitoring
// ──────────────────────────────────────────────────────────────

class ServerManager {
public:
    // Instance management
    uint32_t            AddInstance(const std::string& name, const ServerConfig& cfg);
    bool                RemoveInstance(uint32_t id);
    ServerInstance*     GetInstance(uint32_t id);
    const ServerInstance* GetInstance(uint32_t id) const;
    const std::vector<ServerInstance>& Instances() const { return m_instances; }

    // Lifecycle
    bool Start(uint32_t id, const std::string& executable = "");
    bool Stop(uint32_t id);
    bool Restart(uint32_t id, const std::string& executable = "");
    void StopAll();

    // Status polling
    ServerStatus QueryStatus(uint32_t id) const;
    void         RefreshAll();

    // Config persistence
    bool SaveConfig(uint32_t id, const std::string& path = "") const;
    bool LoadConfig(uint32_t id, const std::string& path);

    // Logs
    std::string  ReadLog(uint32_t id, size_t lastNLines = 100) const;
    void         ClearLog(uint32_t id);

    // Callbacks
    using StatusFn = std::function<void(uint32_t id, ServerStatus status)>;
    void SetStatusCallback(StatusFn fn);

    using OutputFn = std::function<void(uint32_t id, const std::string& line)>;
    void SetOutputCallback(OutputFn fn);

private:
    std::vector<ServerInstance> m_instances;
    uint32_t                    m_nextID = 1;
    StatusFn                    m_statusFn;
    OutputFn                    m_outputFn;

    void changeStatus(ServerInstance& inst, ServerStatus status);
    static std::string CurrentTimestamp();
};

} // namespace Tools
