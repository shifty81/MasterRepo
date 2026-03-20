#pragma once
#include <cstdint>
#include "Core/PluginSystem/IPlugin.h"
#include "Builder/Parts/PartDef.h"

namespace Plugins {

// ──────────────────────────────────────────────────────────────
// IBuilderPlugin — interface for ship/structure builder plugins
// ──────────────────────────────────────────────────────────────
//
// Builder plugins contribute part libraries and can react to
// assembly events (placement, removal, validation).
//
// Usage:
//   class MyShipPartsPlugin : public Plugins::IBuilderPlugin {
//   public:
//       Core::PluginSystem::PluginInfo GetInfo() const override { ... }
//       bool OnLoad()   override { m_lib.LoadFromDisk("..."); return true; }
//       void OnUnload() override {}
//       Builder::PartLibrary* GetPartLibrary() override { return &m_lib; }
//       int GetModuleCount() const override { return m_lib.Count(); }
//   };

class IBuilderPlugin : public Core::PluginSystem::IPlugin {
public:
    static constexpr const char* PluginType = "Builder";

    ~IBuilderPlugin() override = default;

    // ── Part contribution ────────────────────────────────────
    /// Return the PartLibrary this plugin contributes; must remain valid
    /// for the lifetime of the plugin.
    virtual Builder::PartLibrary* GetPartLibrary() = 0;

    /// Number of part modules (PartDef entries) in the contributed library.
    [[nodiscard]] virtual int GetModuleCount() const = 0;

    // ── Assembly events ──────────────────────────────────────
    virtual void OnPartPlaced(uint32_t /*defId*/, uint32_t /*instanceId*/) {}
    virtual void OnPartRemoved(uint32_t /*instanceId*/) {}
    virtual void OnAssemblyValidated(bool /*valid*/) {}
};

} // namespace Plugins
