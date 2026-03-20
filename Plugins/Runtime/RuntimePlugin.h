#pragma once
#include "Core/PluginSystem/IPlugin.h"
#include "Runtime/ECS/ECS.h"

namespace Plugins {

// ──────────────────────────────────────────────────────────────
// IRuntimePlugin — interface for ECS / gameplay runtime plugins
// ──────────────────────────────────────────────────────────────
//
// Runtime plugins integrate with the ECS World tick loop.  They can
// register systems, react to world init/shutdown, and receive a per-frame
// tick with the live World reference.  System priority controls ordering
// relative to other runtime plugins; lower values run first.
//
// Usage:
//   class MyPhysicsPlugin : public Plugins::IRuntimePlugin {
//   public:
//       Core::PluginSystem::PluginInfo GetInfo() const override { ... }
//       bool OnLoad()   override { return true; }
//       void OnUnload() override {}
//       void OnWorldInit(Runtime::ECS::World& w)     override { /* register systems */ }
//       void OnWorldShutdown(Runtime::ECS::World& w) override { /* deregister   */ }
//       void OnTick(Runtime::ECS::World& w, float dt) override { /* step physics */ }
//       int  GetSystemPriority() const override { return -10; } // run before default
//   };

class IRuntimePlugin : public Core::PluginSystem::IPlugin {
public:
    static constexpr const char* PluginType = "Runtime";

    ~IRuntimePlugin() override = default;

    // ── World lifecycle ──────────────────────────────────────
    virtual void OnWorldInit    (Runtime::ECS::World& world) = 0;
    virtual void OnWorldShutdown(Runtime::ECS::World& world) = 0;

    // ── Per-frame tick ───────────────────────────────────────
    virtual void OnTick(Runtime::ECS::World& world, float dt) = 0;

    // ── Ordering ─────────────────────────────────────────────
    /// Lower priority values execute first (default 0).
    [[nodiscard]] virtual int GetSystemPriority() const { return 0; }
};

} // namespace Plugins
