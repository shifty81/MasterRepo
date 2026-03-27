#pragma once
/**
 * @file ScriptingBridge.h
 * @brief C++ ↔ script interop: function registry, value marshalling, call-in/call-out.
 *
 * Features:
 *   - ScriptValue: type-erased variant (Nil, Bool, Int, Float, String, Object handle)
 *   - Register C++ functions callable from script: RegisterFunction(name, fn)
 *   - Call script-side functions from C++: CallScript(name, args) → ScriptValue
 *   - Object binding: expose C++ objects with method/property tables
 *   - Script events: fire named events to all script listeners
 *   - Error handling: last error string, per-call error callback
 *   - Multiple script backends pluggable via SetBackend (Lua, Python, JS stub)
 *   - Sandboxed execution: time limit, memory limit
 *   - Script loading from file or string buffer
 *   - Hot-reload: re-execute script file on change
 *
 * Typical usage:
 * @code
 *   ScriptingBridge sb;
 *   sb.Init();
 *   sb.SetBackend(LuaBackend::Create());
 *   sb.RegisterFunction("Log", [](ScriptArgs a){ Log(a[0].AsString()); return ScriptValue{}; });
 *   sb.LoadFile("Scripts/game_init.lua");
 *   ScriptValue result = sb.CallScript("OnStart", {});
 *   sb.FireEvent("OnPlayerDied", {ScriptValue(playerId)});
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Engine {

// ── ScriptValue ──────────────────────────────────────────────────────────────

enum class ScriptValueType : uint8_t { Nil=0, Bool, Int, Float, String, Object };

class ScriptValue {
public:
    ScriptValue()                       : m_type(ScriptValueType::Nil) {}
    explicit ScriptValue(bool v)        : m_type(ScriptValueType::Bool),   m_b(v) {}
    explicit ScriptValue(int32_t v)     : m_type(ScriptValueType::Int),    m_i(v) {}
    explicit ScriptValue(float v)       : m_type(ScriptValueType::Float),  m_f(v) {}
    explicit ScriptValue(std::string v) : m_type(ScriptValueType::String), m_s(std::move(v)) {}
    explicit ScriptValue(uint64_t obj)  : m_type(ScriptValueType::Object), m_obj(obj) {}

    ScriptValueType Type()     const { return m_type; }
    bool            IsNil()    const { return m_type==ScriptValueType::Nil; }
    bool            AsBool()   const { return m_b; }
    int32_t         AsInt()    const { return m_i; }
    float           AsFloat()  const { return m_f; }
    const std::string& AsString()  const { return m_s; }
    uint64_t        AsObject() const { return m_obj; }

private:
    ScriptValueType m_type{ScriptValueType::Nil};
    bool         m_b{false};
    int32_t      m_i{0};
    float        m_f{0.f};
    std::string  m_s;
    uint64_t     m_obj{0};
};

using ScriptArgs = std::vector<ScriptValue>;
using ScriptFn   = std::function<ScriptValue(ScriptArgs)>;

// ── Backend interface ────────────────────────────────────────────────────────

class IScriptBackend {
public:
    virtual ~IScriptBackend() = default;
    virtual bool        LoadFile  (const std::string& path)        = 0;
    virtual bool        LoadString(const std::string& code)        = 0;
    virtual ScriptValue Call      (const std::string& fnName,
                                    const ScriptArgs& args)         = 0;
    virtual bool        HasFunction(const std::string& name) const = 0;
    virtual void        RegisterFunction(const std::string& name,
                                          ScriptFn fn)               = 0;
    virtual std::string LastError() const                           = 0;
    virtual void        SetMemoryLimit(uint64_t bytes)             {}
    virtual void        SetTimeLimit  (float ms)                   {}
};

// ── ScriptingBridge ──────────────────────────────────────────────────────────

class ScriptingBridge {
public:
    ScriptingBridge();
    ~ScriptingBridge();

    void Init    ();
    void Shutdown();

    // Backend
    void SetBackend(std::shared_ptr<IScriptBackend> backend);
    IScriptBackend* GetBackend() const;

    // Script loading
    bool LoadFile  (const std::string& path);
    bool LoadString(const std::string& code, const std::string& name="<inline>");

    // C++ → script: call a script function by name
    ScriptValue CallScript(const std::string& fnName, const ScriptArgs& args={});
    bool        HasFunction(const std::string& fnName) const;

    // script → C++: register C++ function callable from scripts
    void RegisterFunction(const std::string& name, ScriptFn fn);
    void UnregisterFunction(const std::string& name);

    // Events: fire named event to all script listeners
    void FireEvent(const std::string& eventName, const ScriptArgs& args={});

    // Error handling
    std::string LastError() const;
    void SetOnError(std::function<void(const std::string& msg)> cb);

    // Sandbox limits
    void SetMemoryLimit(uint64_t bytes);
    void SetTimeLimit  (float ms);

    // Hot-reload
    void SetHotReload(bool enabled);
    void ReloadFile  (const std::string& path);

    // Object binding helpers (C++ object → script handle)
    uint64_t BindObject(void* obj);
    void*    ResolveObject(uint64_t handle) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

// ── Stub backend (fallback when no real scripting engine present) ─────────────

class StubScriptBackend : public IScriptBackend {
public:
    bool        LoadFile  (const std::string&)                override { return true; }
    bool        LoadString(const std::string&)                override { return true; }
    ScriptValue Call      (const std::string& name, const ScriptArgs&) override {
        auto it = m_fns.find(name);
        return (it!=m_fns.end()) ? it->second({}) : ScriptValue{};
    }
    bool HasFunction(const std::string& name) const           override { return m_fns.count(name)>0; }
    void RegisterFunction(const std::string& name, ScriptFn fn) override { m_fns[name]=fn; }
    std::string LastError() const                             override { return ""; }
private:
    std::unordered_map<std::string,ScriptFn> m_fns;
    // minimal unordered_map include
    template<typename K,typename V> using umap = std::unordered_map<K,V>;
    umap<std::string,ScriptFn> placeholder_{}; // suppress unused warning
};

} // namespace Engine

#include <unordered_map>
