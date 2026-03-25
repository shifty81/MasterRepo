#include "Core/HotReload/HotReloadManager/HotReloadManager.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace Core {

// ── helpers ───────────────────────────────────────────────────────────────────

static uint64_t NowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

static uint64_t FileModifiedMs(const std::string& path) {
    std::error_code ec;
    auto ftime = fs::last_write_time(path, ec);
    if (ec) return 0;
    // Convert file_time_type to ms since epoch via system_clock
    auto sctp = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(ftime));
    return static_cast<uint64_t>(sctp.time_since_epoch().count());
}

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// ── Impl ─────────────────────────────────────────────────────────────────────

struct HotReloadManager::Impl {
    std::unordered_map<std::string, WatchedFile>                    watched;
    std::unordered_map<std::string, uint64_t>                       lastReloadTime;
    std::vector<ReloadEvent>                                        pending;
    std::function<void(const ReloadEvent&)>                         onReload;
    std::map<std::string, std::function<void(const ReloadEvent&)>>  tagCallbacks;

    uint64_t pollingIntervalMs{1000};
    uint64_t lastCheckMs{0};
    uint64_t reloadCount{0};
};

// ── HotReloadManager ─────────────────────────────────────────────────────────

HotReloadManager::HotReloadManager() : m_impl(new Impl{}) {}
HotReloadManager::~HotReloadManager() { delete m_impl; }

void HotReloadManager::Watch(const std::string& path, const std::string& tag) {
    WatchedFile& wf  = m_impl->watched[path];
    wf.path          = path;
    wf.tag           = tag;
    wf.lastModifiedMs = FileModifiedMs(path);
}

void HotReloadManager::Unwatch(const std::string& path) {
    m_impl->watched.erase(path);
}

void HotReloadManager::Clear() {
    m_impl->watched.clear();
    m_impl->pending.clear();
    m_impl->lastReloadTime.clear();
}

void HotReloadManager::WatchDirectory(const std::string& dir,
                                      const std::vector<std::string>& extensions) {
    std::error_code ec;
    for (const auto& de : fs::recursive_directory_iterator(dir, ec)) {
        if (ec) break;
        if (!de.is_regular_file()) continue;
        if (!extensions.empty()) {
            std::string ext = ToLower(de.path().extension().string());
            bool match = false;
            for (const auto& e : extensions) {
                if (ToLower(e) == ext) { match = true; break; }
            }
            if (!match) continue;
        }
        Watch(de.path().string());
    }
}

void HotReloadManager::Tick() {
    uint64_t now = NowMs();
    if (now - m_impl->lastCheckMs < m_impl->pollingIntervalMs) return;
    m_impl->lastCheckMs = now;

    for (auto& [path, wf] : m_impl->watched) {
        uint64_t modMs = FileModifiedMs(path);
        if (modMs == 0) continue;
        if (modMs > wf.lastModifiedMs) {
            wf.lastModifiedMs = modMs;

            ReloadEvent ev;
            ev.path        = path;
            ev.tag         = wf.tag;
            ev.timestampMs = now;

            m_impl->pending.push_back(ev);
            m_impl->lastReloadTime[path] = now;
            ++m_impl->reloadCount;

            if (m_impl->onReload) m_impl->onReload(ev);

            auto it = m_impl->tagCallbacks.find(wf.tag);
            if (it != m_impl->tagCallbacks.end()) it->second(ev);
        }
    }
}

void HotReloadManager::OnReload(std::function<void(const ReloadEvent&)> callback) {
    m_impl->onReload = std::move(callback);
}

void HotReloadManager::OnReloadTag(const std::string& tag,
                                   std::function<void(const ReloadEvent&)> callback) {
    m_impl->tagCallbacks[tag] = std::move(callback);
}

size_t HotReloadManager::WatchedCount() const { return m_impl->watched.size(); }

std::vector<ReloadEvent> HotReloadManager::PendingReloads() const {
    return m_impl->pending;
}

void HotReloadManager::SetPollingInterval(uint64_t ms) {
    m_impl->pollingIntervalMs = ms;
}

bool HotReloadManager::IsWatching(const std::string& path) const {
    return m_impl->watched.count(path) > 0;
}

uint64_t HotReloadManager::LastReloadTime(const std::string& path) const {
    auto it = m_impl->lastReloadTime.find(path);
    return it != m_impl->lastReloadTime.end() ? it->second : 0;
}

HotReloadStats HotReloadManager::Stats() const {
    HotReloadStats s{};
    s.watchedFiles = m_impl->watched.size();
    s.reloadCount  = m_impl->reloadCount;
    s.lastCheckMs  = m_impl->lastCheckMs;
    return s;
}

} // namespace Core
