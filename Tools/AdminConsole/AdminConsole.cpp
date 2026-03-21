#include "Tools/AdminConsole/AdminConsole.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
namespace Tools {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

AdminConsole::AdminConsole(AdminConsoleConfig cfg)
    : m_cfg(std::move(cfg))
{}

AdminConsole::~AdminConsole() { Shutdown(); }

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void AdminConsole::Init()
{
    if (m_initialised) return;
    RegisterBuiltinCommands();
    Output(m_cfg.motd);
    m_initialised = true;
}

void AdminConsole::Shutdown() { m_initialised = false; }

// ── Per-frame ─────────────────────────────────────────────────────────────────

void AdminConsole::Tick()
{
    if (!m_cfg.interactive || !m_initialised) return;
    // Non-blocking stdin check: only reads if data is available
    // (Would use select() on Unix; stub reads one line if available)
}

// ── Metric feed ───────────────────────────────────────────────────────────────

void AdminConsole::FeedMetrics(const ServerMetrics& metrics)
{
    m_latest = metrics;
}

// ── Command registration ──────────────────────────────────────────────────────

void AdminConsole::RegisterCommand(ConsoleCommand cmd)
{
    auto it = std::find_if(m_commands.begin(), m_commands.end(),
                           [&](const ConsoleCommand& c){ return c.name == cmd.name; });
    if (it != m_commands.end()) *it = std::move(cmd);
    else                        m_commands.push_back(std::move(cmd));
}

void AdminConsole::UnregisterCommand(const std::string& name)
{
    m_commands.erase(
        std::remove_if(m_commands.begin(), m_commands.end(),
                       [&](const ConsoleCommand& c){ return c.name == name; }),
        m_commands.end());
}

// ── Execution ─────────────────────────────────────────────────────────────────

std::string AdminConsole::ParseAndExecute(const std::string& line)
{
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    if (tokens.empty()) return {};

    const std::string& name = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    for (const auto& cmd : m_commands) {
        if (cmd.name == name && cmd.handler) {
            try {
                return cmd.handler(args);
            } catch (const std::exception& e) {
                return "Error: " + std::string(e.what());
            }
        }
    }
    return "Unknown command: '" + name + "'. Type 'help' for list.";
}

std::string AdminConsole::Execute(const std::string& line)
{
    std::string result = ParseAndExecute(line);
    Output(result);

    // Log to file if configured
    if (!m_cfg.logPath.empty()) {
        try {
            fs::create_directories(fs::path(m_cfg.logPath).parent_path());
            std::ofstream f(m_cfg.logPath, std::ios::app);
            std::time_t t = std::time(nullptr);
            char buf[32];
            std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]",
                          std::localtime(&t));
            f << buf << " > " << line << "\n";
            if (!result.empty()) f << buf << "   " << result << "\n";
        } catch (...) {}
    }
    return result;
}

// ── Validation ────────────────────────────────────────────────────────────────

void AdminConsole::PostValidationResult(const std::string& summary, bool passed)
{
    std::string line = (passed ? "[VAL OK] " : "[VAL FAIL] ") + summary;
    m_validationLog.push_back(line);
    if (m_validationLog.size() > 100)
        m_validationLog.erase(m_validationLog.begin());
    Output(line);
}

// ── HTTP metrics ──────────────────────────────────────────────────────────────

std::string AdminConsole::BuildPrometheusMetrics() const
{
    std::ostringstream oss;
    const auto& m = m_latest;
    oss << "# HELP atlas_cpu_pct CPU usage percent\n"
        << "atlas_cpu_pct " << m.cpuUsagePct << "\n"
        << "# HELP atlas_memory_mb Memory usage MB\n"
        << "atlas_memory_mb " << m.memoryUsageMB << "\n"
        << "# HELP atlas_players Player count\n"
        << "atlas_players " << m.playerCount << "\n"
        << "# HELP atlas_tick_rate Tick rate Hz\n"
        << "atlas_tick_rate " << m.tickRate << "\n"
        << "# HELP atlas_avg_tick_ms Average tick duration ms\n"
        << "atlas_avg_tick_ms " << m.avgTickTimeMs << "\n"
        << "# HELP atlas_uptime_sec Server uptime seconds\n"
        << "atlas_uptime_sec " << m.uptimeSec << "\n"
        << "# HELP atlas_network_in_kbs Network in KB/s\n"
        << "atlas_network_in_kbs " << m.networkInKBs << "\n"
        << "# HELP atlas_network_out_kbs Network out KB/s\n"
        << "atlas_network_out_kbs " << m.networkOutKBs << "\n";
    return oss.str();
}

// ── Reporting ─────────────────────────────────────────────────────────────────

std::string AdminConsole::BuildStatusReport() const
{
    std::ostringstream oss;
    const auto& m = m_latest;
    oss << "=== Atlas Engine Server Status ===\n"
        << "Version:      " << m.serverVersion << "\n"
        << "Map:          " << m.mapName << "\n"
        << "Uptime:       " << (int)m.uptimeSec << "s\n"
        << "Players:      " << m.playerCount << " / " << m.maxPlayers << "\n"
        << "Tick rate:    " << m.tickRate << " Hz\n"
        << "Avg tick:     " << m.avgTickTimeMs << " ms\n"
        << "Peak tick:    " << m.peakTickTimeMs << " ms\n"
        << "CPU:          " << m.cpuUsagePct << "%\n"
        << "Memory:       " << m.memoryUsageMB << " / "
                            << m.memoryTotalMB << " MB\n"
        << "Net in/out:   " << m.networkInKBs << " / "
                            << m.networkOutKBs << " KB/s\n"
        << "AI agents:    " << m.activeAIAgents << "\n"
        << "Pending jobs: " << m.pendingJobs << "\n";
    return oss.str();
}

void AdminConsole::PrintStatus() const
{
    Output(BuildStatusReport());
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void AdminConsole::SetOutputCallback(OutputFn fn) { m_outputFn = std::move(fn); }

void AdminConsole::Output(const std::string& line) const
{
    if (m_outputFn) {
        m_outputFn(line);
    } else {
        // Default: write to stdout
        if (!line.empty())
            std::cout << line << "\n";
    }
}

// ── Built-in commands ─────────────────────────────────────────────────────────

std::string AdminConsole::CmdHelp(const std::vector<std::string>&) const
{
    std::ostringstream oss;
    oss << "Available commands:\n";
    for (const auto& c : m_commands)
        oss << "  " << c.name
            << (c.usage.empty() ? "" : " " + c.usage)
            << "  — " << c.description << "\n";
    return oss.str();
}

std::string AdminConsole::CmdStatus(const std::vector<std::string>&) const
{
    return BuildStatusReport();
}

std::string AdminConsole::CmdMetrics(const std::vector<std::string>&) const
{
    return BuildPrometheusMetrics();
}

void AdminConsole::RegisterBuiltinCommands()
{
    RegisterCommand({"help",    "",  "Show this help message",
                     [this](const std::vector<std::string>& a){
                         return CmdHelp(a); }});
    RegisterCommand({"status",  "",  "Show server status",
                     [this](const std::vector<std::string>& a){
                         return CmdStatus(a); }});
    RegisterCommand({"metrics", "",  "Dump Prometheus metrics",
                     [this](const std::vector<std::string>& a){
                         return CmdMetrics(a); }});
    RegisterCommand({"quit",    "",  "Shutdown the console",
                     [](const std::vector<std::string>&){
                         return "Shutting down..."; }});
}

} // namespace Tools
