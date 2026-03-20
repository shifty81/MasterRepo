#include "Runtime/World/SceneManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace Runtime::World {

// ─────────────────────────────────────────────────────────────────────────────
// Scene load / save / new
// ─────────────────────────────────────────────────────────────────────────────

bool SceneManager::LoadScene(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;

    std::string data((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());

    SceneDesc desc;
    desc.path     = path;
    desc.isLoaded = true;
    desc.isDirty  = false;

    // Extract name from JSON: look for "name": "..."
    auto nameKey = data.find("\"name\"");
    if (nameKey != std::string::npos) {
        auto colon = data.find(':', nameKey);
        if (colon != std::string::npos) {
            auto q1 = data.find('"', colon + 1);
            if (q1 != std::string::npos) {
                auto q2 = data.find('"', q1 + 1);
                if (q2 != std::string::npos)
                    desc.name = data.substr(q1 + 1, q2 - q1 - 1);
            }
        }
    }
    if (desc.name.empty()) desc.name = path;

    m_current = desc;
    AddToRecent(desc);
    if (!DeserializeFromString(data)) {
        // Non-fatal: file loaded but content couldn't be parsed
    }
    return true;
}

bool SceneManager::SaveScene(const std::string& path) {
    std::string data;
    if (!SerializeToString(data)) return false;

    std::ofstream f(path);
    if (!f) return false;
    f << data;
    m_current.path    = path;
    m_current.isDirty = false;
    return true;
}

bool SceneManager::NewScene(const std::string& name) {
    m_current       = {};
    m_current.name  = name;
    m_current.isLoaded = true;
    m_current.isDirty  = true;
    return true;
}

bool SceneManager::UnloadScene() {
    m_current = {};
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// State queries
// ─────────────────────────────────────────────────────────────────────────────

bool             SceneManager::IsLoaded()         const { return m_current.isLoaded; }
const SceneDesc& SceneManager::GetCurrentScene()  const { return m_current; }
void             SceneManager::MarkDirty(bool d)        { m_current.isDirty = d; }

// ─────────────────────────────────────────────────────────────────────────────
// Recent scenes
// ─────────────────────────────────────────────────────────────────────────────

std::vector<SceneDesc> SceneManager::GetRecentScenes() const {
    return m_recentScenes;
}

void SceneManager::AddToRecent(const SceneDesc& desc) {
    // Remove duplicate entry with same path
    m_recentScenes.erase(
        std::remove_if(m_recentScenes.begin(), m_recentScenes.end(),
                       [&](const SceneDesc& s){ return s.path == desc.path; }),
        m_recentScenes.end());
    m_recentScenes.insert(m_recentScenes.begin(), desc);
    if (static_cast<int>(m_recentScenes.size()) > MaxRecent)
        m_recentScenes.resize(MaxRecent);
}

void SceneManager::ClearRecent() {
    m_recentScenes.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Serialization (minimal JSON-lines format)
// ─────────────────────────────────────────────────────────────────────────────

bool SceneManager::SerializeToString(std::string& out) const {
    // Emit a minimal JSON object capturing scene metadata
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << m_current.name << "\",\n";
    ss << "  \"path\": \"" << m_current.path << "\",\n";
    ss << "  \"isDirty\": " << (m_current.isDirty ? "true" : "false") << "\n";
    ss << "}\n";
    out = ss.str();
    return true;
}

bool SceneManager::DeserializeFromString(const std::string& data) {
    // Parse "name" and "path" from a simple JSON string (no external lib)
    auto extract = [&](const std::string& key) -> std::string {
        auto pos = data.find("\"" + key + "\"");
        if (pos == std::string::npos) return {};
        auto colon = data.find(':', pos);
        if (colon == std::string::npos) return {};
        auto q1 = data.find('"', colon + 1);
        if (q1 == std::string::npos) return {};
        auto q2 = data.find('"', q1 + 1);
        if (q2 == std::string::npos) return {};
        return data.substr(q1 + 1, q2 - q1 - 1);
    };

    std::string name = extract("name");
    std::string path = extract("path");
    if (!name.empty()) m_current.name = name;
    if (!path.empty()) m_current.path = path;
    m_current.isLoaded = true;
    return true;
}

} // namespace Runtime::World
