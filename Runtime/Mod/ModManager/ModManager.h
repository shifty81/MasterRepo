#pragma once
/**
 * @file ModManager.h
 * @brief Mod discovery, manifest loading, hot-load ordering, conflict resolution.
 *
 * Features:
 *   - Mod manifest: id, version, name, author, dependencies, conflicts, loadOrder
 *   - Discovery: scan directory for mod folders / zip files
 *   - Dependency resolution: topological sort → load order list
 *   - Conflict detection: flags conflicting mods, offers resolution strategies
 *   - Hot-load: optional runtime load/unload of mod assets/scripts
 *   - Enable/Disable individual mods
 *   - Persisted enabled-mod list (JSON)
 *   - Mount point: mods can override VFS paths
 *   - On-load / on-unload callbacks per mod
 *   - Version compatibility check
 *
 * Typical usage:
 * @code
 *   ModManager mm;
 *   mm.Init("/game/mods");
 *   mm.Discover();
 *   auto sorted = mm.GetLoadOrder();
 *   for (auto& id : sorted) mm.Load(id);
 *   mm.SetOnLoad([](const std::string& id){ Log("Loaded mod: "+id); });
 * @endcode
 */

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Runtime {

struct ModManifest {
    std::string              id;
    std::string              name;
    std::string              version;      ///< semver string e.g. "1.2.0"
    std::string              author;
    std::string              description;
    std::string              path;         ///< filesystem path to mod root
    std::vector<std::string> dependencies; ///< mod ids this mod requires
    std::vector<std::string> conflicts;    ///< mod ids incompatible with this
    int32_t                  loadOrder{0}; ///< manual hint; lower = earlier
    bool                     enabled{true};
};

enum class ModState : uint8_t { Discovered=0, Enabled, Loaded, Disabled, Error };

struct ModStatus {
    std::string id;
    ModState    state{ModState::Discovered};
    std::string errorMsg;
};

class ModManager {
public:
    ModManager();
    ~ModManager();

    void Init    (const std::string& modsDirectory);
    void Shutdown();

    // Discovery
    uint32_t Discover();                   ///< scans modsDirectory; returns count found
    bool     RegisterManifest(const ModManifest& manifest);

    // Load ordering
    std::vector<std::string> GetLoadOrder() const;  ///< sorted by deps + loadOrder hint
    std::vector<std::string> GetConflicts() const;  ///< pairs of conflicting mod ids (flat)

    // Lifecycle
    bool Load   (const std::string& id);
    bool Unload (const std::string& id);
    void Enable (const std::string& id);
    void Disable(const std::string& id);
    bool IsLoaded  (const std::string& id) const;
    bool IsEnabled (const std::string& id) const;

    // Query
    const ModManifest* GetManifest(const std::string& id)  const;
    ModStatus          GetStatus  (const std::string& id)  const;
    std::vector<ModManifest> GetAll()                       const;
    std::vector<ModManifest> GetEnabled()                   const;
    uint32_t ModCount()                                     const;

    // Persistence
    bool SaveEnabled(const std::string& path) const;
    bool LoadEnabled(const std::string& path);

    // Callbacks
    void SetOnLoad  (std::function<void(const std::string& id)> cb);
    void SetOnUnload(std::function<void(const std::string& id)> cb);
    void SetOnError (std::function<void(const std::string& id, const std::string& err)> cb);

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Runtime
