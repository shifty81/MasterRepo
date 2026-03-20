#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Core {

struct StackFrame {
    std::string function;
    std::string file;
    uint32_t    line      = 0;
    uint64_t    address   = 0;
};

struct CrashInfo {
    std::string              timestamp;
    std::string              reason;
    std::string              thread;
    std::vector<StackFrame>  stackTrace;
    std::vector<std::string> logs;
    std::string              buildVersion;
    std::string              platform;
    uint64_t                 memoryUsedBytes = 0;
};

class CrashReporter {
public:
    static CrashReporter& Instance();

    void Install();
    void Uninstall();
    bool IsInstalled() const;

    bool WriteCrashReport(const CrashInfo& info, const std::string& outputPath = "");
    bool LoadCrashReport(const std::string& path, CrashInfo& out) const;

    void OnCrash(const std::string& reason);

    void SetOutputDir(const std::string& dir);
    void SetBuildVersion(const std::string& version);
    void SetMaxReports(size_t max);

    void AddLog(const std::string& message);
    void SetLastLogs(const std::vector<std::string>& logs);

    std::vector<std::string> ListReports() const;
    bool DeleteReport(const std::string& path);

    using CrashCallback = std::function<void(const CrashInfo&)>;
    void SetCrashCallback(CrashCallback cb);

    uint32_t ReportCount() const;

private:
    CrashReporter() = default;
    std::string generatePath() const;
    CrashInfo   collectInfo(const std::string& reason) const;

    bool        m_installed    = false;
    std::string m_outputDir    = "Logs/Crashes/";
    std::string m_buildVersion = "0.1.0";
    size_t      m_maxReports   = 20;
    CrashCallback m_callback;
    std::vector<std::string> m_recentLogs;
};

} // namespace Core
