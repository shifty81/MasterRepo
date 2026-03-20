#include "Versions/VersionManager.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace Core {

// ──────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────

static std::string VMEscapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

static std::string VMStringBetween(const std::string& src,
                                    const std::string& open,
                                    const std::string& close,
                                    size_t start = 0) {
    auto a = src.find(open, start);
    if (a == std::string::npos) return {};
    a += open.size();
    auto b = src.find(close, a);
    if (b == std::string::npos) return src.substr(a);
    return src.substr(a, b - a);
}

// ──────────────────────────────────────────────────────────────
// Static helpers
// ──────────────────────────────────────────────────────────────

/*static*/ std::string VersionManager::CurrentTimestamp() {
    auto now     = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm   = {};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

/*static*/ std::vector<std::pair<std::string,std::string>>
VersionManager::ParseFlatJson(const std::string& json) {
    std::vector<std::pair<std::string,std::string>> kv;
    size_t pos = 0;
    while (pos < json.size()) {
        auto ka = json.find('"', pos);
        if (ka == std::string::npos) break;
        auto kb = json.find('"', ka + 1);
        if (kb == std::string::npos) break;
        std::string key = json.substr(ka + 1, kb - ka - 1);

        auto colon = json.find(':', kb);
        if (colon == std::string::npos) break;
        size_t valStart = colon + 1;
        while (valStart < json.size() && std::isspace(json[valStart])) ++valStart;

        std::string val;
        if (valStart < json.size() && json[valStart] == '"') {
            auto ve = json.find('"', valStart + 1);
            if (ve == std::string::npos) break;
            val = json.substr(valStart + 1, ve - valStart - 1);
            pos = ve + 1;
        } else {
            auto ve = json.find_first_of(",}\n", valStart);
            val = (ve != std::string::npos)
                      ? json.substr(valStart, ve - valStart)
                      : json.substr(valStart);
            pos = (ve != std::string::npos) ? ve + 1 : json.size();
        }
        kv.emplace_back(std::move(key), std::move(val));
    }
    return kv;
}

// ──────────────────────────────────────────────────────────────
// Private path helpers
// ──────────────────────────────────────────────────────────────

std::string VersionManager::kindSubdir(VersionKind kind) const {
    switch (kind) {
    case VersionKind::Snapshot:    return m_root + "Snapshots/";
    case VersionKind::SceneFrame:  return m_root + "SceneFrames/";
    case VersionKind::ObjectFrame: return m_root + "ObjectFrames/";
    case VersionKind::SystemFrame: return m_root + "SystemFrames/";
    }
    return m_root + "Frames/";
}

std::string VersionManager::entryPath(const VersionEntry& e) const {
    return kindSubdir(e.kind) + std::to_string(e.id) + "_" + e.tag + ".json";
}

// ──────────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────────

VersionManager::VersionManager(const std::string& versionRoot)
    : m_root(versionRoot) {
    if (!m_root.empty() && m_root.back() != '/') m_root += '/';
    // Create subdirectories if they don't exist
    for (auto kind : { VersionKind::Snapshot, VersionKind::SceneFrame,
                       VersionKind::ObjectFrame, VersionKind::SystemFrame }) {
        std::filesystem::create_directories(kindSubdir(kind));
    }
}

// ──────────────────────────────────────────────────────────────
// Create
// ──────────────────────────────────────────────────────────────

uint64_t VersionManager::CreateSnapshot(const std::string& tag,
                                          const std::string& description,
                                          const std::string& dataJson,
                                          const std::string& author) {
    VersionEntry e;
    e.id          = m_nextId++;
    e.kind        = VersionKind::Snapshot;
    e.tag         = tag.empty() ? ("auto_" + CurrentTimestamp()) : tag;
    e.description = description;
    e.author      = author;
    e.timestamp   = CurrentTimestamp();
    e.dataPath    = entryPath(e);
    e.parentId    = 0;

    std::filesystem::create_directories(kindSubdir(e.kind));
    std::ofstream out(e.dataPath);
    if (out) out << dataJson;

    m_entries.push_back(e);
    return e.id;
}

uint64_t VersionManager::CreateFrame(VersionKind kind,
                                      const std::string& tag,
                                      const std::string& dataJson,
                                      uint64_t parentId) {
    VersionEntry e;
    e.id       = m_nextId++;
    e.kind     = kind;
    e.tag      = tag.empty() ? ("frame_" + std::to_string(e.id)) : tag;
    e.timestamp = CurrentTimestamp();
    e.dataPath  = entryPath(e);
    e.parentId  = parentId;

    std::filesystem::create_directories(kindSubdir(kind));
    std::ofstream out(e.dataPath);
    if (out) out << dataJson;

    m_entries.push_back(e);
    return e.id;
}

// ──────────────────────────────────────────────────────────────
// Query
// ──────────────────────────────────────────────────────────────

std::vector<VersionEntry> VersionManager::GetAll() const { return m_entries; }

std::vector<VersionEntry> VersionManager::GetByKind(VersionKind kind) const {
    std::vector<VersionEntry> out;
    for (const auto& e : m_entries)
        if (e.kind == kind) out.push_back(e);
    return out;
}

const VersionEntry* VersionManager::GetById(uint64_t id) const {
    for (const auto& e : m_entries)
        if (e.id == id) return &e;
    return nullptr;
}

const VersionEntry* VersionManager::GetByTag(const std::string& tag) const {
    for (const auto& e : m_entries)
        if (e.tag == tag) return &e;
    return nullptr;
}

std::vector<VersionEntry> VersionManager::GetPinned() const {
    std::vector<VersionEntry> out;
    for (const auto& e : m_entries)
        if (e.pinned) out.push_back(e);
    return out;
}

std::vector<VersionEntry> VersionManager::GetRecent(size_t count) const {
    if (m_entries.empty()) return {};
    size_t start = m_entries.size() > count ? m_entries.size() - count : 0;
    return { m_entries.begin() + static_cast<ptrdiff_t>(start), m_entries.end() };
}

// ──────────────────────────────────────────────────────────────
// Restore
// ──────────────────────────────────────────────────────────────

std::string VersionManager::LoadData(uint64_t id) const {
    const auto* e = GetById(id);
    if (!e) return {};
    std::ifstream f(e->dataPath);
    if (!f) return {};
    return { std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() };
}

bool VersionManager::RestoreTo(uint64_t id, const std::string& outputPath) const {
    std::string data = LoadData(id);
    if (data.empty()) return false;
    std::ofstream out(outputPath);
    if (!out) return false;
    out << data;
    return true;
}

// ──────────────────────────────────────────────────────────────
// Diff
// ──────────────────────────────────────────────────────────────

VersionDiff VersionManager::Diff(uint64_t fromId, uint64_t toId) const {
    VersionDiff diff;
    diff.fromId = fromId;
    diff.toId   = toId;

    std::string dataA = LoadData(fromId);
    std::string dataB = LoadData(toId);

    auto kvA = ParseFlatJson(dataA);
    auto kvB = ParseFlatJson(dataB);

    // Build maps
    std::unordered_map<std::string,std::string> mapA, mapB;
    for (auto& [k,v] : kvA) mapA[k] = v;
    for (auto& [k,v] : kvB) mapB[k] = v;

    for (auto& [k,v] : mapA) {
        auto it = mapB.find(k);
        if (it == mapB.end()) diff.removedKeys.push_back(k);
        else if (it->second != v) diff.modifiedKeys.push_back(k);
    }
    for (auto& [k,v] : mapB) {
        if (mapA.find(k) == mapA.end()) diff.addedKeys.push_back(k);
    }
    return diff;
}

// ──────────────────────────────────────────────────────────────
// Modify
// ──────────────────────────────────────────────────────────────

bool VersionManager::Pin(uint64_t id, bool pin) {
    for (auto& e : m_entries)
        if (e.id == id) { e.pinned = pin; return true; }
    return false;
}

bool VersionManager::Delete(uint64_t id) {
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->id == id) {
            if (it->pinned) return false;
            std::filesystem::remove(it->dataPath);
            m_entries.erase(it);
            return true;
        }
    }
    return false;
}

