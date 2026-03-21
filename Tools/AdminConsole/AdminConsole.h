#pragma once
// AdminConsole — Server admin console (Blueprint: Server/admin_console)
//
// Provides a text-based administrative interface for a running Atlas game
// server.  It exposes:
//   • Server metrics (CPU, memory, player count, tick rate)
//   • Live validation output from the AI Validator
//   • Command shell for server management (kick, ban, reload, status)
//   • HTTP metrics endpoint stub (simple line-protocol for external dashboards)
//
// The console can be driven programmatically (for testing) or via the
// server's standard input.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Server metrics snapshot
// ─────────────────────────────────────────────────────────────────────────────

struct ServerMetrics {
    float    cpuUsagePct      = 0.f;
    float    memoryUsageMB    = 0.f;
    float    memoryTotalMB    = 0.f;
    int      playerCount      = 0;
    int      maxPlayers       = 64;
    float    tickRate         = 30.f;
    float    avgTickTimeMs    = 0.f;
    float    peakTickTimeMs   = 0.f;
    float    uptimeSec        = 0.f;
    int      activeAIAgents   = 0;
    int      pendingJobs      = 0;
    float    networkInKBs     = 0.f;
    float    networkOutKBs    = 0.f;
    std::string serverVersion;
    std::string mapName;
};

// ─────────────────────────────────────────────────────────────────────────────
// Console command
// ─────────────────────────────────────────────────────────────────────────────

struct ConsoleCommand {
    std::string              name;
    std::string              usage;
    std::string              description;
    std::function<std::string(const std::vector<std::string>& args)> handler;
};

// ─────────────────────────────────────────────────────────────────────────────
// HTTP metrics endpoint config (stub)
// ─────────────────────────────────────────────────────────────────────────────

struct HTTPMetricsConfig {
    bool     enabled = false;
    uint16_t port    = 8081;
    std::string path = "/metrics";   // Prometheus-compatible line protocol
};

// ─────────────────────────────────────────────────────────────────────────────
// AdminConsoleConfig
// ─────────────────────────────────────────────────────────────────────────────

struct AdminConsoleConfig {
    bool              interactive  = true;    // read from stdin
    std::string       logPath;                // write command log here
    HTTPMetricsConfig http;
    std::string       motd = "Atlas Engine Admin Console — type 'help'";
};

// ─────────────────────────────────────────────────────────────────────────────
// AdminConsole
// ─────────────────────────────────────────────────────────────────────────────

class AdminConsole {
public:
    explicit AdminConsole(AdminConsoleConfig cfg = {});
    ~AdminConsole();

    // ── Lifecycle ────────────────────────────────────────────────────────────

    void Init();
    void Shutdown();

    // ── Per-frame ────────────────────────────────────────────────────────────

    /// Process any pending input from stdin (non-blocking).
    void Tick();

    // ── Metric feed ──────────────────────────────────────────────────────────

    void FeedMetrics(const ServerMetrics& metrics);

    // ── Command registration ─────────────────────────────────────────────────

    void RegisterCommand(ConsoleCommand cmd);
    void UnregisterCommand(const std::string& name);

    // ── Programmatic command execution ───────────────────────────────────────

    std::string Execute(const std::string& line);

    // ── Validation output injection ──────────────────────────────────────────

    void PostValidationResult(const std::string& summary,
                               bool passed);

    // ── HTTP metrics ─────────────────────────────────────────────────────────

    /// Build Prometheus-format metrics line-protocol from latest snapshot.
    std::string BuildPrometheusMetrics() const;

    // ── Reporting ─────────────────────────────────────────────────────────────

    std::string BuildStatusReport() const;
    void        PrintStatus()       const;

    // ── Callbacks ────────────────────────────────────────────────────────────

    using OutputFn = std::function<void(const std::string& line)>;
    void SetOutputCallback(OutputFn fn);

private:
    AdminConsoleConfig                 m_cfg;
    std::vector<ConsoleCommand>        m_commands;
    ServerMetrics                      m_latest;
    std::vector<std::string>           m_validationLog;
    OutputFn                           m_outputFn;
    bool                               m_initialised = false;

    void RegisterBuiltinCommands();
    std::string CmdHelp(const std::vector<std::string>& args) const;
    std::string CmdStatus(const std::vector<std::string>& args) const;
    std::string CmdMetrics(const std::vector<std::string>& args) const;
    std::string ParseAndExecute(const std::string& line);
    void Output(const std::string& line) const;
};

} // namespace Tools
