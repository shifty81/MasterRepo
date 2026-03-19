#include "Engine/Platform/Platform.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__)
#  include <unistd.h>
#  include <sys/sysinfo.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#  include <sys/sysctl.h>
#  include <sys/types.h>
#endif

namespace Engine::Platform {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// File system helpers
// ---------------------------------------------------------------------------

bool FileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool DirectoryExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

bool CreateDirectory(const std::string& path) {
    std::error_code ec;
    fs::create_directories(path, ec);
    return !ec;
}

std::string ReadTextFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool WriteTextFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open())
        return false;
    file << content;
    return file.good();
}

std::vector<std::string> ListFiles(const std::string& dir, const std::string& ext) {
    std::vector<std::string> results;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file())
            continue;
        if (!ext.empty() && entry.path().extension().string() != ext)
            continue;
        results.push_back(entry.path().string());
    }
    return results;
}

std::string GetExecutablePath() {
#if defined(_WIN32)
    char buf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return buf;
#elif defined(__linux__)
    char buf[4096] = {};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) buf[len] = '\0';
    return buf;
#elif defined(__APPLE__)
    char buf[4096] = {};
    uint32_t size = sizeof(buf);
    _NSGetExecutablePath(buf, &size);
    return buf;
#else
    return {};
#endif
}

std::string GetWorkingDirectory() {
    std::error_code ec;
    return fs::current_path(ec).string();
}

// ---------------------------------------------------------------------------
// Time
// ---------------------------------------------------------------------------

uint64_t GetTimeMilliseconds() {
    // steady_clock is monotonic — suitable for measuring durations and elapsed time,
    // not affected by system clock adjustments.
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

std::string GetTimestampISO8601() {
    // system_clock reflects the real wall-clock time, suitable for human-readable
    // timestamps. It may be adjusted by NTP or the user; do not use for durations.
    using namespace std::chrono;
    auto now    = system_clock::now();
    auto tt     = system_clock::to_time_t(now);
    auto ms     = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
#if defined(_WIN32)
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

// ---------------------------------------------------------------------------
// System
// ---------------------------------------------------------------------------

uint32_t GetCPUCoreCount() {
    uint32_t cores = static_cast<uint32_t>(std::thread::hardware_concurrency());
    return cores > 0 ? cores : 1u;
}

uint64_t GetTotalMemoryMB() {
#if defined(_WIN32)
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    return ms.ullTotalPhys / (1024ULL * 1024ULL);
#elif defined(__linux__)
    struct sysinfo si{};
    sysinfo(&si);
    return static_cast<uint64_t>(si.totalram) * si.mem_unit / (1024ULL * 1024ULL);
#elif defined(__APPLE__)
    uint64_t mem = 0;
    size_t len   = sizeof(mem);
    sysctlbyname("hw.memsize", &mem, &len, nullptr, 0);
    return mem / (1024ULL * 1024ULL);
#else
    return 0;
#endif
}

} // namespace Engine::Platform
