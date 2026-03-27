#pragma once
/**
 * @file LuaScriptEngine.h
 * @brief Lightweight Lua scripting engine: state management, binding, coroutines, sandboxing.
 *
 * Features:
 *   - Init() / Shutdown(): create and tear down Lua state
 *   - ExecString(code) → bool: execute Lua source from string
 *   - ExecFile(path) → bool: load and execute a .lua file
 *   - CallFunction(name, args...) → LuaValue: call a named global function
 *   - RegisterFunction(name, fn): expose a C++ callable to Lua
 *   - RegisterTable(name): create a Lua table namespace
 *   - SetGlobal(name, value) / GetGlobal(name) → LuaValue
 *   - PushCoroutine(code) → coId: create a resumable coroutine from source
 *   - ResumeCoroutine(coId) → LuaYieldState: {Done, Yielded, Error}
 *   - SetSandbox(on): restrict access to io/os/require
 *   - SetMemoryLimit(bytes): cap Lua allocator; exceeding fires OnMemoryOverflow
 *   - SetOnError(cb): callback(errorMessage) for runtime errors
 *   - GetLastError() → string
 *   - CollectGarbage(): force a GC cycle
 *   - Reset(): clear all state without full Shutdown/Init
 */

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <vector>

namespace Engine {

using LuaValue = std::variant<std::monostate, bool, double, std::string>;

enum class LuaYieldState : uint8_t { Done, Yielded, Error };

class LuaScriptEngine {
public:
    LuaScriptEngine();
    ~LuaScriptEngine();

    void Init    ();
    void Shutdown();
    void Reset   ();

    // Execution
    bool      ExecString  (const std::string& code);
    bool      ExecFile    (const std::string& path);
    LuaValue  CallFunction(const std::string& name,
                           const std::vector<LuaValue>& args = {});

    // Binding
    void RegisterFunction(const std::string& name,
                          std::function<LuaValue(std::vector<LuaValue>)> fn);
    void RegisterTable   (const std::string& name);
    void SetGlobal       (const std::string& name, const LuaValue& val);
    LuaValue GetGlobal   (const std::string& name) const;

    // Coroutines
    uint32_t      PushCoroutine  (const std::string& code);
    LuaYieldState ResumeCoroutine(uint32_t coId);
    void          KillCoroutine  (uint32_t coId);

    // Safety
    void SetSandbox     (bool on);
    void SetMemoryLimit (size_t bytes);
    void SetOnError     (std::function<void(const std::string&)> cb);
    void SetOnMemoryOverflow(std::function<void()> cb);

    // Diagnostics
    std::string GetLastError () const;
    void        CollectGarbage();
    size_t      GetMemoryUsage() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
