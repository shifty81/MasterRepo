#include "Tools/ScriptRunner/ScriptRunner.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

// Windows / POSIX popen abstraction (same pattern as Agents/CodeAgent/ToolSystem.cpp)
#ifdef _WIN32
#  define POPEN  _popen
#  define PCLOSE _pclose
#else
#  define POPEN  popen
#  define PCLOSE pclose
#endif

namespace Tools {

// ── Language detection ────────────────────────────────────────────────────
ScriptLang DetectLang(const std::string& filePath) {
    auto ext = std::filesystem::path(filePath).extension().string();
    std::string lext = ext;
    std::transform(lext.begin(), lext.end(), lext.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (lext == ".lua")  return ScriptLang::Lua;
    if (lext == ".py")   return ScriptLang::Python;
    if (lext == ".sh")   return ScriptLang::Shell;
    return ScriptLang::Unknown;
}

struct ScriptRunner::Impl {};

ScriptRunner::ScriptRunner() : m_impl(new Impl()) {}
ScriptRunner::~ScriptRunner() { delete m_impl; }

std::string ScriptRunner::FindInterpreter(ScriptLang lang) {
    switch (lang) {
    case ScriptLang::Lua:
        if (std::system("lua5.4 --version >>/dev/null 2>&1") == 0) return "lua5.4";
        if (std::system("lua    --version >>/dev/null 2>&1") == 0) return "lua";
        return "lua";
    case ScriptLang::Python:
        if (std::system("python3 --version >>/dev/null 2>&1") == 0) return "python3";
        return "python";
    case ScriptLang::Shell:
        return "sh";
    default:
        return "";
    }
}

bool ScriptRunner::ValidateScript(const ScriptRunRequest& req,
                                   std::string& outError) const
{
    if (!req.sourceCode.empty()) return true; // string-based; always valid
    if (req.filePath.empty()) { outError = "No file path specified"; return false; }
    if (!std::filesystem::exists(req.filePath)) {
        outError = "Script file not found: " + req.filePath; return false;
    }
    return true;
}

std::string ScriptRunner::buildCommand(const ScriptRunRequest& req) const {
    ScriptLang lang = req.lang;
    if (lang == ScriptLang::Unknown) lang = DetectLang(req.filePath);

    std::string interp = req.interpreterPath.empty()
        ? FindInterpreter(lang) : req.interpreterPath;

    std::ostringstream cmd;
    // Inject env vars inline (POSIX: VAR=val interp ...)
    for (const auto& [k,v] : req.env)
        cmd << k << "=" << v << " ";

    if (!req.workingDir.empty())
        cmd << "cd " << req.workingDir << " && ";

    cmd << interp;

    if (!req.filePath.empty()) {
        cmd << " " << req.filePath;
    } else {
        // Write source to temp file
        // (for string-based runs)
        cmd << " -"; // read from stdin (not supported in all interps, but good default)
    }

    for (const auto& arg : req.args)
        cmd << " " << arg;

    if (req.captureStderr)
        cmd << " 2>&1";

    return cmd.str();
}

ScriptRunResult ScriptRunner::Run(const ScriptRunRequest& req) {
    return RunStreaming(req, nullptr);
}

ScriptRunResult ScriptRunner::RunStreaming(const ScriptRunRequest& req,
                                            OutputLineCb cb)
{
    ScriptRunResult result;
    std::string err;
    if (!ValidateScript(req, err)) {
        result.errorMessage = err;
        result.exitCode     = -1;
        return result;
    }

    std::string cmd = buildCommand(req);
    auto t0 = std::chrono::steady_clock::now();

    FILE* fp = POPEN(cmd.c_str(), "r");
    if (!fp) {
        result.errorMessage = "Failed to launch: " + cmd;
        result.exitCode     = -1;
        return result;
    }

    char buf[512];
    std::ostringstream capture;
    float timeout = req.timeoutSec > 0.0f ? req.timeoutSec : 30.0f;

    while (std::fgets(buf, sizeof(buf), fp)) {
        std::string line(buf);
        // Strip trailing newline
        if (!line.empty() && line.back() == '\n') line.pop_back();
        capture << line << "\n";
        if (cb) cb(line);

        // Approximate timeout check (not real-time accurate with blocking popen)
        auto elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - t0).count();
        if (elapsed > timeout) {
            PCLOSE(fp);
            result.timedOut    = true;
            result.stdoutCapture = capture.str();
            result.wallTimeMs = elapsed * 1000.0;
            return result;
        }
    }

    int rc = PCLOSE(fp);
    auto elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - t0).count();

    result.stdoutCapture = capture.str();
    result.wallTimeMs    = elapsed * 1000.0;
#ifdef _WIN32
    result.exitCode = rc;
#else
    result.exitCode = WIFEXITED(rc) ? WEXITSTATUS(rc) : rc;
#endif
    return result;
}

ScriptRunResult ScriptRunner::RunFile(const std::string& path, float timeoutSec) {
    ScriptRunRequest req;
    req.filePath   = path;
    req.lang       = DetectLang(path);
    req.timeoutSec = timeoutSec;
    return Run(req);
}

ScriptRunResult ScriptRunner::RunString(ScriptLang lang,
                                         const std::string& code,
                                         float timeoutSec)
{
    // Write to temp file then execute
    namespace fs = std::filesystem;
    std::string ext;
    switch (lang) {
    case ScriptLang::Lua:    ext = ".lua";  break;
    case ScriptLang::Python: ext = ".py";   break;
    case ScriptLang::Shell:  ext = ".sh";   break;
    default:                 ext = ".txt";  break;
    }
    std::string tmp = (fs::temp_directory_path() / ("sr_tmp" + ext)).string();
    if (FILE* f = std::fopen(tmp.c_str(), "w")) {
        std::fwrite(code.c_str(), 1, code.size(), f);
        std::fclose(f);
    }
    ScriptRunRequest req;
    req.filePath   = tmp;
    req.lang       = lang;
    req.timeoutSec = timeoutSec;
    auto result = Run(req);
    std::remove(tmp.c_str());
    return result;
}

} // namespace Tools
