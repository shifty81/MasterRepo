#include "Core/PackageManager/PackageManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace Core {

// ── registry ───────────────────────────────────────────────────

void PackageManager::AddRegistry(const std::string& name, const std::string& path) {
    for (auto& r : m_registries)
        if (r.name == name) { r.path = path; return; }
    m_registries.push_back({name, path});
}

void PackageManager::RemoveRegistry(const std::string& name) {
    m_registries.erase(
        std::remove_if(m_registries.begin(), m_registries.end(),
                       [&](const Registry& r){ return r.name == name; }),
        m_registries.end());
}

bool PackageManager::RefreshIndex() {
    m_index.clear();
    bool ok = true;
    for (const auto& reg : m_registries)
        if (!LoadRegistryPackages(reg)) ok = false;
    return ok;
}

bool PackageManager::LoadRegistryPackages(const Registry& reg) {
    // Load a simple line-oriented manifest: name|version|description|author
    std::ifstream f(reg.path + "/index.txt");
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        PackageInfo pkg;
        std::getline(ss, pkg.name,        '|');
        std::getline(ss, pkg.version,     '|');
        std::getline(ss, pkg.description, '|');
        std::getline(ss, pkg.author,      '|');
        if (!pkg.name.empty()) m_index.push_back(std::move(pkg));
    }
    return true;
}

// ── query ──────────────────────────────────────────────────────

std::optional<PackageInfo> PackageManager::Find(const std::string& name) const {
    for (const auto& p : m_index)
        if (p.name == name) return p;
    return std::nullopt;
}

std::vector<PackageInfo> PackageManager::Search(const std::string& query) const {
    std::vector<PackageInfo> results;
    for (const auto& p : m_index)
        if (p.name.find(query) != std::string::npos ||
            p.description.find(query) != std::string::npos)
            results.push_back(p);
    return results;
}

std::vector<PackageInfo> PackageManager::ListInstalled() const {
    std::vector<PackageInfo> results;
    for (const auto& p : m_index)
        if (p.installed) results.push_back(p);
    return results;
}

// ── install / uninstall ────────────────────────────────────────

void PackageManager::SetInstallDir(const std::string& dir) { m_installDir = dir; }
void PackageManager::SetProgressCallback(ProgressFn fn)    { m_progress = std::move(fn); }

bool PackageManager::Install(const std::string& name, const std::string& version) {
    auto pkgOpt = Find(name);
    if (!pkgOpt) return false;
    PackageInfo pkg = *pkgOpt;
    if (!version.empty()) pkg.version = version;

    // Install dependencies first
    auto deps = ResolveDependencies(name);
    for (const auto& dep : deps)
        if (!dep.installed) DoInstall(dep);

    return DoInstall(pkg);
}

bool PackageManager::Uninstall(const std::string& name) {
    return DoUninstall(name);
}

bool PackageManager::Update(const std::string& name) {
    DoUninstall(name);
    return Install(name);
}

bool PackageManager::UpdateAll() {
    bool ok = true;
    for (auto& p : m_index)
        if (p.installed) if (!Update(p.name)) ok = false;
    return ok;
}

bool PackageManager::DoInstall(const PackageInfo& pkg) {
    if (m_progress) m_progress(pkg.name, 0);
    // Mark installed in index
    for (auto& p : m_index)
        if (p.name == pkg.name) { p.installed = true; break; }
    if (m_progress) m_progress(pkg.name, 100);
    return true;
}

bool PackageManager::DoUninstall(const std::string& name) {
    for (auto& p : m_index)
        if (p.name == name) { p.installed = false; return true; }
    return false;
}

// ── dependency resolution ──────────────────────────────────────

std::vector<PackageInfo> PackageManager::ResolveDependencies(const std::string& name) const {
    std::vector<PackageInfo> resolved;
    auto pkgOpt = Find(name);
    if (!pkgOpt) return resolved;
    for (const auto& depStr : pkgOpt->dependencies) {
        // Format: "name@version"
        auto atPos = depStr.find('@');
        std::string depName = (atPos != std::string::npos) ? depStr.substr(0, atPos) : depStr;
        auto depOpt = Find(depName);
        if (depOpt) resolved.push_back(*depOpt);
    }
    return resolved;
}

bool PackageManager::HasConflicts(const std::string& name) const {
    (void)name;
    return false; // basic impl — no conflicts detected
}

// ── lock file ─────────────────────────────────────────────────

bool PackageManager::SaveLockFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "# MasterRepo package lock file\n";
    for (const auto& p : m_index)
        if (p.installed)
            f << p.name << "|" << p.version << "\n";
    return true;
}

bool PackageManager::LoadLockFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto pos = line.find('|');
        std::string pkgName = (pos != std::string::npos) ? line.substr(0, pos) : line;
        for (auto& p : m_index)
            if (p.name == pkgName) { p.installed = true; break; }
    }
    return true;
}

} // namespace Core
