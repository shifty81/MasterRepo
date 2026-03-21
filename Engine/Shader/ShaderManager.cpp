#include "Engine/Shader/ShaderManager.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace Engine {

struct ShaderManager::Impl {
    std::unordered_map<std::string, ShaderAsset>  byName;
    std::unordered_map<ShaderID, std::string>      idToName;
    ShaderID nextId{1};
    ShaderID bound{0};

    // File path cache for hot-reload.
    struct SourcePaths { std::string vert, frag; };
    std::unordered_map<std::string, SourcePaths> pathCache;
};

static std::string ReadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

ShaderManager::ShaderManager() : m_impl(new Impl()) {}
ShaderManager::~ShaderManager() { delete m_impl; }

ShaderID ShaderManager::LoadFromFiles(const std::string& name,
                                       const std::string& vertPath,
                                       const std::string& fragPath)
{
    std::string vs = ReadFile(vertPath);
    std::string fs = ReadFile(fragPath);
    ShaderID id = LoadFromSource(name, vs, fs);
    if (id) m_impl->pathCache[name] = {vertPath, fragPath};
    return id;
}

ShaderID ShaderManager::LoadFromSource(const std::string& name,
                                        const std::string& vertSrc,
                                        const std::string& fragSrc)
{
    // Replace existing if same name.
    auto existing = m_impl->byName.find(name);
    if (existing != m_impl->byName.end()) {
        ShaderID old = existing->second.id;
        m_impl->idToName.erase(old);
        m_impl->byName.erase(existing);
    }

    ShaderAsset asset;
    asset.id             = m_impl->nextId++;
    asset.name           = name;
    asset.vertexSource   = vertSrc;
    asset.fragmentSource = fragSrc;
    asset.compiled       = true; // stub: treat as always compiled
    m_impl->byName[name]          = asset;
    m_impl->idToName[asset.id]    = name;
    return asset.id;
}

bool ShaderManager::Reload(const std::string& name) {
    auto it = m_impl->pathCache.find(name);
    if (it == m_impl->pathCache.end()) return false;
    std::string vs = ReadFile(it->second.vert);
    std::string fs = ReadFile(it->second.frag);
    if (vs.empty() || fs.empty()) return false;
    LoadFromSource(name, vs, fs);
    return true;
}

bool ShaderManager::Unload(const std::string& name) {
    auto it = m_impl->byName.find(name);
    if (it == m_impl->byName.end()) return false;
    m_impl->idToName.erase(it->second.id);
    m_impl->pathCache.erase(name);
    m_impl->byName.erase(it);
    return true;
}

void ShaderManager::UnloadAll() {
    m_impl->byName.clear();
    m_impl->idToName.clear();
    m_impl->pathCache.clear();
    m_impl->bound = 0;
}

ShaderID ShaderManager::GetID(const std::string& name) const {
    auto it = m_impl->byName.find(name);
    return it != m_impl->byName.end() ? it->second.id : 0;
}

ShaderAsset* ShaderManager::Get(const std::string& name) {
    auto it = m_impl->byName.find(name);
    return it != m_impl->byName.end() ? &it->second : nullptr;
}

ShaderAsset* ShaderManager::GetByID(ShaderID id) {
    auto it = m_impl->idToName.find(id);
    if (it == m_impl->idToName.end()) return nullptr;
    return Get(it->second);
}

bool ShaderManager::Has(const std::string& name) const {
    return m_impl->byName.count(name) > 0;
}

size_t ShaderManager::Count() const { return m_impl->byName.size(); }

std::vector<std::string> ShaderManager::ListNames() const {
    std::vector<std::string> names;
    names.reserve(m_impl->byName.size());
    for (auto& [n, _] : m_impl->byName) names.push_back(n);
    return names;
}

void ShaderManager::Bind(ShaderID id)  { m_impl->bound = id; /* GPU stub */ }
void ShaderManager::Unbind()           { m_impl->bound = 0;  /* GPU stub */ }

} // namespace Engine
