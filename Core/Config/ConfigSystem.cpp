#include "Core/Config/ConfigSystem.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <filesystem>

namespace Core {

// ── Helpers ───────────────────────────────────────────────────────────────
static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

struct ConfigRecord {
    std::string value;
    std::string source;
    int         layer{0};
};

// Multi-layer map: key → sorted list of records (ascending layer = lower priority)
struct ConfigSystem::Impl {
    std::unordered_map<std::string, std::vector<ConfigRecord>> data;
    // Track loaded files for live-reload
    struct FileWatch {
        std::string  path;
        ConfigFormat fmt;
        int          layer;
        std::time_t  mtime{0};
    };
    std::vector<FileWatch> watched;
    std::vector<ConfigChangedCallback> callbacks;
};

ConfigSystem::ConfigSystem() : m_impl(new Impl()) {}
ConfigSystem::~ConfigSystem() { delete m_impl; }

// ── INI parser ────────────────────────────────────────────────────────────
static bool ParseINI(const std::string& content, const std::string& source,
                     int layer, std::unordered_map<std::string,
                     std::vector<ConfigRecord>>& data)
{
    std::istringstream ss(content);
    std::string line, section;
    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') {
            size_t e = line.find(']');
            section = (e != std::string::npos) ? line.substr(1, e - 1) : "";
            continue;
        }
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = Trim(line.substr(0, eq));
        std::string value = Trim(line.substr(eq + 1));
        if (!section.empty()) key = section + "." + key;
        ConfigRecord rec{value, source, layer};
        data[key].push_back(rec);
    }
    return true;
}

// ── Minimal JSON parser ───────────────────────────────────────────────────
static std::string JsonUnescapeStr(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[++i]) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            default: out += s[i]; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

static bool ParseJSON(const std::string& content, const std::string& source,
                      int layer, std::unordered_map<std::string,
                      std::vector<ConfigRecord>>& data)
{
    // Flat JSON: { "section.key": "value", ... } — no nested objects
    size_t pos = 0;
    auto skipWS = [&]{ while (pos < content.size() &&
        (content[pos]==' '||content[pos]=='\t'||
         content[pos]=='\n'||content[pos]=='\r')) ++pos; };
    auto readStr = [&]() -> std::string {
        if (pos >= content.size() || content[pos] != '"') return {};
        ++pos;
        std::string out;
        for (; pos < content.size(); ++pos) {
            char c = content[pos];
            if (c == '"') { ++pos; break; }
            if (c == '\\' && pos + 1 < content.size()) {
                char esc = content[++pos];
                if (esc=='n') out+='\n';
                else if (esc=='t') out+='\t';
                else if (esc=='r') out+='\r';
                else out += esc;
            } else out += c;
        }
        return out;
    };

    skipWS();
    if (pos >= content.size() || content[pos] != '{') return false;
    ++pos;
    while (pos < content.size()) {
        skipWS();
        if (pos >= content.size()) break;
        if (content[pos] == '}') break;
        if (content[pos] == ',') { ++pos; continue; }
        std::string key = readStr();
        if (key.empty()) break;
        skipWS();
        if (pos >= content.size() || content[pos] != ':') break;
        ++pos;
        skipWS();
        std::string value;
        if (pos < content.size() && content[pos] == '"') {
            value = readStr();
        } else {
            size_t start = pos;
            while (pos < content.size() && content[pos] != ',' &&
                   content[pos] != '}' && content[pos] != '\n') ++pos;
            value = Trim(content.substr(start, pos - start));
        }
        if (!key.empty()) data[key].push_back({value, source, layer});
    }
    return true;
}

static ConfigFormat DetectFormat(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return ConfigFormat::INI;
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == "json" ? ConfigFormat::JSON : ConfigFormat::INI;
}

bool ConfigSystem::LoadFile(const std::string& path, ConfigFormat fmt, int layer) {
    std::ifstream f(path);
    if (!f) return false;
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    if (fmt == ConfigFormat::Auto) fmt = DetectFormat(path);
    bool ok = LoadString(content, fmt, path, layer);
    if (ok) {
        std::time_t mtime = 0;
        std::error_code ec;
        auto ft = std::filesystem::last_write_time(path, ec);
        if (!ec) {
            auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::file_clock::to_sys(ft));
            mtime = static_cast<std::time_t>(sctp.time_since_epoch().count());
        }
        m_impl->watched.push_back({path, fmt, layer, mtime});
    }
    return ok;
}

bool ConfigSystem::LoadString(const std::string& content, ConfigFormat fmt,
                               const std::string& sourceName, int layer)
{
    if (fmt == ConfigFormat::JSON)
        return ParseJSON(content, sourceName, layer, m_impl->data);
    return ParseINI(content, sourceName, layer, m_impl->data);
}

void ConfigSystem::SetDefault(const std::string& key, const std::string& value) {
    auto& records = m_impl->data[key];
    // Layer -1 = defaults (lowest priority)
    for (auto& r : records) if (r.layer == -1) { r.value = value; return; }
    records.insert(records.begin(), {value, "<default>", -1});
}