bool VersionManager::SetDescription(uint64_t id, const std::string& desc) {
    for (auto& e : m_entries)
        if (e.id == id) { e.description = desc; return true; }
    return false;
}

// ──────────────────────────────────────────────────────────────
// Persistence
// ──────────────────────────────────────────────────────────────

static const char* KindToStr(VersionKind k) {
    switch (k) {
    case VersionKind::Snapshot:    return "Snapshot";
    case VersionKind::SceneFrame:  return "SceneFrame";
    case VersionKind::ObjectFrame: return "ObjectFrame";
    case VersionKind::SystemFrame: return "SystemFrame";
    }
    return "Unknown";
}

static VersionKind StrToKind(const std::string& s) {
    if (s == "SceneFrame")  return VersionKind::SceneFrame;
    if (s == "ObjectFrame") return VersionKind::ObjectFrame;
    if (s == "SystemFrame") return VersionKind::SystemFrame;
    return VersionKind::Snapshot;
}

bool VersionManager::SaveIndex(const std::string& indexPath) const {
    std::string path = indexPath.empty() ? (m_root + "index.json") : indexPath;
    std::ofstream out(path);
    if (!out) return false;

    out << "[\n";
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        out << "  {"
            << "\"id\":"         << e.id          << ","
            << "\"kind\":\""     << KindToStr(e.kind) << "\","
            << "\"tag\":\""      << VMEscapeJson(e.tag)         << "\","
            << "\"author\":\""   << VMEscapeJson(e.author)      << "\","
            << "\"timestamp\":\"" << VMEscapeJson(e.timestamp)  << "\","
            << "\"description\":\"" << VMEscapeJson(e.description) << "\","
            << "\"dataPath\":\"" << VMEscapeJson(e.dataPath)    << "\","
            << "\"parentId\":"   << e.parentId    << ","
            << "\"pinned\":"     << (e.pinned ? "true" : "false")
            << "}";
        if (i + 1 < m_entries.size()) out << ",";
        out << "\n";
    }
    out << "]\n";
    return true;
}

