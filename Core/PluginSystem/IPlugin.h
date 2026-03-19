#pragma once

#include <string>

namespace Core::PluginSystem {

/// Metadata describing a registered plugin.
struct PluginInfo {
    std::string Name;
    std::string Version;
    std::string Description;
};

/// Abstract interface that all plugins must implement.
///
/// The PluginSystem calls OnLoad() immediately after registration and
/// OnUnload() before removal.  Plugins should perform resource acquisition
/// in OnLoad() and release everything in OnUnload().
///
/// Usage:
///   class MyPlugin : public Core::PluginSystem::IPlugin {
///   public:
///       PluginInfo GetInfo() const override {
///           return { "MyPlugin", "1.0", "Does something useful" };
///       }
///       bool OnLoad()   override { /* init */  return true; }
///       void OnUnload() override { /* cleanup */ }
///   };
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /// Return static metadata about this plugin.
    [[nodiscard]] virtual PluginInfo GetInfo() const = 0;

    /// Called when the plugin is registered with the PluginSystem.
    /// Return true on success; false causes registration to fail.
    virtual bool OnLoad() = 0;

    /// Called before the plugin is removed from the PluginSystem.
    virtual void OnUnload() = 0;
};

} // namespace Core::PluginSystem