static const std::string& BestValue(const std::vector<ConfigRecord>& records,
                                     const std::string& def)
{
    if (records.empty()) return def;
    // Highest layer wins
    const ConfigRecord* best = &records[0];
    for (const auto& r : records) if (r.layer > best->layer) best = &r;
    return best->value;
}

std::string ConfigSystem::GetString(const std::string& key,
                                     const std::string& def) const
{
    auto it = m_impl->data.find(key);
    if (it == m_impl->data.end()) return def;
    return BestValue(it->second, def);
}

int64_t ConfigSystem::GetInt(const std::string& key, int64_t def) const {
    std::string s = GetString(key);
    if (s.empty()) return def;
    try { return std::stoll(s); } catch (...) { return def; }
}

double ConfigSystem::GetDouble(const std::string& key, double def) const {
    std::string s = GetString(key);
    if (s.empty()) return def;
    try { return std::stod(s); } catch (...) { return def; }
}

bool ConfigSystem::GetBool(const std::string& key, bool def) const {
    std::string s = GetString(key);
    if (s.empty()) return def;
    return s == "1" || s == "true" || s == "yes" || s == "on";
}

void ConfigSystem::SetString(const std::string& key,
                              const std::string& value, int layer)
{
    std::string old = GetString(key);
    auto& records = m_impl->data[key];
    for (auto& r : records) if (r.layer == layer) { r.value = value; goto fire; }
    records.push_back({value, "<runtime>", layer});
fire:
    if (old != value)
        for (auto& cb : m_impl->callbacks) cb(key, old, value);
}
void ConfigSystem::SetInt   (const std::string& k, int64_t v, int l) { SetString(k, std::to_string(v), l); }
void ConfigSystem::SetDouble(const std::string& k, double v, int l)  { SetString(k, std::to_string(v), l); }
void ConfigSystem::SetBool  (const std::string& k, bool v, int l)    { SetString(k, v ? "true" : "false", l); }

bool ConfigSystem::Has(const std::string& key) const {
    return m_impl->data.count(key) > 0;
}

void ConfigSystem::Remove(const std::string& key, int layer) {
    auto it = m_impl->data.find(key);
    if (it == m_impl->data.end()) return;
    auto& records = it->second;
    records.erase(std::remove_if(records.begin(), records.end(),
                  [layer](const ConfigRecord& r){ return r.layer == layer; }),
                  records.end());
    if (records.empty()) m_impl->data.erase(it);
}

std::vector<std::string> ConfigSystem::Keys(const std::string& prefix) const {
    std::vector<std::string> keys;
    for (const auto& [k, _] : m_impl->data)
        if (prefix.empty() || k.find(prefix) == 0) keys.push_back(k);
    std::sort(keys.begin(), keys.end());
    return keys;
}

std::vector<ConfigEntry> ConfigSystem::AllEntries() const {
    std::vector<ConfigEntry> out;
    for (const auto& [key, records] : m_impl->data) {
        for (const auto& r : records)
            out.push_back({key, r.value, r.source, r.layer});
    }
    return out;
}

bool ConfigSystem::SaveLayer(int layer, const std::string& path,
                              ConfigFormat fmt) const
{
    std::ofstream f(path);
    if (!f) return false;
    if (fmt == ConfigFormat::JSON) {
        f << "{\n";
        bool first = true;
        for (const auto& [key, records] : m_impl->data) {
            for (const auto& r : records) {
                if (r.layer != layer) continue;
                if (!first) f << ",\n";
                first = false;
                f << "  \"" << key << "\": \"" << r.value << "\"";
            }
        }
        f << "\n}\n";
    } else {
        // INI
        for (const auto& [key, records] : m_impl->data) {
            for (const auto& r : records) {
                if (r.layer != layer) continue;
                f << key << " = " << r.value << "\n";
            }
        }
    }
    return true;
}

void ConfigSystem::PollReload() {
    for (auto& fw : m_impl->watched) {
        std::error_code ec;
        auto ft = std::filesystem::last_write_time(fw.path, ec);
        if (ec) continue;
        auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
            std::chrono::file_clock::to_sys(ft));
        std::time_t mtime = static_cast<std::time_t>(sctp.time_since_epoch().count());
        if (mtime != fw.mtime) {
            fw.mtime = mtime;
            LoadFile(fw.path, fw.fmt, fw.layer);
        }
    }
}

void ConfigSystem::OnChanged(ConfigChangedCallback cb) {
    m_impl->callbacks.push_back(std::move(cb));
}

void ConfigSystem::Clear(int layer) {
    if (layer < 0) {
        m_impl->data.clear();
    } else {
        for (auto& [key, records] : m_impl->data)
            records.erase(std::remove_if(records.begin(), records.end(),
                          [layer](const ConfigRecord& r){ return r.layer == layer; }),
                          records.end());
    }
}

} // namespace Core
