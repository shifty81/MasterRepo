#pragma once
/**
 * @file ModLoader.h
 * @brief Modding support — load mod packages and expose hook points to scripts.
 *
 * Enables:
 *   - Scan a mods directory for mod descriptor files (mod.json)
 *   - Load / unload mods at runtime
 *   - Hook registry: game systems fire named hooks; mods register callbacks
 *   - Integration with Core::LuaBinding for Lua-scripted mods
 *   - Conflict detection (two mods modifying the same asset/system)
 *
 * Hooks supported by default:
 *   "on_game_start", "on_entity_spawn", "on_entity_die",
 *   "on_item_use", "on_craft_complete", "on_build_placed",
 *   "on_faction_rep_change", "on_level_up", "on_pcg_generate"
 *
 * Usage:
 * @code
 *   ModLoader loader;
 *   loader.Init("Projects/NovaForge/Mods");
 *   loader.ScanMods();
 *   loader.LoadMod("example_mod");
 *   loader.FireHook("on_game_start", {});
 * @endcode
 */

#include "Core/Scripting/LuaBinding.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Core {

// ── Mod descriptor ────────────────────────────────────────────────────────────

struct ModDescriptor {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string entryScript;   ///< relative path to main Lua script
    std::vector<std::string> dependencies;
    bool loaded{false};
};

// ── Hook argument (extends ScriptValue) ──────────────────────────────────────

using HookArgs = std::vector<ScriptValue>;
using HookFn   = std::function<ScriptValue(const HookArgs&)>;
// ── Conflict record ───────────────────────────────────────────────────────────

struct ModConflict {
    std::string modA;
    std::string modB;
    std::string resourcePath;  ///< asset / hook that both mods touch
};

// ── ModLoader ─────────────────────────────────────────────────────────────────

class ModLoader {
public:
    /// @param modsDir  Directory that contains per-mod subdirectories.
    void Init(const std::string& modsDir = "Mods");

    // ── Discovery ─────────────────────────────────────────────────────────
    /// Scan modsDir for mod.json files and populate the known-mods list.
    size_t ScanMods();

    /// All discovered mod IDs (loaded or not).
    std::vector<std::string> AllModIds() const;

    const ModDescriptor* GetDescriptor(const std::string& modId) const;

    // ── Load / unload ──────────────────────────────────────────────────────
    bool Load(const std::string& modId);
    bool Unload(const std::string& modId);
    bool IsLoaded(const std::string& modId) const;
    size_t LoadedCount() const;

    // ── Hook system ───────────────────────────────────────────────────────
    /// Register a C++ callback under a hook name.
    void RegisterHook(const std::string& hookName, HookFn fn);

    /// Register a scripted hook (the Lua function is called via LuaBinding).
    void RegisterScriptHook(const std::string& hookName,
                             const std::string& functionName,
                             const std::string& modId);

    /// Fire all callbacks registered for a hook.
    /// Returns the last non-nil result, or ScriptNil.
    ScriptValue FireHook(const std::string& hookName, const HookArgs& args = {});

    std::vector<std::string> RegisteredHookNames() const;

    // ── Script binding ────────────────────────────────────────────────────
    /// Expose a C++ function to all loaded mod scripts.
    void ExposeToScripts(const std::string& name, ScriptFn fn);

    LuaBinding& GetBinding();

    // ── Conflict detection ────────────────────────────────────────────────
    std::vector<ModConflict> GetConflicts() const;

    // ── Execution path ────────────────────────────────────────────────────
    std::string GetModPath(const std::string& modId) const;

private:
    bool ParseModJson(const std::string& jsonPath, ModDescriptor& out);
    void RegisterDefaultHooks();

    std::string m_modsDir;
    std::unordered_map<std::string, ModDescriptor> m_mods;

    /// hookName → list of callbacks
    std::unordered_map<std::string, std::vector<HookFn>> m_hooks;

    LuaBinding m_binding;
};

} // namespace Core
