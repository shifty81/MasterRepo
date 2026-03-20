#include "Core/CrashReport/CrashReport.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <csignal>
#include <iomanip>
#include <algorithm>

namespace Core {

// ── singleton ─────────────────────────────────────────────────────────────────

CrashReporter& CrashReporter::Instance() {
    static CrashReporter s;
    return s;
}

// ── signal handlers ───────────────────────────────────────────────────────────

static void signalHandler(int sig) {
    std::string reason;
    switch (sig) {
        case SIGSEGV: reason = "SIGSEGV"; break;
        case SIGABRT: reason = "SIGABRT"; break;
        case SIGFPE:  reason = "SIGFPE";  break;
        default:      reason = "SIGNAL(" + std::to_string(sig) + ")"; break;
    }
    CrashReporter::Instance().OnCrash(reason);

    // Re-raise with default handler so the OS records the exit code correctly.
    signal(sig, SIG_DFL);
    raise(sig);
}

void CrashReporter::Install() {
    if (m_installed) return;
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE,  signalHandler);
    m_installed = true;
}

void CrashReporter::Uninstall() {
    if (!m_installed) return;
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE,  SIG_DFL);
    m_installed = false;
}

bool CrashReporter::IsInstalled() const { return m_installed; }

// ── path generation ───────────────────────────────────────────────────────────

std::string CrashReporter::generatePath() const {
    auto now  = std::chrono::system_clock::now();
    auto tt   = std::chrono::system_clock::to_time_t(now);
    std::tm  tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream ss;
    ss << m_outputDir
       << std::put_time(&tm, "%Y%m%d_%H%M%S")
       << "_crash.json";
    return ss.str();
}

// ── collect info ──────────────────────────────────────────────────────────────

CrashInfo CrashReporter::collectInfo(const std::string& reason) const {
    CrashInfo info;
    auto now  = std::chrono::system_clock::now();
    auto tt   = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    info.timestamp    = ts.str();
    info.reason       = reason;
    info.buildVersion = m_buildVersion;
    info.logs         = m_recentLogs;

#if defined(__linux__)
    info.platform = "Linux";
#elif defined(_WIN32)
    info.platform = "Windows";
#elif defined(__APPLE__)
    info.platform = "macOS";
#else
    info.platform = "Unknown";
#endif

    return info;
}

// ── on crash ──────────────────────────────────────────────────────────────────

void CrashReporter::OnCrash(const std::string& reason) {
    CrashInfo info = collectInfo(reason);
    WriteCrashReport(info);
    if (m_callback) m_callback(info);
}

// ── JSON helpers (local, no deps) ─────────────────────────────────────────────

static std::string escJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else           out += c;
    }
    return out;
}

// ── write report ──────────────────────────────────────────────────────────────

bool CrashReporter::WriteCrashReport(const CrashInfo& info, const std::string& outputPath) {
    namespace fs = std::filesystem;

    std::string path = outputPath.empty() ? generatePath() : outputPath;

    // Ensure output directory exists.
    fs::path dir = fs::path(path).parent_path();
    if (!dir.empty()) {
        std::error_code ec;
        fs::create_directories(dir, ec);
    }

    std::ofstream f(path);
    if (!f) return false;

    f << "{\n";
    f << "  \"timestamp\": \""    << escJson(info.timestamp)    << "\",\n";
    f << "  \"reason\": \""       << escJson(info.reason)       << "\",\n";
    f << "  \"thread\": \""       << escJson(info.thread)       << "\",\n";
    f << "  \"buildVersion\": \"" << escJson(info.buildVersion) << "\",\n";
    f << "  \"platform\": \""     << escJson(info.platform)     << "\",\n";
    f << "  \"memoryUsedBytes\": " << info.memoryUsedBytes       << ",\n";

    f << "  \"logs\": [";
    for (size_t i = 0; i < info.logs.size(); ++i) {
        if (i) f << ", ";
        f << "\"" << escJson(info.logs[i]) << "\"";
    }
    f << "],\n";

    f << "  \"stackTrace\": [";
    for (size_t i = 0; i < info.stackTrace.size(); ++i) {
        if (i) f << ", ";
        const auto& fr = info.stackTrace[i];
        f << "{\"function\": \"" << escJson(fr.function) << "\","
          << " \"file\": \""     << escJson(fr.file)     << "\","
          << " \"line\": "       << fr.line              << ","
          << " \"address\": "    << fr.address           << "}";
    }
    f << "]\n}\n";

    if (!f.good()) return false;
    f.close();

    // Prune oldest reports if over limit.
    auto reports = ListReports();
    if (reports.size() > m_maxReports) {
        std::sort(reports.begin(), reports.end());
        size_t toDelete = reports.size() - m_maxReports;
        for (size_t i = 0; i < toDelete; ++i)
            DeleteReport(reports[i]);
    }

    return true;
}

// ── load report ───────────────────────────────────────────────────────────────

bool CrashReporter::LoadCrashReport(const std::string& path, CrashInfo& out) const {
    std::ifstream f(path);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    auto extractStr = [&](const std::string& key) -> std::string {
        std::string pat = "\"" + key + "\": \"";
        auto pos = json.find(pat);
        if (pos == std::string::npos) return {};
        pos += pat.size();
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) { ++pos; }
            val += json[pos++];
        }
        return val;
    };

    out.timestamp    = extractStr("timestamp");
    out.reason       = extractStr("reason");
    out.thread       = extractStr("thread");
    out.buildVersion = extractStr("buildVersion");
    out.platform     = extractStr("platform");
    return true;
}

// ── configuration ─────────────────────────────────────────────────────────────

void CrashReporter::SetOutputDir(const std::string& dir)      { m_outputDir    = dir; }
void CrashReporter::SetBuildVersion(const std::string& ver)   { m_buildVersion = ver; }
void CrashReporter::SetMaxReports(size_t max)                  { m_maxReports   = max; }
void CrashReporter::AddLog(const std::string& msg)            { m_recentLogs.push_back(msg); }
void CrashReporter::SetLastLogs(const std::vector<std::string>& l) { m_recentLogs = l; }
void CrashReporter::SetCrashCallback(CrashCallback cb)        { m_callback = std::move(cb); }

// ── report listing ────────────────────────────────────────────────────────────

std::vector<std::string> CrashReporter::ListReports() const {
    namespace fs = std::filesystem;
    std::vector<std::string> result;
    std::error_code ec;
    if (!fs::exists(m_outputDir, ec)) return result;
    for (auto& entry : fs::directory_iterator(m_outputDir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        auto name = entry.path().filename().string();
        if (name.size() >= 11 &&
            name.substr(name.size() - 11) == "_crash.json")
            result.push_back(entry.path().string());
    }
    return result;
}

bool CrashReporter::DeleteReport(const std::string& path) {
    std::error_code ec;
    return std::filesystem::remove(path, ec);
}

uint32_t CrashReporter::ReportCount() const {
    return static_cast<uint32_t>(ListReports().size());
}

} // namespace Core
