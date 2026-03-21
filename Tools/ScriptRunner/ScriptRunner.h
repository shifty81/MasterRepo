#pragma once
/**
 * @file ScriptRunner.h
 * @brief Execute Lua/Python scripts from the IDE with captured stdout/stderr, env, timeout.
 *
 * ScriptRunner provides a safe subprocess-based script execution layer:
 *   - Configurable interpreter path and working directory
 *   - Environment variable injection per run
 *   - Stdout + stderr capture via pipe (non-blocking, line-buffered)
 *   - Timeout enforcement: kills process after deadline
 *   - ExitCode + captured output returned in RunResult
 *
 * NOTE: Actual subprocess spawning is OS-dependent (POSIX popen/fork or
 *       Windows CreateProcess). The implementation uses FILE* popen for
 *       portability, with stderr redirect to stdout ("2>&1").
 */

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Tools {

// ── Script language tag ────────────────────────────────────────────────────
enum class ScriptLang : uint8_t { Lua, Python, Shell, Unknown };

ScriptLang DetectLang(const std::string& filePath);

// ── Run request ───────────────────────────────────────────────────────────
struct ScriptRunRequest {
    std::string filePath;                          ///< Script file to execute
    std::string sourceCode;                        ///< Alternative: run from string
    std::vector<std::string> args;                ///< Positional arguments
    std::unordered_map<std::string, std::string> env; ///< Extra env vars
    std::string workingDir;                        ///< Override cwd
    float       timeoutSec{10.0f};                ///< Kill after this many seconds
    ScriptLang  lang{ScriptLang::Unknown};
    std::string interpreterPath;                   ///< Override (e.g. "/usr/bin/python3")
    bool        captureStderr{true};              ///< Merge stderr into output
};

// ── Run result ────────────────────────────────────────────────────────────
struct ScriptRunResult {
    int32_t     exitCode{0};
    bool        timedOut{false};
    bool        killed{false};
    std::string stdoutCapture;
    std::string stderrCapture;
    double      wallTimeMs{0.0};
    std::string errorMessage;  ///< Set if runner itself failed to launch
    bool Success() const { return !timedOut && !killed && exitCode == 0; }
};

// ── Output callback (streaming) ────────────────────────────────────────────
using OutputLineCb = std::function<void(const std::string& line)>;

// ── Runner ────────────────────────────────────────────────────────────────
class ScriptRunner {
public:
    ScriptRunner();
    ~ScriptRunner();

    // ── interpreter discovery ─────────────────────────────────
    static std::string FindInterpreter(ScriptLang lang);

    // ── synchronous execution ─────────────────────────────────
    ScriptRunResult Run(const ScriptRunRequest& req);

    // ── streaming (line-by-line callback) ─────────────────────
    /// Calls @p cb for each output line as it arrives.
    /// Blocks until script exits or timeout.
    ScriptRunResult RunStreaming(const ScriptRunRequest& req,
                                 OutputLineCb cb);

    // ── convenience wrappers ──────────────────────────────────
    ScriptRunResult RunFile(const std::string& path,
                             float timeoutSec = 10.0f);
    ScriptRunResult RunString(ScriptLang lang,
                               const std::string& code,
                               float timeoutSec = 5.0f);

    // ── sandbox helpers ───────────────────────────────────────
    /// Validate script file (exists, extension matches lang).
    bool ValidateScript(const ScriptRunRequest& req,
                        std::string& outError) const;

private:
    struct Impl;
    Impl* m_impl{nullptr};

    std::string buildCommand(const ScriptRunRequest& req) const;
};

} // namespace Tools
