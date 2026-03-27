#pragma once
/**
 * @file ScriptHotReload.h
 * @brief File-watcher + symbol reload: watch paths, detect change, trigger callback.
 *
 * Features:
 *   - Watch file paths (or directory + extension filter)
 *   - Poll-based modification time check (no inotify dependency)
 *   - Tick(dt): check watched files every pollIntervalSec
 *   - On-change callback per watched path (or global)
 *   - Version counter: incremented on each reload
 *   - Pause / Resume watching
 *   - ForceReload(path): trigger reload immediately
 *   - GetVersion(path) → uint32_t
 *   - ClearAll
 *   - WatchGroup: named group of paths, fire single callback when any changes
 *
 * Typical usage:
 * @code
 *   ScriptHotReload hr;
 *   hr.Init();
 *   hr.Watch("scripts/player.lua", [](const std::string& path){
 *       LuaVM::ReloadFile(path);
 *   });
 *   hr.WatchDir("shaders/", ".glsl", [](const std::string& p){ RecompileShader(p); });
 *   hr.SetPollInterval(0.5f);
 *   // each frame:
 *   hr.Tick(dt);
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Engine {

using HotReloadCallback = std::function<void(const std::string& path)>;

class ScriptHotReload {
public:
    ScriptHotReload();
    ~ScriptHotReload();

    void Init    ();
    void Shutdown();
    void Tick    (float dt);

    // Watch individual file
    void Watch  (const std::string& path, HotReloadCallback cb={});
    void Unwatch(const std::string& path);

    // Watch directory + extension filter (e.g. ".lua")
    void WatchDir  (const std::string& dir, const std::string& ext,
                     HotReloadCallback cb={});
    void UnwatchDir(const std::string& dir);

    // Global fallback callback (fires for any change)
    void SetGlobalCallback(HotReloadCallback cb);

    // Watch group
    void         CreateGroup(const std::string& groupName);
    void         AddToGroup (const std::string& groupName, const std::string& path);
    void         SetGroupCallback(const std::string& groupName, HotReloadCallback cb);

    // Control
    void SetPollInterval(float seconds);
    void Pause          ();
    void Resume         ();
    bool IsPaused       () const;

    // Force / query
    void     ForceReload(const std::string& path);
    uint32_t GetVersion (const std::string& path) const;
    bool     HasChanged (const std::string& path) const;

    void ClearAll();
    std::vector<std::string> GetWatchedPaths() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
