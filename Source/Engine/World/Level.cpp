#include "Engine/World/Level.h"
#include "Core/Logging/Log.h"

namespace NF {

void Level::Load(const std::filesystem::path& path) {
    if (m_Loaded) {
        Logger::Log(LogLevel::Warning, "Level",
                    "Load() called on already-loaded level; unloading first.");
        Unload();
    }
    m_Path   = path;
    m_Loaded = true;
    Logger::Log(LogLevel::Info, "Level", "Loaded level: " + path.string());
}

void Level::Unload() {
    if (!m_Loaded) return;
    // Replace the world with a fresh empty one to destroy all entities.
    m_World  = World{};
    m_Loaded = false;
    Logger::Log(LogLevel::Info, "Level", "Unloaded level: " + m_Path.string());
    m_Path.clear();
}

void Level::Update([[maybe_unused]] float dt) {
    // Per-frame level logic (scripting, streaming, …) goes here.
}

} // namespace NF
