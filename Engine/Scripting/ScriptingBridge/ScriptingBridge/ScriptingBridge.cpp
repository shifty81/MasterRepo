#include "Engine/Scripting/ScriptingBridge/ScriptingBridge/ScriptingBridge.h"
#include <unordered_map>
#include <vector>

namespace Engine {

struct ScriptingBridge::Impl {
    std::shared_ptr<IScriptBackend>                backend;
    std::unordered_map<std::string, ScriptFn>      cppFunctions;
    std::function<void(const std::string&)>        onError;
    bool                                           hotReload{false};
    std::string                                    lastError;
    uint64_t                                       nextObjHandle{1};
    std::unordered_map<uint64_t, void*>            objectMap;
    std::vector<std::string>                       loadedFiles;
};

ScriptingBridge::ScriptingBridge()  : m_impl(new Impl) {}
ScriptingBridge::~ScriptingBridge() { Shutdown(); delete m_impl; }

void ScriptingBridge::Init()
{
    if (!m_impl->backend)
        m_impl->backend = std::make_shared<StubScriptBackend>();
}

void ScriptingBridge::Shutdown() { m_impl->loadedFiles.clear(); }

void ScriptingBridge::SetBackend(std::shared_ptr<IScriptBackend> be) { m_impl->backend=be; }
IScriptBackend* ScriptingBridge::GetBackend() const { return m_impl->backend.get(); }

bool ScriptingBridge::LoadFile(const std::string& path)
{
    if (!m_impl->backend) { m_impl->lastError="no backend"; return false; }
    bool ok = m_impl->backend->LoadFile(path);
    if (!ok) { m_impl->lastError=m_impl->backend->LastError(); return false; }
    m_impl->loadedFiles.push_back(path);
    // Register all C++ functions into backend
    for (auto& [name,fn] : m_impl->cppFunctions)
        m_impl->backend->RegisterFunction(name, fn);
    return true;
}

bool ScriptingBridge::LoadString(const std::string& code, const std::string& /*name*/)
{
    if (!m_impl->backend) { m_impl->lastError="no backend"; return false; }
    bool ok = m_impl->backend->LoadString(code);
    if (!ok) m_impl->lastError=m_impl->backend->LastError();
    return ok;
}

ScriptValue ScriptingBridge::CallScript(const std::string& fnName, const ScriptArgs& args)
{
    if (!m_impl->backend) return ScriptValue{};
    auto result = m_impl->backend->Call(fnName, args);
    m_impl->lastError = m_impl->backend->LastError();
    if (!m_impl->lastError.empty() && m_impl->onError) m_impl->onError(m_impl->lastError);
    return result;
}

bool ScriptingBridge::HasFunction(const std::string& name) const
{
    return m_impl->backend && m_impl->backend->HasFunction(name);
}

void ScriptingBridge::RegisterFunction(const std::string& name, ScriptFn fn)
{
    m_impl->cppFunctions[name]=fn;
    if (m_impl->backend) m_impl->backend->RegisterFunction(name, fn);
}

void ScriptingBridge::UnregisterFunction(const std::string& name)
{
    m_impl->cppFunctions.erase(name);
}

void ScriptingBridge::FireEvent(const std::string& eventName, const ScriptArgs& args)
{
    // Convention: call "On<EventName>" in script
    if (m_impl->backend && m_impl->backend->HasFunction("On"+eventName))
        m_impl->backend->Call("On"+eventName, args);
}

std::string ScriptingBridge::LastError() const { return m_impl->lastError; }
void ScriptingBridge::SetOnError(std::function<void(const std::string&)> cb) { m_impl->onError=cb; }

void ScriptingBridge::SetMemoryLimit(uint64_t bytes) { if(m_impl->backend) m_impl->backend->SetMemoryLimit(bytes); }
void ScriptingBridge::SetTimeLimit  (float ms)       { if(m_impl->backend) m_impl->backend->SetTimeLimit(ms); }

void ScriptingBridge::SetHotReload(bool e) { m_impl->hotReload=e; }
void ScriptingBridge::ReloadFile(const std::string& path)
{
    if (m_impl->hotReload) LoadFile(path);
}

uint64_t ScriptingBridge::BindObject(void* obj)
{
    uint64_t h = m_impl->nextObjHandle++;
    m_impl->objectMap[h] = obj;
    return h;
}

void* ScriptingBridge::ResolveObject(uint64_t handle) const
{
    auto it = m_impl->objectMap.find(handle);
    return it!=m_impl->objectMap.end() ? it->second : nullptr;
}

} // namespace Engine
