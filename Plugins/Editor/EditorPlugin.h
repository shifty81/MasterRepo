#pragma once
#include <string>
#include <vector>
#include "Core/PluginSystem/IPlugin.h"
#include "Editor/Docking/DockingSystem.h"
#include "Editor/UI/EditorLayout.h"

namespace Plugins {

// ──────────────────────────────────────────────────────────────
// IEditorPlugin — interface for editor extension plugins
// ──────────────────────────────────────────────────────────────
//
// Editor plugins may contribute dockable panels, menu items, and
// react to scene lifecycle events.  They are loaded by the Editor
// startup sequence after the core docking system is ready.
//
// Usage:
//   class MyEditorPlugin : public Plugins::IEditorPlugin {
//   public:
//       Core::PluginSystem::PluginInfo GetInfo() const override { ... }
//       bool OnLoad()   override { OnEditorInit(); return true; }
//       void OnUnload() override { OnEditorShutdown(); }
//       void OnEditorInit()     override { /* register panels */ }
//       void OnEditorShutdown() override { /* cleanup */ }
//       std::vector<Editor::IPanel*> GetPanels() override { return { &m_myPanel }; }
//   };

class IEditorPlugin : public Core::PluginSystem::IPlugin {
public:
    static constexpr const char* PluginType = "Editor";

    ~IEditorPlugin() override = default;

    // ── Lifecycle ────────────────────────────────────────────
    virtual void OnEditorInit()     = 0;
    virtual void OnEditorShutdown() = 0;

    // ── Panels ───────────────────────────────────────────────
    /// Return panels that should be registered with the docking system.
    virtual std::vector<Editor::IPanel*> GetPanels() = 0;

    // ── Scene events ─────────────────────────────────────────
    virtual void OnSceneLoaded(const std::string& /*scenePath*/) {}
    virtual void OnSceneUnloaded() {}

    // ── Menu contribution ────────────────────────────────────
    /// Return a flat list of menu path strings, e.g. "View/My Panel".
    virtual std::vector<std::string> GetMenuItems() { return {}; }
};

} // namespace Plugins
