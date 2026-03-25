#include "Core/Scripting/ModLoader.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace Core {

void ModLoader::Init(const std::string& modsDir) {
    m_modsDir = modsDir;
    m_mods.clear();
    m_hooks.clear();
    RegisterDefaultHooks();
}

void ModLoader::RegisterDefaultHooks() {
    // Pre-register common hook names so callers can RegisterHook without ScanMods first
    const char* defaultHooks[] = {
        "on_game_start", "on_game_end",
        "on_entity_spawn", "on_entity_die",
        "on_item_use", "on_craft_complete",
        "on_build_placed", "on_build_removed",
        "on_faction_rep_change", "on_level_up",
        "on_pcg_generate", "on_save", "on_load"
    };
    for (const char* h : defaultHooks)
        m_hooks.emplace(h, std::vector<HookFn>{});
}

// ── Discovery ─────────────────────────────────────────────────────────────────

size_t ModLoader::ScanMods() {
    size_t found = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(m_modsDir, ec)) {
        if (!entry.is_directory()) continue;
        std::string jsonPath = entry.path().string() + "/mod.json";
        ModDescriptor desc;
        if (ParseModJson(jsonPath, desc)) {
            m_mods[desc.id] = std::move(desc);
            ++found;
        }
    }
    return found;
}

std::vector<std::string> ModLoader::AllModIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : m_mods) ids.push_back(id);
    return ids;
}

const ModDescriptor* ModLoader::GetDescriptor(const std::string& modId) const {
    auto it = m_mods.find(modId);
    return it != m_mods.end() ? &it->second : nullptr;
}

// ── Load / unload ─────────────────────────────────────────────────────────────

bool ModLoader::Load(const std::string& modId) {
    auto it = m_mods.find(modId);
    if (it == m_mods.end()) return false;
    if (it->second.loaded) return true;
    // In a full implementation, execute the entryScript via a Lua backend.
    // Here we mark it loaded and let any pre-registered C++ hooks take effect.
    it->second.loaded = true;
    FireHook("on_game_start", {});
    return true;
}

bool ModLoader::Unload(const std::string& modId) {
    auto it = m_mods.find(modId);
    if (it == m_mods.end() || !it->second.loaded) return false;
    it->second.loaded = false;
    return true;
}

bool ModLoader::IsLoaded(const std::string& modId) const {
    auto it = m_mods.find(modId);
    return it != m_mods.end() && it->second.loaded;
}

size_t ModLoader::LoadedCount() const {
    size_t n = 0;
    for (const auto& [id, m] : m_mods) if (m.loaded) ++n;
    return n;
}

// ── Hook system ───────────────────────────────────────────────────────────────

void ModLoader::RegisterHook(const std::string& hookName, HookFn fn) {
    m_hooks[hookName].push_back(std::move(fn));
}

void ModLoader::RegisterScriptHook(const std::string& hookName,
                                    const std::string& functionName,
                                    const std::string& /*modId*/) {
    // Wrap the LuaBinding call() into a HookFn
    RegisterHook(hookName, [this, functionName](const HookArgs& args) -> ScriptValue {
        ScriptArgs sargs(args.begin(), args.end());
        auto results = m_binding.Call(functionName, sargs);
        if (results.empty()) return ScriptNil{};
        return results[0];
    });
}

ScriptValue ModLoader::FireHook(const std::string& hookName, const HookArgs& args) {
    auto it = m_hooks.find(hookName);
    if (it == m_hooks.end()) return ScriptNil{};
    ScriptValue last = ScriptNil{};
    for (const auto& fn : it->second) {
        ScriptValue result = fn(args);
        if (!std::holds_alternative<ScriptNil>(result))
            last = result;
    }
    return last;
}

std::vector<std::string> ModLoader::RegisteredHookNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : m_hooks) names.push_back(name);
    return names;
}

// ── Script binding ────────────────────────────────────────────────────────────

void ModLoader::ExposeToScripts(const std::string& name, ScriptFn fn) {
    ScriptFunction sf;
    sf.name = name;
    sf.fn   = std::move(fn);
    m_binding.Register(std::move(sf));
}

LuaBinding& ModLoader::GetBinding() { return m_binding; }

// ── Conflict detection ────────────────────────────────────────────────────────

std::vector<ModConflict> ModLoader::GetConflicts() const {
    // Simple heuristic: any two loaded mods that register the same hook names
    // are flagged as potentially conflicting.
    std::vector<ModConflict> conflicts;
    auto ids = AllModIds();
    for (size_t i = 0; i < ids.size(); ++i)
        for (size_t j = i + 1; j < ids.size(); ++j) {
            if (!IsLoaded(ids[i]) || !IsLoaded(ids[j])) continue;
            // In a full impl, compare asset override lists.
            // Placeholder: no conflicts detected from descriptor alone.
        }
    return conflicts;
}

std::string ModLoader::GetModPath(const std::string& modId) const {
    return m_modsDir + "/" + modId;
}

// ── Private helpers ───────────────────────────────────────────────────────────

bool ModLoader::ParseModJson(const std::string& jsonPath, ModDescriptor& out) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) return false;
    std::string json((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());

    auto readStr = [&](const std::string& key, const std::string& fallback = "") {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return fallback;
        pos = json.find(':', pos);
        if (pos == std::string::npos) return fallback;
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return fallback;
        size_t end = json.find('"', pos + 1);
        return end == std::string::npos ? fallback : json.substr(pos + 1, end - pos - 1);
    };

    out.id          = readStr("id");
    if (out.id.empty()) return false;
    out.name         = readStr("name",         out.id);
    out.version      = readStr("version",      "1.0.0");
    out.author       = readStr("author",       "Unknown");
    out.description  = readStr("description",  "");
    out.entryScript  = readStr("entry",        "main.lua");
    return true;
}

} // namespace Core
