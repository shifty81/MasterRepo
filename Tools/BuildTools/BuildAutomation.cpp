#include "Tools/BuildTools/BuildAutomation.h"
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <filesystem>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// BuildResult helpers
// ──────────────────────────────────────────────────────────────

static std::vector<std::string> FilterLines(const std::string& text,
                                              const std::string& token) {
    std::vector<std::string> out;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find(token) != std::string::npos)
            out.push_back(line);
    }
    return out;
}

std::vector<std::string> BuildResult::Errors()   const {
    return FilterLines(output, " error:");
}
std::vector<std::string> BuildResult::Warnings() const {
    return FilterLines(output, " warning:");
}

// ──────────────────────────────────────────────────────────────
// Utilities
// ──────────────────────────────────────────────────────────────

std::string BuildAutomation::BuildTypeName(BuildType t) {
    switch (t) {
    case BuildType::Debug:         return "Debug";
    case BuildType::Release:       return "Release";
    case BuildType::RelWithDebInfo: return "RelWithDebInfo";
    case BuildType::MinSizeRel:    return "MinSizeRel";
    }
    return "Debug";
}

std::string BuildAutomation::PlatformToolchain(BuildPlatform p) {
    switch (p) {
    case BuildPlatform::Linux:   return "linux-toolchain.cmake";
    case BuildPlatform::Windows: return "windows-toolchain.cmake";
    case BuildPlatform::MacOS:   return "macos-toolchain.cmake";
    case BuildPlatform::Web:     return "emscripten.cmake";
    default:                     return "";
    }
}

// ──────────────────────────────────────────────────────────────
// Command runner (popen)
// ──────────────────────────────────────────────────────────────

BuildResult BuildAutomation::RunCommand(const std::string& cmd, OutputFn onLine) {
    BuildResult res;
    std::string fullCmd = cmd + " 2>&1";
#if defined(_WIN32)
    FILE* pipe = _popen(fullCmd.c_str(), "r");
#else
    FILE* pipe = popen(fullCmd.c_str(), "r");
#endif
    if (!pipe) {
        res.errorSummary = "Failed to launch: " + cmd;
        return res;
    }
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        // Remove trailing newline
        if (!line.empty() && line.back() == '\n') line.pop_back();
        res.output += line + "\n";
        if (onLine) onLine(line);
    }
#if defined(_WIN32)
    res.exitCode = _pclose(pipe);
#else
    res.exitCode = pclose(pipe);
#endif
    res.success = (res.exitCode == 0);
    if (!res.success) {
        auto errs = res.Errors();
        if (!errs.empty()) res.errorSummary = errs.front();
        else               res.errorSummary = "Build failed (exit " + std::to_string(res.exitCode) + ")";
    }
    return res;
}

// ──────────────────────────────────────────────────────────────
// Command-line builders
// ──────────────────────────────────────────────────────────────

std::string BuildAutomation::ConfigureCommandLine(const BuildConfig& cfg) {
    std::ostringstream cmd;
    cmd << "cmake -S " << cfg.projectRoot
        << " -B " << cfg.buildDir
        << " -G \"" << cfg.generator << "\""
        << " -DCMAKE_BUILD_TYPE=" << BuildTypeName(cfg.type);

    std::string tc = PlatformToolchain(cfg.platform);
    if (!tc.empty()) cmd << " -DCMAKE_TOOLCHAIN_FILE=" << tc;

    for (const auto& arg : cfg.extraCMakeArgs)
        cmd << " " << arg;

    return cmd.str();
}

std::string BuildAutomation::BuildCommandLine(const BuildConfig& cfg) {
    std::ostringstream cmd;
    cmd << "cmake --build " << cfg.buildDir
        << " --config " << BuildTypeName(cfg.type);
    if (cfg.jobs > 0) cmd << " --parallel " << cfg.jobs;
    else              cmd << " --parallel";
    return cmd.str();
}

// ──────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────

BuildResult BuildAutomation::Configure(const BuildConfig& cfg) {
    std::filesystem::create_directories(cfg.buildDir);
    return RunCommand(ConfigureCommandLine(cfg));
}

BuildResult BuildAutomation::Build(const BuildConfig& cfg) {
    return RunCommand(BuildCommandLine(cfg));
}

BuildResult BuildAutomation::ConfigureAndBuild(const BuildConfig& cfg) {
    BuildResult cfg_res = Configure(cfg);
    if (!cfg_res.success) return cfg_res;
    BuildResult bld_res = Build(cfg);
    bld_res.output = cfg_res.output + bld_res.output;
    return bld_res;
}

BuildResult BuildAutomation::RunTests(const BuildConfig& cfg,
                                       const std::string& testFilter) {
    std::ostringstream cmd;
    cmd << "ctest --test-dir " << cfg.buildDir << " -C " << BuildTypeName(cfg.type);
    if (!testFilter.empty()) cmd << " -R " << testFilter;
    return RunCommand(cmd.str());
}

BuildResult BuildAutomation::Clean(const BuildConfig& cfg) {
    std::ostringstream cmd;
    cmd << "cmake --build " << cfg.buildDir << " --target clean";
    return RunCommand(cmd.str());
}

BuildResult BuildAutomation::BuildWithOutput(const BuildConfig& cfg, OutputFn onLine) {
    return RunCommand(BuildCommandLine(cfg), std::move(onLine));
}

} // namespace Tools
