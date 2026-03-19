#include "Core/VersionSystem/VersionSystem.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace Core::VersionSystem {

// ---------------------------------------------------------------------------
// FNV-1a 64-bit constants
// ---------------------------------------------------------------------------

namespace {

constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME        = 1099511628211ULL;

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

VersionSnapshot VersionSystem::CreateSnapshot(const std::string& rootPath,
                                               const std::string& description) {
    VersionSnapshot snap;
    snap.ID          = GenerateID();
    snap.Timestamp   = CurrentTimestamp();
    snap.Description = description;

    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(rootPath,
             fs::directory_options::skip_permission_denied, ec)) {
        if (!entry.is_regular_file(ec)) { continue; }

        FileEntry fe;
        fe.Path = entry.path().string();
        fe.Size = static_cast<std::size_t>(entry.file_size(ec));
        fe.Hash = ComputeFileHash(fe.Path);
        snap.Files.push_back(std::move(fe));
    }

    return snap;
}

void VersionSystem::AddSnapshot(VersionSnapshot snap) {
    std::lock_guard lock(m_Mutex);
    m_History.push_back(std::move(snap));
}

std::vector<VersionSnapshot> VersionSystem::GetHistory() const {
    std::lock_guard lock(m_Mutex);
    return m_History;
}

bool VersionSystem::SaveHistory(const std::string& path) const {
    std::lock_guard lock(m_Mutex);

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) { return false; }

    out << "[\n";
    for (std::size_t si = 0; si < m_History.size(); ++si) {
        const auto& snap = m_History[si];
        out << "  {\n"
            << "    \"id\": \""          << JsonEscape(snap.ID)          << "\",\n"
            << "    \"timestamp\": \""   << JsonEscape(snap.Timestamp)   << "\",\n"
            << "    \"description\": \"" << JsonEscape(snap.Description) << "\",\n"
            << "    \"files\": [\n";

        for (std::size_t fi = 0; fi < snap.Files.size(); ++fi) {
            const auto& fe = snap.Files[fi];
            out << "      {"
                << "\"path\": \""  << JsonEscape(fe.Path)          << "\", "
                << "\"hash\": \""  << JsonEscape(fe.Hash)          << "\", "
                << "\"size\": "    << fe.Size                      << "}";
            if (fi + 1 < snap.Files.size()) { out << ','; }
            out << '\n';
        }

        out << "    ]\n  }";
        if (si + 1 < m_History.size()) { out << ','; }
        out << '\n';
    }
    out << "]\n";

    return out.good();
}

bool VersionSystem::LoadHistory(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) { return false; }

    const std::string content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());

    std::vector<VersionSnapshot> history;

    // Parse snapshot objects: locate each top-level { } block containing "id".
    std::size_t pos = 0;
    while (pos < content.size()) {
        const std::size_t open = content.find('{', pos);
        if (open == std::string::npos) { break; }

        // Find matching close brace at the same nesting depth.
        int depth = 0;
        std::size_t close = open;
        for (std::size_t i = open; i < content.size(); ++i) {
            if (content[i] == '{') { ++depth; }
            else if (content[i] == '}') {
                if (--depth == 0) { close = i; break; }
            }
        }
        if (depth != 0) { break; }

        const std::string block = content.substr(open, close - open + 1);

        // Skip file-entry blocks (they don't have an "id" key at this level).
        if (block.find("\"id\":") == std::string::npos) {
            pos = close + 1;
            continue;
        }

        auto extractStr = [&](const std::string& key) -> std::string {
            const std::string search = "\"" + key + "\": \"";
            auto kp = block.find(search);
            if (kp == std::string::npos) { return {}; }
            kp += search.size();
            auto ep = block.find('"', kp);
            if (ep == std::string::npos) { return {}; }
            return block.substr(kp, ep - kp);
        };

        VersionSnapshot snap;
        snap.ID          = extractStr("id");
        snap.Timestamp   = extractStr("timestamp");
        snap.Description = extractStr("description");

        // Parse file entries within the "files": [...] array.
        const std::size_t filesStart = block.find("\"files\":");
        if (filesStart != std::string::npos) {
            const std::size_t arrOpen  = block.find('[', filesStart);
            const std::size_t arrClose = block.rfind(']');
            if (arrOpen != std::string::npos && arrClose != std::string::npos) {
                const std::string arr = block.substr(arrOpen, arrClose - arrOpen + 1);
                std::size_t fp = 0;
                while (fp < arr.size()) {
                    const std::size_t fo = arr.find('{', fp);
                    const std::size_t fc = arr.find('}', fo);
                    if (fo == std::string::npos || fc == std::string::npos) { break; }
                    const std::string fe_block = arr.substr(fo, fc - fo + 1);

                    auto extractFeStr = [&](const std::string& key) -> std::string {
                        const std::string srch = "\"" + key + "\": \"";
                        auto kp2 = fe_block.find(srch);
                        if (kp2 == std::string::npos) { return {}; }
                        kp2 += srch.size();
                        auto ep2 = fe_block.find('"', kp2);
                        if (ep2 == std::string::npos) { return {}; }
                        return fe_block.substr(kp2, ep2 - kp2);
                    };

                    FileEntry fe;
                    fe.Path = extractFeStr("path");
                    fe.Hash = extractFeStr("hash");

                    const std::string sizeSearch = "\"size\": ";
                    auto sp = fe_block.find(sizeSearch);
                    if (sp != std::string::npos) {
                        sp += sizeSearch.size();
                        fe.Size = static_cast<std::size_t>(std::stoull(fe_block.substr(sp)));
                    }

                    if (!fe.Path.empty()) {
                        snap.Files.push_back(std::move(fe));
                    }
                    fp = fc + 1;
                }
            }
        }

        if (!snap.ID.empty()) {
            history.push_back(std::move(snap));
        }
        pos = close + 1;
    }

    std::lock_guard lock(m_Mutex);
    m_History = std::move(history);
    return true;
}

std::string VersionSystem::ComputeFileHash(const std::string& path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) { return {}; }

    constexpr std::size_t CHUNK = 65536;
    std::vector<uint8_t>  buf(CHUNK);
    uint64_t hash = FNV_OFFSET_BASIS;

    while (in) {
        in.read(reinterpret_cast<char*>(buf.data()), CHUNK);
        const std::streamsize n = in.gcount();
        if (n <= 0) { break; }

        const uint8_t* data = buf.data();
        for (std::streamsize i = 0; i < n; ++i) {
            hash ^= static_cast<uint64_t>(data[i]);
            hash *= FNV_PRIME;
        }
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return ss.str();
}

// --- Private helpers ---

std::string VersionSystem::GenerateID() {
    // Pseudo-UUID: 8-4-4-4-12 hex characters from the current timestamp and
    // a monotonically increasing counter.
    static std::atomic<uint64_t> counter{0};
    const uint64_t cnt = ++counter;

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const uint64_t ms  = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());

    // Mix counter and timestamp bits.
    const uint64_t hi = (ms  >> 16) & 0xFFFFFFFFULL;
    const uint64_t lo = ((ms & 0xFFFF) << 16) | (cnt & 0xFFFF);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8)  << hi
       << '-'
       << std::setw(4)  << ((lo >> 16) & 0xFFFF)
       << '-'
       << std::setw(4)  << (lo & 0xFFFF)
       << '-'
       << std::setw(4)  << ((cnt >> 16) & 0xFFFF)
       << '-'
       << std::setw(12) << (cnt ^ ms);
    return ss.str();
}

std::string VersionSystem::CurrentTimestamp() {
    const auto now  = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string VersionSystem::JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (const char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

} // namespace Core::VersionSystem
