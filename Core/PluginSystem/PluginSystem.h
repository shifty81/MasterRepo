#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/PluginSystem/IPlugin.h"

namespace Core::PluginSystem {

/// Thread-safe registry that owns and manages plugin lifetimes.
///
/// Plugins are registered as shared_ptr<IPlugin>.  On registration the
/// system calls IPlugin::OnLoad(); on removal it calls IPlugin::OnUnload().
/// The PluginSystem retains shared ownership so external code may safely
/// hold IPlugin* pointers obtained via GetPlugin() for the duration a plugin
/// remains registered.
///
/// Usage:
///   Core::PluginSystem::PluginSystem ps;
///
///   auto p = std::make_shared<MyPlugin>();
///   if (ps.RegisterPlugin(p)) {
///       ps.GetPlugin("MyPlugin")->OnLoad(); // already called internally
///   }
///
///   ps.UnregisterPlugin("MyPlugin");
///   ps.UnloadAll();
class PluginSystem {
public:
    PluginSystem()  = default;
    ~PluginSystem() { UnloadAll(); }

    PluginSystem(const PluginSystem&)            = delete;
    PluginSystem& operator=(const PluginSystem&) = delete;

    /// Register a plugin and invoke IPlugin::OnLoad().
    /// Returns false if a plugin with the same name is already registered
    /// or if OnLoad() returns false.
    bool RegisterPlugin(std::shared_ptr<IPlugin> plugin);

    /// Invoke IPlugin::OnUnload() and remove the plugin by name.
    /// No-op if the name is not registered.
    void UnregisterPlugin(const std::string& name);

    /// Return a raw pointer to the plugin with the given name, or nullptr.
    /// The pointer is only valid while the plugin remains registered.
    [[nodiscard]] IPlugin* GetPlugin(const std::string& name);

    /// Return metadata snapshots for every currently loaded plugin.
    [[nodiscard]] std::vector<PluginInfo> GetLoadedPlugins() const;

    /// Unload and remove every registered plugin.
    void UnloadAll();

private:
    mutable std::mutex                                      m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IPlugin>> m_Plugins;
};

} // namespace Core::PluginSystem
