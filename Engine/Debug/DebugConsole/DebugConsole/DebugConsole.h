#pragma once
/**
 * @file DebugConsole.h
 * @brief In-game command console: tokeniser, registered commands, history, autocomplete.
 *
 * Features:
 *   - Command registration: name, description, argument count, handler callback
 *   - Tokeniser: splits input string → command name + argument vector
 *   - Autocomplete: Tab-completion of command names and optional arg hints
 *   - Command history: up/down arrow navigation (configurable history size)
 *   - Output log: lines with severity (Info, Warning, Error)
 *   - On-output callback (for HUD rendering)
 *   - Built-in commands: help, clear, echo, set (CVar), list
 *   - CVar (console variable) registry: read/write float/int/string/bool CVars
 *   - Thread-safe command queue: SubmitCommand() safe from any thread
 *   - Tick: drains queued commands and processes them
 *   - Print/PrintWarning/PrintError helpers
 *
 * Typical usage:
 * @code
 *   DebugConsole dc;
 *   dc.Init(200);
 *   dc.Register("spawn", "Spawn entity", 1,
 *       [](const std::vector<std::string>& args){ SpawnEntity(args[0]); });
 *   dc.SetCVar("gravity", -9.81f);
 *   dc.SubmitCommand("spawn player");
 *   dc.Tick();
 *   dc.SetOnOutput([](const std::string& line, LogSeverity s){ DrawLine(line, s); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

enum class LogSeverity : uint8_t { Info=0, Warning, Error };

struct ConsoleLogLine {
    std::string  text;
    LogSeverity  severity{LogSeverity::Info};
};

using CmdHandler = std::function<void(const std::vector<std::string>& args)>;

struct ConsoleCommand {
    std::string  name;
    std::string  description;
    int32_t      minArgs{0};   ///< -1 = variadic
    CmdHandler   handler;
};

class DebugConsole {
public:
    DebugConsole();
    ~DebugConsole();

    void Init    (uint32_t historySize=200);
    void Shutdown();
    void Tick    ();          ///< drain queued commands

    // Command registry
    void Register  (const std::string& name, const std::string& desc,
                    int32_t minArgs, CmdHandler handler);
    void Unregister(const std::string& name);
    bool HasCommand(const std::string& name) const;

    // Execute synchronously
    bool Execute(const std::string& line);
    // Thread-safe enqueue
    void SubmitCommand(const std::string& line);

    // Autocomplete: returns matching command names
    std::vector<std::string> Autocomplete(const std::string& prefix) const;

    // History navigation
    void          PushHistory(const std::string& line);
    std::string   HistoryUp();
    std::string   HistoryDown();
    uint32_t      HistoryCount() const;
    void          ClearHistory();

    // Output log
    void Print        (const std::string& msg);
    void PrintWarning (const std::string& msg);
    void PrintError   (const std::string& msg);
    const std::vector<ConsoleLogLine>& GetLog() const;
    void ClearLog();

    void SetOnOutput(std::function<void(const ConsoleLogLine&)> cb);

    // CVars
    void        SetCVar (const std::string& name, float v);
    void        SetCVar (const std::string& name, int32_t v);
    void        SetCVar (const std::string& name, bool v);
    void        SetCVar (const std::string& name, const std::string& v);
    float       GetCVarFloat (const std::string& name, float def=0.f)     const;
    int32_t     GetCVarInt   (const std::string& name, int32_t def=0)     const;
    bool        GetCVarBool  (const std::string& name, bool def=false)    const;
    std::string GetCVarString(const std::string& name,
                               const std::string& def="")                  const;
    bool        HasCVar(const std::string& name) const;
    void        ListCVars(std::vector<std::string>& outNames) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
