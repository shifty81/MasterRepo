#include "Core/HotReload/HotReload.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace Core {

// ── singleton ─────────────────────────────────────────────────────────────────

HotReload& HotReload::Instance() {
    static HotReload s;
    return s;
}

// ── FNV-1a hash ───────────────────────────────────────────────────────────────

std::string HotReload::hashFile(const std::string& path) const {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};

    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME  = 1099511628211ULL;
    uint64_t hash = FNV_OFFSET;

    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
        for (std::streamsize i = 0; i < f.gcount(); ++i) {
            hash ^= static_cast<uint8_t>(buf[i]);
            hash *= FNV_PRIME;
        }
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

// ── change detection ──────────────────────────────────────────────────────────

bool HotReload::hasChanged(const WatchEntry& e) const {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(e.path, ec)) return false;
    std::string current = hashFile(e.path);
    return !current.empty() && current != e.lastHash;
}

// ── watch management ──────────────────────────────────────────────────────────

void HotReload::Watch(const std::string& path, HotReloadKind kind) {
    for (auto& e : m_entries)
        if (e.path == path) return; // already watching

    WatchEntry entry;
    entry.path    = path;
    entry.kind    = kind;
    entry.lastHash = hashFile(path);
    entry.lastModified = std::chrono::system_clock::now();
    m_entries.push_back(std::move(entry));
}

void HotReload::WatchDirectory(const std::string& dir, HotReloadKind kind) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(dir, ec)) return;
    for (auto& entry : fs::recursive_directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec))
            Watch(entry.path().string(), kind);
    }
}

void HotReload::Unwatch(const std::string& path) {
    m_entries.erase(
        std::remove_if(m_entries.begin(), m_entries.end(),
                       [&](const WatchEntry& e) { return e.path == path; }),
        m_entries.end());
}

void HotReload::UnwatchAll() {
    m_entries.clear();
}

// ── polling ───────────────────────────────────────────────────────────────────

std::vector<ReloadEvent> HotReload::Poll() {
    std::vector<ReloadEvent> events;
    if (!m_enabled) return events;

    for (auto& entry : m_entries) {
        if (!entry.enabled) continue;
        if (!hasChanged(entry)) continue;

        ReloadEvent ev;
        ev.path    = entry.path;
        ev.kind    = entry.kind;
        ev.success = true;

        // Update tracking state.
        entry.lastHash     = hashFile(entry.path);
        entry.lastModified = std::chrono::system_clock::now();
        ++m_totalReloads;

        // Dispatch callback.
        auto it = m_callbacks.find(static_cast<int>(entry.kind));
        if (it != m_callbacks.end() && it->second) {
            try {
                it->second(ev);
            } catch (...) {
                ev.success = true; // still report event, callback threw
            }
        }
        events.push_back(ev);
    }
    return events;
}

// ── callbacks ─────────────────────────────────────────────────────────────────

void HotReload::SetCallback(HotReloadKind kind, ReloadCallback cb) {
    m_callbacks[static_cast<int>(kind)] = std::move(cb);
}

// ── force reload ──────────────────────────────────────────────────────────────

ReloadEvent HotReload::ForceReload(const std::string& path) {
    ReloadEvent ev;
    ev.path = path;
    for (auto& entry : m_entries) {
        if (entry.path != path) continue;
        ev.kind    = entry.kind;
        ev.success = true;
        entry.lastHash     = hashFile(path);
        entry.lastModified = std::chrono::system_clock::now();
        ++m_totalReloads;
        auto it = m_callbacks.find(static_cast<int>(entry.kind));
        if (it != m_callbacks.end() && it->second) it->second(ev);
        return ev;
    }
    ev.success      = false;
    ev.errorMessage = "Path not watched: " + path;
    ++m_failedReloads;
    return ev;
}

// ── accessors ─────────────────────────────────────────────────────────────────

const std::vector<WatchEntry>& HotReload::Entries() const { return m_entries; }
size_t  HotReload::WatchCount()  const { return m_entries.size(); }
bool    HotReload::IsEnabled()   const { return m_enabled; }
void    HotReload::SetEnabled(bool e)  { m_enabled = e; }
uint64_t HotReload::TotalReloads()  const { return m_totalReloads; }
uint64_t HotReload::FailedReloads() const { return m_failedReloads; }

bool HotReload::IsWatching(const std::string& path) const {
    for (auto& e : m_entries)
        if (e.path == path) return true;
    return false;
}

} // namespace Core
