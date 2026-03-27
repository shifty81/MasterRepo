#include "Engine/Debug/DebugConsole/DebugConsole/DebugConsole.h"
#include <algorithm>
#include <deque>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Engine {

using CVarValue = std::variant<bool, int32_t, float, std::string>;

struct DebugConsole::Impl {
    std::unordered_map<std::string, ConsoleCommand> commands;
    std::unordered_map<std::string, CVarValue>      cvars;
    std::vector<ConsoleLogLine>                     log;
    std::deque<std::string>                         history;
    std::mutex                                      queueMtx;
    std::deque<std::string>                         commandQueue;
    uint32_t                                        historySize{200};
    int32_t                                         historyPos{-1};
    std::function<void(const ConsoleLogLine&)>      onOutput;

    std::vector<std::string> Tokenise(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) tokens.push_back(tok);
        return tokens;
    }

    void AddLog(const std::string& msg, LogSeverity sev) {
        ConsoleLogLine l{msg, sev};
        log.push_back(l);
        if ((uint32_t)log.size() > historySize) log.erase(log.begin());
        if (onOutput) onOutput(l);
    }
};

DebugConsole::DebugConsole()  : m_impl(new Impl) {}
DebugConsole::~DebugConsole() { Shutdown(); delete m_impl; }

void DebugConsole::Init(uint32_t historySize) {
    m_impl->historySize = historySize;
    // Built-in: help
    Register("help", "List all commands", 0, [this](const std::vector<std::string>&) {
        for (auto& [name, cmd] : m_impl->commands)
            Print("  " + name + " — " + cmd.description);
    });
    Register("clear", "Clear output log", 0, [this](const std::vector<std::string>&) { ClearLog(); });
    Register("echo",  "Echo arguments",   -1, [this](const std::vector<std::string>& args) {
        std::string out; for (auto& a:args) out+=a+" "; Print(out);
    });
    Register("list",  "List CVars",       0, [this](const std::vector<std::string>&) {
        std::vector<std::string> names; ListCVars(names);
        for (auto& n:names) Print("  "+n);
    });
}

void DebugConsole::Shutdown() { m_impl->commands.clear(); m_impl->log.clear(); }

void DebugConsole::Register(const std::string& name, const std::string& desc,
                              int32_t minArgs, CmdHandler handler)
{
    m_impl->commands[name] = {name, desc, minArgs, handler};
}

void DebugConsole::Unregister(const std::string& name) { m_impl->commands.erase(name); }
bool DebugConsole::HasCommand(const std::string& name) const { return m_impl->commands.count(name)>0; }

bool DebugConsole::Execute(const std::string& line) {
    if (line.empty()) return false;
    PushHistory(line);
    auto tokens = m_impl->Tokenise(line);
    if (tokens.empty()) return false;
    auto it = m_impl->commands.find(tokens[0]);
    if (it == m_impl->commands.end()) {
        PrintError("Unknown command: " + tokens[0]);
        return false;
    }
    std::vector<std::string> args(tokens.begin()+1, tokens.end());
    if (it->second.minArgs >= 0 && (int32_t)args.size() < it->second.minArgs) {
        PrintError("Too few arguments for: " + tokens[0]);
        return false;
    }
    if (it->second.handler) it->second.handler(args);
    return true;
}

void DebugConsole::SubmitCommand(const std::string& line) {
    std::lock_guard<std::mutex> lk(m_impl->queueMtx);
    m_impl->commandQueue.push_back(line);
}

void DebugConsole::Tick() {
    std::deque<std::string> batch;
    { std::lock_guard<std::mutex> lk(m_impl->queueMtx); batch.swap(m_impl->commandQueue); }
    for (auto& cmd : batch) Execute(cmd);
}

std::vector<std::string> DebugConsole::Autocomplete(const std::string& prefix) const {
    std::vector<std::string> out;
    for (auto& [name, _] : m_impl->commands)
        if (name.substr(0, prefix.size()) == prefix) out.push_back(name);
    std::sort(out.begin(), out.end());
    return out;
}

void DebugConsole::PushHistory(const std::string& line) {
    m_impl->history.push_front(line);
    if ((uint32_t)m_impl->history.size() > m_impl->historySize)
        m_impl->history.pop_back();
    m_impl->historyPos = -1;
}

std::string DebugConsole::HistoryUp() {
    if (m_impl->history.empty()) return "";
    m_impl->historyPos = std::min(m_impl->historyPos+1, (int32_t)m_impl->history.size()-1);
    return m_impl->history[(uint32_t)m_impl->historyPos];
}
std::string DebugConsole::HistoryDown() {
    if (m_impl->historyPos<=0) { m_impl->historyPos=-1; return ""; }
    return m_impl->history[(uint32_t)(--m_impl->historyPos)];
}
uint32_t DebugConsole::HistoryCount() const { return (uint32_t)m_impl->history.size(); }
void     DebugConsole::ClearHistory() { m_impl->history.clear(); m_impl->historyPos=-1; }

void DebugConsole::Print       (const std::string& msg) { m_impl->AddLog(msg, LogSeverity::Info); }
void DebugConsole::PrintWarning(const std::string& msg) { m_impl->AddLog(msg, LogSeverity::Warning); }
void DebugConsole::PrintError  (const std::string& msg) { m_impl->AddLog(msg, LogSeverity::Error); }
const std::vector<ConsoleLogLine>& DebugConsole::GetLog() const { return m_impl->log; }
void DebugConsole::ClearLog()  { m_impl->log.clear(); }
void DebugConsole::SetOnOutput(std::function<void(const ConsoleLogLine&)> cb) { m_impl->onOutput=cb; }

// CVars
void DebugConsole::SetCVar(const std::string& name, float v)             { m_impl->cvars[name]=v; }
void DebugConsole::SetCVar(const std::string& name, int32_t v)           { m_impl->cvars[name]=v; }
void DebugConsole::SetCVar(const std::string& name, bool v)              { m_impl->cvars[name]=v; }
void DebugConsole::SetCVar(const std::string& name, const std::string& v){ m_impl->cvars[name]=v; }

float DebugConsole::GetCVarFloat(const std::string& name, float def) const {
    auto it=m_impl->cvars.find(name); if(it==m_impl->cvars.end()) return def;
    if (auto* v=std::get_if<float>(&it->second)) return *v;
    if (auto* v=std::get_if<int32_t>(&it->second)) return (float)*v;
    return def;
}
int32_t DebugConsole::GetCVarInt(const std::string& name, int32_t def) const {
    auto it=m_impl->cvars.find(name); if(it==m_impl->cvars.end()) return def;
    if (auto* v=std::get_if<int32_t>(&it->second)) return *v;
    if (auto* v=std::get_if<float>(&it->second)) return (int32_t)*v;
    return def;
}
bool DebugConsole::GetCVarBool(const std::string& name, bool def) const {
    auto it=m_impl->cvars.find(name); if(it==m_impl->cvars.end()) return def;
    if (auto* v=std::get_if<bool>(&it->second)) return *v;
    return def;
}
std::string DebugConsole::GetCVarString(const std::string& name, const std::string& def) const {
    auto it=m_impl->cvars.find(name); if(it==m_impl->cvars.end()) return def;
    if (auto* v=std::get_if<std::string>(&it->second)) return *v;
    return def;
}
bool DebugConsole::HasCVar(const std::string& name) const { return m_impl->cvars.count(name)>0; }
void DebugConsole::ListCVars(std::vector<std::string>& out) const {
    for (auto& [k,_]:m_impl->cvars) out.push_back(k);
}

} // namespace Engine
