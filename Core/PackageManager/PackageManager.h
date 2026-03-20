#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Package descriptor
// ──────────────────────────────────────────────────────────────

struct PackageInfo {
    std::string name;
    std::string version;       // semver string, e.g. "1.2.3"
    std::string description;
    std::string author;
    std::string repositoryUrl; // local path or remote URL
    std::vector<std::string> dependencies; // "name@version" pairs
    bool installed = false;
};

// ──────────────────────────────────────────────────────────────
// PackageManager — dependency management for plugins & modules
// ──────────────────────────────────────────────────────────────

class PackageManager {
public:
    using ProgressFn = std::function<void(const std::string& package, int percent)>;

    // Registry management
    void AddRegistry(const std::string& name, const std::string& path);
    void RemoveRegistry(const std::string& name);

    // Package index — loaded from registry manifests
    bool RefreshIndex();
    const std::vector<PackageInfo>& Index() const { return m_index; }

    // Query
    std::optional<PackageInfo>        Find(const std::string& name) const;
    std::vector<PackageInfo>          Search(const std::string& query) const;
    std::vector<PackageInfo>          ListInstalled() const;

    // Install / uninstall
    bool Install(const std::string& name, const std::string& version = "");
    bool Uninstall(const std::string& name);
    bool Update(const std::string& name);
    bool UpdateAll();

    // Dependency resolution
    std::vector<PackageInfo> ResolveDependencies(const std::string& name) const;
    bool HasConflicts(const std::string& name) const;

    // Install location
    void SetInstallDir(const std::string& dir);
    const std::string& InstallDir() const { return m_installDir; }

    // Progress callback during install
    void SetProgressCallback(ProgressFn fn);

    // Serialise / restore installed package list
    bool SaveLockFile(const std::string& path) const;
    bool LoadLockFile(const std::string& path);

private:
    struct Registry {
        std::string name;
        std::string path; // local directory or URL
    };

    bool        LoadRegistryPackages(const Registry& reg);
    bool        DoInstall(const PackageInfo& pkg);
    bool        DoUninstall(const std::string& name);
    std::string LockFilePath() const;

    std::vector<Registry>    m_registries;
    std::vector<PackageInfo> m_index;
    std::string              m_installDir;
    ProgressFn               m_progress;
};

} // namespace Core
