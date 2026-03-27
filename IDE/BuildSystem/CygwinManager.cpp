#include "IDE/BuildSystem/CygwinManager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <windows.h>
#  undef DrawText
#  undef SendMessage
#endif

namespace fs = std::filesystem;

namespace IDE::BuildSystem {

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
CygwinManager& CygwinManager::Instance() {
    static CygwinManager inst;
    return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
// EmbeddedDir — relative to the editor's working directory
// ─────────────────────────────────────────────────────────────────────────────
std::string CygwinManager::EmbeddedDir() {
    return "Tools/Cygwin64";
}

// ─────────────────────────────────────────────────────────────────────────────
// ToCygwinPath  "C:\foo\bar" → "/cygdrive/c/foo/bar"
// ─────────────────────────────────────────────────────────────────────────────
std::string CygwinManager::ToCygwinPath(const std::string& winPath) {
    if (winPath.size() >= 2 && winPath[1] == ':') {
        char drive = static_cast<char>(std::tolower(static_cast<unsigned char>(winPath[0])));
        std::string rest = winPath.substr(2);
        std::replace(rest.begin(), rest.end(), '\\', '/');
        return std::string("/cygdrive/") + drive + rest;
    }
    // Already POSIX-style — just normalise back-slashes
    std::string out = winPath;
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// ToWindowsPath  "/cygdrive/c/foo/bar" → "C:/foo/bar"
// ─────────────────────────────────────────────────────────────────────────────
std::string CygwinManager::ToWindowsPath(const std::string& cygPath) {
    // Must be at least "/cygdrive/X" (11 chars)
    if (cygPath.rfind("/cygdrive/", 0) == 0 && cygPath.size() >= 11) {
        char drive = static_cast<char>(std::toupper(static_cast<unsigned char>(cygPath[10])));
        std::string rest = cygPath.substr(11); // everything after "/cygdrive/X"
        return std::string(1, drive) + ":" + rest;
    }
    return cygPath;
}

// ─────────────────────────────────────────────────────────────────────────────
// Detect — probe for Cygwin; result cached after first call
// ─────────────────────────────────────────────────────────────────────────────
CygwinManager::Status CygwinManager::Detect() {
    if (m_status != Status::NotFound) return m_status; // cached

#ifndef _WIN32
    // On Linux/macOS native bash is used directly — no Cygwin needed.
    return m_status; // stays NotFound
#else
    // 1. Embedded (portable, highest priority)
    {
        fs::path embedded = fs::path(EmbeddedDir()) / "bin" / "bash.exe";
        if (fs::exists(embedded)) {
            m_bashPath = embedded.string();
            m_rootDir  = fs::path(EmbeddedDir()).string();
            m_status   = Status::Embedded;
            return m_status;
        }
    }

    // 2. Well-known system install paths
    static const char* kSystemPaths[] = {
        "C:/cygwin64/bin/bash.exe",
        "C:/cygwin/bin/bash.exe",
        "D:/cygwin64/bin/bash.exe",
        "D:/cygwin/bin/bash.exe",
        nullptr
    };
    for (int i = 0; kSystemPaths[i]; ++i) {
        fs::path p = kSystemPaths[i];
        if (fs::exists(p)) {
            m_bashPath = p.string();
            m_rootDir  = p.parent_path().parent_path().string();
            m_status   = Status::SystemInstall;
            return m_status;
        }
    }

    // 3. PATH search — accept only if the resolved path contains "cygwin"
    {
        CHAR buf[MAX_PATH] = {};
        DWORD len = SearchPathA(nullptr, "bash.exe", nullptr, MAX_PATH, buf, nullptr);
        if (len > 0 && len < MAX_PATH) {
            std::string found = buf;
            std::string lower = found;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower.find("cygwin") != std::string::npos) {
                m_bashPath = found;
                m_rootDir  = fs::path(found).parent_path().parent_path().string();
                m_status   = Status::SystemInstall;
                return m_status;
            }
        }
    }

    return m_status; // NotFound
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// MakeShellCommand — build an isolated bash command string for std::system()
// ─────────────────────────────────────────────────────────────────────────────
std::string CygwinManager::MakeShellCommand(const std::string& bashCmd,
                                             const std::string& logFile) const {
#ifndef _WIN32
    // Unix: use system bash directly; run in background
    return "bash -c '" + bashCmd + "' >> " + logFile + " 2>&1 &";
#else
    if (m_bashPath.empty()) {
        // No Cygwin — fall back to Git Bash / MSYS2 if present on PATH
        return "bash -c \"" + bashCmd + "\" >> " + logFile + " 2>&1";
    }

    // Build Cygwin-internal PATH prepend (Cygwin format) for isolation
    std::string binCyg  = ToCygwinPath(fs::path(m_rootDir).append("bin").string());
    std::string usrBin  = ToCygwinPath(fs::path(m_rootDir).append("usr/bin").string());

    // The exported PATH prepends Cygwin dirs; the rest of $PATH is still
    // inherited so Windows tools (cmake, etc.) remain reachable.
    std::string isolate = "export PATH=\"" + binCyg + ":" + usrBin + ":$PATH\"; ";

    // Normalise bash path to back-slashes for cmd.exe
    std::string bash = m_bashPath;
    std::replace(bash.begin(), bash.end(), '/', '\\');

    // Final command: "C:\cygwin64\bin\bash.exe" -c "export PATH=...; <cmd>" >> log 2>&1
    return "\"" + bash + "\" -c \"" + isolate + bashCmd + "\" >> " + logFile + " 2>&1";
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupEmbedded — write a portable setup script and prompt the user
// ─────────────────────────────────────────────────────────────────────────────
bool CygwinManager::SetupEmbedded(const std::function<void(const std::string&)>& progress) {
#ifndef _WIN32
    progress("Cygwin setup is only applicable on Windows.");
    return false;
#else
    // Already present?
    fs::path bashExe = fs::path(EmbeddedDir()) / "bin" / "bash.exe";
    if (fs::exists(bashExe)) {
        progress("Embedded Cygwin already present at: " + EmbeddedDir());
        return true;
    }

    progress("Preparing embedded Cygwin64 at: " + EmbeddedDir());

    // Ensure the Tools/ directory exists
    std::error_code ec;
    fs::create_directories("Tools", ec);
    if (ec) {
        progress("Error: could not create Tools/ directory: " + ec.message());
        return false;
    }

    // Write setup_cygwin_portable.cmd next to the editor
    static const char* kScriptPath = "Tools/setup_cygwin_portable.cmd";
    progress("Writing: " + std::string(kScriptPath));

    std::ofstream f(kScriptPath);
    if (!f) {
        progress("Error: could not write " + std::string(kScriptPath));
        return false;
    }

    f << "@echo off\r\n"
         "setlocal enabledelayedexpansion\r\n"
         "\r\n"
         ":: Atlas Editor — Portable Cygwin64 Setup\r\n"
         ":: Installs a minimal Cygwin64 into Tools\\Cygwin64 so that the editor\r\n"
         ":: can run bash build scripts without a system-wide Cygwin installation.\r\n"
         "::\r\n"
         ":: Packages: bash make gcc-core gcc-g++ binutils coreutils grep sed gawk findutils\r\n"
         "\r\n"
         "set CYGWIN_ROOT=%~dp0Cygwin64\r\n"
         "set SETUP_EXE=%~dp0cygwin-setup-x86_64.exe\r\n"
         "set PACKAGES=bash,make,gcc-core,gcc-g++,binutils,coreutils,grep,sed,gawk,findutils\r\n"
         "set MIRROR=https://mirrors.kernel.org/sourceware/cygwin\r\n"
         "set PKG_CACHE=%~dp0cygwin-packages\r\n"
         "\r\n"
         "echo [Atlas] Downloading Cygwin setup-x86_64.exe ...\r\n"
         "powershell -NoProfile -Command \"\r\n"
         "  [Net.ServicePointManager]::SecurityProtocol = 'Tls12';\r\n"
         "  Invoke-WebRequest -Uri 'https://www.cygwin.com/setup-x86_64.exe'\r\n"
         "    -OutFile '%SETUP_EXE%' -UseBasicParsing\"\r\n"
         "\r\n"
         "if not exist \"%SETUP_EXE%\" (\r\n"
         "    echo [Error] Failed to download setup-x86_64.exe — check internet access.\r\n"
         "    exit /b 1\r\n"
         ")\r\n"
         "\r\n"
         "echo [Atlas] Installing packages: %PACKAGES%\r\n"
         "\"%SETUP_EXE%\" ^^\r\n"
         "    --quiet-mode ^^\r\n"
         "    --no-admin ^^\r\n"
         "    --no-shortcuts ^^\r\n"
         "    --no-startmenu ^^\r\n"
         "    --root \"%CYGWIN_ROOT%\" ^^\r\n"
         "    --local-package-dir \"%PKG_CACHE%\" ^^\r\n"
         "    --packages \"%PACKAGES%\" ^^\r\n"
         "    --site \"%MIRROR%\"\r\n"
         "\r\n"
         "if exist \"%CYGWIN_ROOT%\\bin\\bash.exe\" (\r\n"
         "    echo [Atlas] SUCCESS — Cygwin64 installed at %CYGWIN_ROOT%\r\n"
         "    echo [Atlas] Restart AtlasEditor to use embedded Cygwin for builds.\r\n"
         ") else (\r\n"
         "    echo [Error] Installation may have failed — bash.exe not found.\r\n"
         "    exit /b 1\r\n"
         ")\r\n"
         "\r\n"
         ":: Clean up the downloaded installer to save space\r\n"
         "del /q \"%SETUP_EXE%\" 2>nul\r\n"
         "\r\n"
         "endlocal\r\n";

    f.close();
    if (!f) {
        progress("Error: write failed for " + std::string(kScriptPath));
        return false;
    }

    progress("Setup script written to: " + std::string(kScriptPath));
    progress("Run Tools\\setup_cygwin_portable.cmd (no admin required) to install.");
    progress("After installation, restart AtlasEditor to detect embedded Cygwin.");
    return true;
#endif
}

} // namespace IDE::BuildSystem
