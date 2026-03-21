#!/usr/bin/env bash
# =============================================================================
# detect_compiler.sh — Cross-platform C++ compiler detection helper
#
# Source this file; it exports:
#   CXX_FOUND     = true | false
#   CXX_NAME      = human-readable compiler string (for logging)
#   CXX           = path / command to invoke the compiler (exported)
#
# On Windows (Git Bash / MSYS2 / Cygwin) the script:
#   1. Checks whether any compiler is already in PATH (CXX envvar first).
#   2. If not, locates Visual Studio via vswhere.exe and activates
#      vcvarsall.bat so that cl.exe and the MSVC linker enter PATH.
#   3. Checks common MinGW / LLVM toolchains installed under MSYS2 or
#      the official LLVM installer.
#
# Usage:
#   source "$(dirname "${BASH_SOURCE[0]}")/detect_compiler.sh"
#   if [[ "${CXX_FOUND}" != "true" ]]; then
#       echo "No compiler — aborting"; exit 1
#   fi
# =============================================================================

CXX_FOUND=false
CXX_NAME=""

# ── Helper: try a single candidate ───────────────────────────────────────────
_try_cxx() {
    local cmd="$1"
    if command -v "${cmd}" &>/dev/null; then
        local ver
        ver="$("${cmd}" --version 2>&1 | head -1)" || ver="(version unknown)"
        CXX_FOUND=true
        CXX_NAME="${cmd}: ${ver}"
        export CXX="${cmd}"
        return 0
    fi
    return 1
}

# ── Helper: try MSVC cl.exe ───────────────────────────────────────────────────
_try_cl() {
    # cl.exe prints version to stderr
    if command -v cl &>/dev/null; then
        local ver
        ver="$(cl 2>&1 | head -1)" || ver="(version unknown)"
        CXX_FOUND=true
        CXX_NAME="cl (MSVC): ${ver}"
        export CXX="cl"
        return 0
    fi
    return 1
}

# ── Step 1: honour $CXX if already set ───────────────────────────────────────
if [[ -n "${CXX:-}" ]] && command -v "${CXX}" &>/dev/null; then
    ver="$("${CXX}" --version 2>&1 | head -1)"
    CXX_FOUND=true
    CXX_NAME="${CXX}: ${ver}"
fi

# ── Step 2: probe common names in PATH ────────────────────────────────────────
if [[ "${CXX_FOUND}" == "false" ]]; then
    _try_cxx g++     || \
    _try_cxx clang++ || \
    _try_cxx c++     || \
    _try_cxx gcc     || \
    _try_cxx cc      || \
    _try_cl          || \
    true
fi

