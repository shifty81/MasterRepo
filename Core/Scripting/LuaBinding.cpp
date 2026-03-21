#include "Core/Scripting/LuaBinding.h"
#include <sstream>
#include <algorithm>

namespace Core {

// ── ScriptValueToString ───────────────────────────────────────────────────
std::string ScriptValueToString(const ScriptValue& v) {
    if (std::holds_alternative<ScriptNil>(v))    return "nil";
    if (std::holds_alternative<bool>(v))         return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<int64_t>(v))      return std::to_string(std::get<int64_t>(v));
    if (std::holds_alternative<double>(v))       return std::to_string(std::get<double>(v));
    if (std::holds_alternative<std::string>(v))  return std::get<std::string>(v);
    return "?";
}

// ── ScriptModule ─────────────────────────────────────────────────────────
ScriptModule::ScriptModule(std::string ns) : m_namespace(std::move(ns)) {}

const std::string& ScriptModule::Namespace() const { return m_namespace; }

void ScriptModule::Register(ScriptFunction func) {
    m_funcs[func.name] = std::move(func);
}

bool ScriptModule::Has(const std::string& name) const {
    return m_funcs.count(name) > 0;
}

const ScriptFunction* ScriptModule::Get(const std::string& name) const {
    auto it = m_funcs.find(name);
    return it != m_funcs.end() ? &it->second : nullptr;
}

ScriptResult ScriptModule::Call(const std::string& name,
                                 const ScriptArgs& args) const
{
    auto it = m_funcs.find(name);
    if (it == m_funcs.end())
        throw std::runtime_error("ScriptModule: unknown function '" + name + "'");
    if (!it->second.fn)
        return {SVNil()};
    return it->second.fn(args);
}

std::vector<std::string> ScriptModule::ListFunctions() const {
    std::vector<std::string> out;
    out.reserve(m_funcs.size());
    for (const auto& [k,_] : m_funcs) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

size_t ScriptModule::FunctionCount() const { return m_funcs.size(); }

// ── LuaBinding::Impl ─────────────────────────────────────────────────────
struct LuaBinding::Impl {
    std::unordered_map<std::string, ScriptModule> modules;
    ScriptModule global{""};
};

LuaBinding::LuaBinding() : m_impl(new Impl()) {}
LuaBinding::~LuaBinding() { delete m_impl; }

void LuaBinding::RegisterModule(ScriptModule module) {
    m_impl->modules[module.Namespace()] = std::move(module);
}

ScriptModule* LuaBinding::GetModule(const std::string& ns) {
    auto it = m_impl->modules.find(ns);
    return it != m_impl->modules.end() ? &it->second : nullptr;
}
const ScriptModule* LuaBinding::GetModule(const std::string& ns) const {
    auto it = m_impl->modules.find(ns);
    return it != m_impl->modules.end() ? &it->second : nullptr;
}

std::vector<std::string> LuaBinding::Namespaces() const {
    std::vector<std::string> out;
    for (const auto& [k,_] : m_impl->modules) out.push_back(k);
    std::sort(out.begin(), out.end());
    return out;
}

void LuaBinding::Register(ScriptFunction func) {
    m_impl->global.Register(std::move(func));
}

ScriptResult LuaBinding::Call(const std::string& qualifiedName,
                               const ScriptArgs& args) const
{
    // Split "namespace.function" or just "function"
    size_t dot = qualifiedName.rfind('.');
    if (dot != std::string::npos) {
        std::string ns   = qualifiedName.substr(0, dot);
        std::string name = qualifiedName.substr(dot + 1);
        auto it = m_impl->modules.find(ns);
        if (it == m_impl->modules.end())
            throw std::runtime_error("LuaBinding: unknown module '" + ns + "'");
        return it->second.Call(name, args);
    }
    return m_impl->global.Call(qualifiedName, args);
}

bool LuaBinding::CanCall(const std::string& qualifiedName) const {
    size_t dot = qualifiedName.rfind('.');
    if (dot != std::string::npos) {
        std::string ns   = qualifiedName.substr(0, dot);
        std::string name = qualifiedName.substr(dot + 1);
        auto it = m_impl->modules.find(ns);
        return it != m_impl->modules.end() && it->second.Has(name);
    }
    return m_impl->global.Has(qualifiedName);
}

std::vector<std::string> LuaBinding::AllFunctions() const {
    std::vector<std::string> out;
    for (const auto& name : m_impl->global.ListFunctions())
        out.push_back(name);
    for (const auto& [ns, mod] : m_impl->modules)
        for (const auto& name : mod.ListFunctions())
            out.push_back(ns + "." + name);
    std::sort(out.begin(), out.end());
    return out;
}

size_t LuaBinding::TotalFunctions() const {
    size_t total = m_impl->global.FunctionCount();
    for (const auto& [_,mod] : m_impl->modules)
        total += mod.FunctionCount();
    return total;
}

std::string LuaBinding::Dump() const {
    std::ostringstream ss;
    ss << "=== LuaBinding (" << TotalFunctions() << " functions) ===\n";
    for (const auto& fn : AllFunctions()) {
        ss << "  " << fn << "\n";
    }
    return ss.str();
}

} // namespace Core