bool VersionManager::LoadIndex(const std::string& indexPath) {
    std::string path = indexPath.empty() ? (m_root + "index.json") : indexPath;
    std::ifstream f(path);
    if (!f) return false;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    m_entries.clear();
    m_nextId = 1;

    // Parse each object in the JSON array
    size_t pos = 0;
    while (true) {
        auto obj_start = content.find('{', pos);
        if (obj_start == std::string::npos) break;
        auto obj_end   = content.find('}', obj_start);
        if (obj_end   == std::string::npos) break;
        std::string obj = content.substr(obj_start, obj_end - obj_start + 1);

        VersionEntry e;
        auto idStr = VMStringBetween(obj, "\"id\":", ",");
        if (!idStr.empty()) {
            try { e.id = std::stoull(idStr); } catch (...) {}
        }
        e.kind        = StrToKind(VMStringBetween(obj, "\"kind\":\"",        "\""));
        e.tag         = VMStringBetween(obj, "\"tag\":\"",         "\"");
        e.author      = VMStringBetween(obj, "\"author\":\"",      "\"");
        e.timestamp   = VMStringBetween(obj, "\"timestamp\":\"",   "\"");
        e.description = VMStringBetween(obj, "\"description\":\"", "\"");
        e.dataPath    = VMStringBetween(obj, "\"dataPath\":\"",    "\"");
        auto pidStr   = VMStringBetween(obj, "\"parentId\":",      ",");
        if (!pidStr.empty()) {
            try { e.parentId = std::stoull(pidStr); } catch (...) {}
        }
        e.pinned = obj.find("\"pinned\":true") != std::string::npos;

        if (e.id >= m_nextId) m_nextId = e.id + 1;
        m_entries.push_back(e);
        pos = obj_end + 1;
    }
    return true;
}

size_t VersionManager::Count() const { return m_entries.size(); }

} // namespace Core
