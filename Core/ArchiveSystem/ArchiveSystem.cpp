#include "Core/ArchiveSystem/ArchiveSystem.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace Core::ArchiveSystem {

ArchiveSystem::ArchiveSystem(std::string archiveRoot)
    : m_ArchiveRoot(std::move(archiveRoot))
{
    std::error_code ec;
    fs::create_directories(m_ArchiveRoot, ec);
    // Silently continue — failure will surface on first Archive() call.
}

bool ArchiveSystem::Archive(const std::string& srcPath,
                             const std::string& reason,
                             const std::string& tags) {
    if (!fs::exists(srcPath)) { return false; }

    std::lock_guard lock(m_Mutex);

    // Build a unique archive path: <root>/<ID>_<original filename>
    const std::string id       = GenerateID();
    const std::string origName = fs::path(srcPath).filename().string();
    const fs::path    dest     = fs::path(m_ArchiveRoot) / (id + "_" + origName);

    std::error_code ec;
    fs::create_directories(dest.parent_path(), ec);
    fs::copy_file(srcPath, dest, fs::copy_options::overwrite_existing, ec);
    if (ec) { return false; }

    ArchiveRecord rec;
    rec.ID           = id;
    rec.OriginalPath = srcPath;
    rec.ArchivePath  = dest.string();
    rec.Timestamp    = CurrentTimestamp();
    rec.Reason       = reason;
    rec.Tags         = tags;

    m_Records.push_back(std::move(rec));
    return true;
}

std::vector<ArchiveRecord> ArchiveSystem::GetRecords() const {
    std::lock_guard lock(m_Mutex);
    return m_Records;
}

bool ArchiveSystem::SaveManifest(const std::string& manifestPath) const {
    std::lock_guard lock(m_Mutex);

    std::ofstream out(manifestPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) { return false; }

    out << "[\n";
    for (std::size_t i = 0; i < m_Records.size(); ++i) {
        const auto& r = m_Records[i];
        out << "  {\n"
            << "    \"id\": \""           << JsonEscape(r.ID)           << "\",\n"
            << "    \"originalPath\": \"" << JsonEscape(r.OriginalPath) << "\",\n"
            << "    \"archivePath\": \""  << JsonEscape(r.ArchivePath)  << "\",\n"
            << "    \"timestamp\": \""    << JsonEscape(r.Timestamp)    << "\",\n"
            << "    \"reason\": \""       << JsonEscape(r.Reason)       << "\",\n"
            << "    \"tags\": \""         << JsonEscape(r.Tags)         << "\"\n"
            << "  }";
        if (i + 1 < m_Records.size()) { out << ','; }
        out << '\n';
    }
    out << "]\n";

    return out.good();
}

bool ArchiveSystem::LoadManifest(const std::string& manifestPath) {
    std::ifstream in(manifestPath);
    if (!in.is_open()) { return false; }

    // Simple hand-rolled JSON array parser for our known manifest schema.
    // Uses basic token extraction rather than a full JSON parser to keep the
    // implementation self-contained.

    const std::string content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());

    std::vector<ArchiveRecord> records;

    // Find each {...} object block using depth-aware brace matching to handle
    // '}' characters that may appear inside JSON string values.
    std::size_t pos = 0;
    while (pos < content.size()) {
        const std::size_t open = content.find('{', pos);
        if (open == std::string::npos) { break; }

        int depth = 0;
        std::size_t close = open;
        bool inString = false;
        for (std::size_t i = open; i < content.size(); ++i) {
            const char c = content[i];
            if (c == '"' && (i == 0 || content[i - 1] != '\\')) { inString = !inString; }
            if (!inString) {
                if      (c == '{') { ++depth; }
                else if (c == '}') { if (--depth == 0) { close = i; break; } }
            }
        }
        if (depth != 0) { break; }

        const std::string block = content.substr(open, close - open + 1);

        auto extractField = [&](const std::string& key) -> std::string {
            const std::string search = "\"" + key + "\": \"";
            auto kp = block.find(search);
            if (kp == std::string::npos) { return {}; }
            kp += search.size();
            // Find the closing quote, skipping escaped quotes.
            auto ep = kp;
            while (ep < block.size()) {
                ep = block.find('"', ep);
                if (ep == std::string::npos) { return {}; }
                if (ep == 0 || block[ep - 1] != '\\') { break; }
                ++ep;
            }
            return block.substr(kp, ep - kp);
        };

        ArchiveRecord rec;
        rec.ID           = extractField("id");
        rec.OriginalPath = extractField("originalPath");
        rec.ArchivePath  = extractField("archivePath");
        rec.Timestamp    = extractField("timestamp");
        rec.Reason       = extractField("reason");
        rec.Tags         = extractField("tags");

        if (!rec.ID.empty()) {
            records.push_back(std::move(rec));
        }

        pos = close + 1;
    }

    std::lock_guard lock(m_Mutex);
    m_Records = std::move(records);
    return true;
}

// --- Private helpers ---

std::string ArchiveSystem::GenerateID() {
    // Called with m_Mutex already held.
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    std::ostringstream ss;
    ss << std::hex << ms << '_' << (++m_Counter);
    return ss.str();
}

std::string ArchiveSystem::CurrentTimestamp() {
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

std::string ArchiveSystem::JsonEscape(const std::string& s) {
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

} // namespace Core::ArchiveSystem
