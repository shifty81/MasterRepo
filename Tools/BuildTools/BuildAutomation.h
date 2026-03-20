#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace Tools {

// ──────────────────────────────────────────────────────────────
// Build configuration
// ──────────────────────────────────────────────────────────────

enum class BuildType { Debug, Release, RelWithDebInfo, MinSizeRel };
enum class BuildPlatform { Native, Linux, Windows, MacOS, Web };

struct BuildConfig {
    std::string   projectRoot;
    std::string   buildDir      = "build";
    BuildType     type          = BuildType::Debug;
    BuildPlatform platform      = BuildPlatform::Native;
    std::string   generator     = "Ninja";  // or "Unix Makefiles", "Visual Studio 17"
    std::vector<std::string> extraCMakeArgs;
    int32_t       jobs          = 0;   // 0 = auto
    bool          clean         = false;
    bool          runTests      = false;
};

// ──────────────────────────────────────────────────────────────
// Build result
// ──────────────────────────────────────────────────────────────

struct BuildResult {
    bool        success   = false;
    int32_t     exitCode  = 0;
    std::string output;   // combined stdout/stderr
    std::string errorSummary;

    // Parse compiler error lines from output
    std::vector<std::string> Errors()   const;
    std::vector<std::string> Warnings() const;
};

// ──────────────────────────────────────────────────────────────
// BuildAutomation
// ──────────────────────────────────────────────────────────────

class BuildAutomation {
public:
    // Configure (cmake -S . -B build ...)
    static BuildResult Configure(const BuildConfig& cfg);

    // Build (cmake --build ...)
    static BuildResult Build(const BuildConfig& cfg);

    // Full configure + build
    static BuildResult ConfigureAndBuild(const BuildConfig& cfg);

    // Run ctest in the build dir
    static BuildResult RunTests(const BuildConfig& cfg,
                                 const std::string& testFilter = "");

    // Clean the build directory
    static BuildResult Clean(const BuildConfig& cfg);

    // Stream output line by line
    using OutputFn = std::function<void(const std::string& line)>;
    static BuildResult BuildWithOutput(const BuildConfig& cfg, OutputFn onLine);

    // Utility: build type as CMake string
    static std::string BuildTypeName(BuildType t);
    static std::string PlatformToolchain(BuildPlatform p);

private:
    static BuildResult RunCommand(const std::string& cmd, OutputFn onLine = nullptr);
    static std::string BuildCommandLine(const BuildConfig& cfg);
    static std::string ConfigureCommandLine(const BuildConfig& cfg);
};

} // namespace Tools