# ── Step 3: Windows — vswhere + vcvarsall ─────────────────────────────────────
# Runs only when: still not found AND we appear to be on Windows
# (WINDIR is set in Git Bash / MSYS2 / Cygwin).
if [[ "${CXX_FOUND}" == "false" && -n "${WINDIR:-}" ]]; then

    # Locate vswhere.exe — it ships with VS 2017+ and Build Tools
    VSWHERE_PATHS=(
        "${PROGRAMFILES(X86):-C:/Program Files (x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
        "${PROGRAMFILES:-C:/Program Files}/Microsoft Visual Studio/Installer/vswhere.exe"
    )
    VSWHERE=""
    for p in "${VSWHERE_PATHS[@]}"; do
        # Convert Windows path to Bash path if needed
        local_p="$(cygpath -u "${p}" 2>/dev/null || echo "${p}")"
        if [[ -x "${local_p}" ]]; then
            VSWHERE="${local_p}"
            break
        fi
    done

    if [[ -n "${VSWHERE}" ]]; then
        # Find the latest VS installation that includes the VC tools
        VS_INSTALL="$("${VSWHERE}" -latest -products '*' \
                       -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
                       -property installationPath 2>/dev/null | tr -d '\r')"

        if [[ -n "${VS_INSTALL}" ]]; then
            # Convert to a Unix path for bash
            VS_INSTALL_UNIX="$(cygpath -u "${VS_INSTALL}" 2>/dev/null || echo "${VS_INSTALL}")"
            VCVARSALL="${VS_INSTALL_UNIX}/VC/Auxiliary/Build/vcvarsall.bat"

            if [[ -f "${VCVARSALL}" ]]; then
                # Run vcvarsall.bat and capture the resulting environment.
                # We use cmd.exe to source it, then print all env vars, and
                # merge the differences into the current bash session.
                VCVARS_WIN="$(cygpath -w "${VCVARSALL}" 2>/dev/null || echo "${VCVARSALL}")"
                VCVARS_ENV="$(cmd.exe //c "\"${VCVARS_WIN}\" x64 > NUL 2>&1 && set" 2>/dev/null)"

                if [[ -n "${VCVARS_ENV}" ]]; then
                    # Export every variable emitted by vcvarsall
                    while IFS='=' read -r key val; do
                        key="${key%%$'\r'}"
                        val="${val%%$'\r'}"
                        # Avoid clobbering critical bash/MSYS vars
                        case "${key}" in
                            PROMPT|PS1|SHLVL|_|PWD|OLDPWD|HOME|TERM|SHELL) continue ;;
                        esac
                        export "${key}=${val}" 2>/dev/null || true
                    done <<< "${VCVARS_ENV}"

                    # Convert the new PATH entries to MSYS-style and prepend
                    WIN_PATH="${PATH}"
                    MSYS_PATHS=""
                    IFS=';' read -ra WIN_PARTS <<< "$(cmd.exe //c "\"${VCVARS_WIN}\" x64 > NUL 2>&1 && echo %PATH%" 2>/dev/null | tr -d '\r')"
                    for wp in "${WIN_PARTS[@]}"; do
                        [[ -z "${wp}" ]] && continue
                        mp="$(cygpath -u "${wp}" 2>/dev/null)" || continue
                        MSYS_PATHS="${mp}:${MSYS_PATHS}"
                    done
                    export PATH="${MSYS_PATHS}${PATH}"

                    # Now cl should be findable
                    _try_cl || true
                fi

                if [[ "${CXX_FOUND}" == "true" ]]; then
                    CXX_NAME="${CXX_NAME} (activated via vcvarsall from ${VS_INSTALL})"
                fi
            fi
        fi
    fi

    # ── Step 3b: MSYS2 MinGW toolchain ────────────────────────────────────────
    if [[ "${CXX_FOUND}" == "false" ]]; then
        for mingw_path in \
            "/mingw64/bin" "/ucrt64/bin" "/clang64/bin" \
            "/c/msys64/mingw64/bin" "/c/msys64/ucrt64/bin"; do
            if [[ -x "${mingw_path}/g++.exe" ]]; then
                export PATH="${mingw_path}:${PATH}"
                _try_cxx g++ && break
            elif [[ -x "${mingw_path}/clang++.exe" ]]; then
                export PATH="${mingw_path}:${PATH}"
                _try_cxx clang++ && break
            fi
        done
    fi

    # ── Step 3c: LLVM official installer ──────────────────────────────────────
    if [[ "${CXX_FOUND}" == "false" ]]; then
        for llvm_path in \
            "/c/Program Files/LLVM/bin" \
            "/c/Program Files (x86)/LLVM/bin"; do
            if [[ -x "${llvm_path}/clang++.exe" ]]; then
                export PATH="${llvm_path}:${PATH}"
                _try_cxx clang++ && break
            fi
        done
    fi
fi

# ── Step 4: Final verdict ─────────────────────────────────────────────────────
if [[ "${CXX_FOUND}" == "true" ]]; then
    : # caller will print the success message
else
    # Print a platform-specific help message
    if [[ -n "${WINDIR:-}" ]]; then
        cat >&2 <<'WINHELP'

  [ERROR] No C++ compiler found on Windows.

  Fix options (choose one):

    A) Run this script from a Visual Studio Developer Command Prompt:
         Start → "x64 Native Tools Command Prompt for VS 2022"
         Then re-run: bash Scripts/Tools/ci_pipeline.sh

    B) Run from a Visual Studio Developer PowerShell:
         Start → "Developer PowerShell for VS 2022"
         bash Scripts/Tools/ci_pipeline.sh

    C) Install Build Tools for Visual Studio 2022 (free, ~3 GB):
         https://aka.ms/vs/17/release/vs_BuildTools.exe
         Select: "Desktop development with C++"
         Then open an "x64 Native Tools Command Prompt" and re-run.

    D) Install MSYS2 + MinGW-w64 (lighter-weight, gcc-based):
         https://www.msys2.org
         In MSYS2 terminal: pacman -S mingw-w64-ucrt-x86_64-gcc
         Use the "UCRT64" MSYS2 terminal and re-run.

WINHELP
    else
        cat >&2 <<'LINHELP'

  [ERROR] No C++ compiler found.

  Fix:
    Ubuntu/Debian:  sudo apt-get install -y g++ build-essential
    Fedora/RHEL:    sudo dnf install -y gcc-c++
    macOS:          xcode-select --install   (or: brew install llvm)

LINHELP
    fi
fi
