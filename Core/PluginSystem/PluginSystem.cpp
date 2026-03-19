#include "Core/PluginSystem/PluginSystem.h"

namespace Core::PluginSystem {

bool PluginSystem::RegisterPlugin(std::shared_ptr<IPlugin> plugin) {
    if (!plugin) { return false; }

    const std::string name = plugin->GetInfo().Name;

    {
        std::lock_guard lock(m_Mutex);
        if (m_Plugins.count(name)) { return false; }
        // Insert before OnLoad so the plugin can call GetPlugin() on itself.
        m_Plugins.emplace(name, plugin);
    }

    if (!plugin->OnLoad()) {
        std::lock_guard lock(m_Mutex);
        m_Plugins.erase(name);
        return false;
    }

    return true;
}

void PluginSystem::UnregisterPlugin(const std::string& name) {
    std::shared_ptr<IPlugin> plugin;

    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Plugins.find(name);
        if (it == m_Plugins.end()) { return; }
        plugin = std::move(it->second);
        m_Plugins.erase(it);
    }

    // OnUnload invoked outside the lock so plugins can call back into the system.
    plugin->OnUnload();
}

IPlugin* PluginSystem::GetPlugin(const std::string& name) {
    std::lock_guard lock(m_Mutex);
    auto it = m_Plugins.find(name);
    return (it != m_Plugins.end()) ? it->second.get() : nullptr;
}

std::vector<PluginInfo> PluginSystem::GetLoadedPlugins() const {
    std::lock_guard lock(m_Mutex);
    std::vector<PluginInfo> result;
    result.reserve(m_Plugins.size());
    for (const auto& [name, plugin] : m_Plugins) {
        result.push_back(plugin->GetInfo());
    }
    return result;
}

void PluginSystem::UnloadAll() {
    // Snapshot plugin list, clear registry, then unload outside the lock.
    std::vector<std::shared_ptr<IPlugin>> plugins;

    {
        std::lock_guard lock(m_Mutex);
        plugins.reserve(m_Plugins.size());
        for (auto& [name, plugin] : m_Plugins) {
            plugins.push_back(std::move(plugin));
        }
        m_Plugins.clear();
    }

    for (auto& plugin : plugins) {
        plugin->OnUnload();
    }
}

} // namespace Core::PluginSystem
