#pragma once
/**
 * @file LuaBinding.h
 * @brief Lightweight script-engine binding / function registry.
 *
 * LuaBinding provides a dependency-free function registry that models the
 * interface a Lua (or other scripting) engine would expose to C++ code:
 *   - Register named C++ functions callable from scripts
 *   - ScriptValue: a simple variant (nil/bool/int/float/string)
 *   - Dispatch: look up a function by name and call it with a value list
 *   - ScriptModule: grouped bindings with a namespace prefix
 *
 * This deliberately avoids pulling in the Lua C library — the binding layer
 * is pure C++20, so the rest of the engine can be tested without Lua installed.
 * Actual Lua integration (luaL_newstate etc.) would live in a platform-specific
 * backend that implements IScriptBackend.
 */

#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <unordered_map>
#include <optional>
#include <stdexcept>

namespace Core {

// ── Value type ────────────────────────────────────────────────────────────
struct ScriptNil {};

using ScriptValue = std::variant<
    ScriptNil,
    bool,
    int64_t,
    double,
    std::string
>;

// Convenience helpers
inline ScriptValue  SV(bool v)              { return v; }
inline ScriptValue  SV(int64_t v)           { return v; }
inline ScriptValue  SV(int v)               { return static_cast<int64_t>(v); }
inline ScriptValue  SV(double v)            { return v; }
inline ScriptValue  SV(const std::string& v){ return v; }
inline ScriptValue  SV(const char* v)       { return std::string(v); }
inline ScriptValue  SVNil()                 { return ScriptNil{}; }

inline bool IsNil(const ScriptValue& v)    { return std::holds_alternative<ScriptNil>(v); }
inline bool IsBool(const ScriptValue& v)   { return std::holds_alternative<bool>(v); }
inline bool IsInt(const ScriptValue& v)    { return std::holds_alternative<int64_t>(v); }
inline bool IsFloat(const ScriptValue& v)  { return std::holds_alternative<double>(v); }
inline bool IsString(const ScriptValue& v) { return std::holds_alternative<std::string>(v); }

std::string ScriptValueToString(const ScriptValue& v);

// ── Function signature ────────────────────────────────────────────────────
using ScriptArgs   = std::vector<ScriptValue>;
using ScriptResult = std::vector<ScriptValue>;
using ScriptFn     = std::function<ScriptResult(const ScriptArgs&)>;

struct ScriptFunction {
    std::string name;
    std::string description;
    std::string signature;  ///< Human-readable e.g. "(string, int) -> bool"
    ScriptFn    fn;
};

// ── Module ────────────────────────────────────────────────────────────────
class ScriptModule {
public:
    explicit ScriptModule(std::string namespaceName = "");
    const std::string& Namespace() const;

    void Register(ScriptFunction func);
    bool Has(const std::string& name) const;
    const ScriptFunction* Get(const std::string& name) const;

    ScriptResult Call(const std::string& name, const ScriptArgs& args) const;

    std::vector<std::string> ListFunctions() const;
    size_t FunctionCount() const;

private:
    std::string m_namespace;
    std::unordered_map<std::string, ScriptFunction> m_funcs;
};

// ── Main binding registry ──────────────────────────────────────────────────
class LuaBinding {
public:
    LuaBinding();
    ~LuaBinding();

    // ── module management ─────────────────────────────────────
    void RegisterModule(ScriptModule module);
    ScriptModule* GetModule(const std::string& ns);
    const ScriptModule* GetModule(const std::string& ns) const;
    std::vector<std::string> Namespaces() const;

    // ── direct function registration (global namespace) ───────
    void Register(ScriptFunction func);

    // ── dispatch ─────────────────────────────────────────────
    /// Call "namespace.function" or "function" (global).
    ScriptResult Call(const std::string& qualifiedName,
                      const ScriptArgs& args = {}) const;

    bool CanCall(const std::string& qualifiedName) const;

    // ── enumeration ───────────────────────────────────────────
    /// List all fully-qualified function names.
    std::vector<std::string> AllFunctions() const;
    size_t TotalFunctions() const;

    // ── introspection ─────────────────────────────────────────
    std::string Dump() const;  ///< Human-readable listing

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Core
