#pragma once
#include <string>
#include <functional>

namespace IDE::BuildSystem {

// ──────────────────────────────────────────────────────────────────────────
// CygwinManager
//
// Detects and manages a portable Cygwin64 environment for use by the IDE
// build system on Windows.  Allows the editor to run Unix shell scripts
// (build_all.sh, etc.) without requiring the user to pre-install a system-
// wide Cygwin, MSYS2, or WSL.
//
// Detection priority (Windows only):
//   1. Embedded install  — <working-dir>/Tools/Cygwin64/bin/bash.exe
//   2. System install    — C:/cygwin64, C:/cygwin, D:/cygwin64 (common paths)
//   3. PATH probe        — looks for "bash.exe" containing "cygwin" in path
//
// On Linux/macOS the class is a no-op (Status::NotFound); callers fall
// through to native bash.
//
// Usage:
//   auto& cygwin = IDE::BuildSystem::CygwinManager::Instance();
//   if (cygwin.Detect() != Status::NotFound) {
//       std::string cmd = cygwin.MakeShellCommand(
//           "Scripts/Tools/build_all.sh --debug-only",
//           "Logs/Build/editor_build.log");
//       std::system(cmd.c_str());
//   }
// ──────────────────────────────────────────────────────────────────────────
class CygwinManager {
public:
    enum class Status {
        NotFound,       ///< No Cygwin detected on this machine
        Embedded,       ///< Found in Tools/Cygwin64 (portable, preferred)
        SystemInstall,  ///< Found at a system-wide install path
    };

    // Singleton
    static CygwinManager& Instance();

    // ── Detection ────────────────────────────────────────────────────────

    /// Probe for Cygwin.  Result is cached after the first call.
    Status Detect();

    /// Cached status (does not re-probe).
    Status GetStatus() const { return m_status; }

    /// Absolute path to bash.exe, or empty string if not found.
    const std::string& BashPath() const { return m_bashPath; }

    /// Absolute path to the Cygwin root dir, or empty if not found.
    const std::string& RootDir() const { return m_rootDir; }

    // ── Command building ─────────────────────────────────────────────────

    /// Build a complete command string that runs bashCmd through the
    /// detected Cygwin bash.  PATH is prepended with Cygwin bin dirs to
    /// ensure isolation from system tools.  stdout+stderr are appended to
    /// logFile.  Safe to pass directly to std::system().
    std::string MakeShellCommand(const std::string& bashCmd,
                                 const std::string& logFile) const;

    // ── Path translation ─────────────────────────────────────────────────

    /// "C:\\foo\\bar" → "/cygdrive/c/foo/bar"
    static std::string ToCygwinPath(const std::string& winPath);

    /// "/cygdrive/c/foo/bar" → "C:/foo/bar"
    static std::string ToWindowsPath(const std::string& cygPath);

    // ── Embedded setup ───────────────────────────────────────────────────

    /// Relative path of the embedded Cygwin directory used by the editor.
    static std::string EmbeddedDir();

    /// Write a Windows batch script (Tools/setup_cygwin_portable.cmd) that
    /// downloads and installs a minimal Cygwin64 into EmbeddedDir().
    /// progress() is called with status lines; returns true on success.
    /// On non-Windows, or if Cygwin is already present, returns false.
    bool SetupEmbedded(const std::function<void(const std::string&)>& progress);

private:
    CygwinManager() = default;
    CygwinManager(const CygwinManager&) = delete;
    CygwinManager& operator=(const CygwinManager&) = delete;

    Status      m_status   = Status::NotFound;
    std::string m_bashPath;
    std::string m_rootDir;
};

} // namespace IDE::BuildSystem
