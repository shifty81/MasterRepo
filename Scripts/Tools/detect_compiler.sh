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

# ── Helper: extract the first non-empty line from a multi-line string ─────────
# Uses only bash built-ins — no subprocess pipe that could hang on Windows.
# Strips LF first, then CR, which correctly handles Unix (LF), Windows (CRLF)
# and old-Mac (CR) line endings:
#   CRLF "line1\r\nline2"  → after %%$'\n'* → "line1\r" → after %%$'\r'* → "line1"
#   LF   "line1\nline2"    → after %%$'\n'* → "line1"   → after %%$'\r'* → "line1"
#   CR   "line1\rline2"    → after %%$'\n'* → unchanged  → after %%$'\r'* → "line1"
_first_line_of() {
    local _s="$1"
    _s="${_s%%$'\n'*}"   # strip everything from first LF (handles LF and CRLF)
    _s="${_s%%$'\r'*}"   # strip remaining CR (handles trailing CR from CRLF or CR-only)
    printf '%s' "${_s}"
}

# ── Helper: try a single candidate ───────────────────────────────────────────
_try_cxx() {
    local cmd="$1"
    if command -v "${cmd}" &>/dev/null; then
        local ver _out
        # Capture ALL output first; do NOT pipe directly to 'head -1'.
        # On Windows, SIGPIPE is not reliably delivered to native processes,
        # so the subprocess can hang indefinitely on a broken pipe.
        # Use _first_line_of (bash parameter expansion) instead.
        _out="$("${cmd}" --version 2>&1 || true)"
        ver="$(_first_line_of "${_out}")"
        [[ -z "${ver}" ]] && ver="(version unknown)"
        CXX_FOUND=true
        CXX_NAME="${cmd}: ${ver}"
        export CXX="${cmd}"
        return 0
    fi
    return 1
}

# ── Helper: try MSVC cl.exe ───────────────────────────────────────────────────
_try_cl() {
    if command -v cl &>/dev/null; then
        # Capture ALL cmd.exe output first; do NOT pipe directly to 'head -1'.
        # On Windows, SIGPIPE is not reliably delivered to native processes
        # (cmd.exe, cl.exe), so 'head -1' exiting can leave the writer hanging
        # indefinitely on a broken pipe.  Extract the first line in pure bash.
        local ver _out
        _out="$(cmd.exe /c "cl 2>&1" 2>/dev/null || true)"
        ver="$(_first_line_of "${_out}")"
        [[ -z "${ver}" ]] && ver="(version unknown)"
        CXX_FOUND=true
        CXX_NAME="cl (MSVC): ${ver}"
        export CXX="cl"
        return 0
    fi
    return 1
}

# ── Step 1: honour $CXX if already set ───────────────────────────────────────
if [[ -n "${CXX:-}" ]] && command -v "${CXX}" &>/dev/null; then
    _cxx1_out="$("${CXX}" --version 2>&1 || true)"
    ver="$(_first_line_of "${_cxx1_out}")"
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

    # ${PROGRAMFILES(X86)} contains parentheses, which are not valid in bash
    # variable names.  Read the value via cmd.exe and fall back to the
    # well-known default path when cmd.exe is unavailable.
    _pfiles_x86="$(cmd.exe /c "echo %PROGRAMFILES(X86)%" 2>/dev/null | tr -d '\r')"
    if [[ -z "${_pfiles_x86}" || "${_pfiles_x86}" == "%PROGRAMFILES(X86)%" ]]; then
        _pfiles_x86="${PROGRAMFILES:-C:/Program Files} (x86)"
    fi

    # Locate vswhere.exe — it ships with VS 2017+ and Build Tools
    VSWHERE_PATHS=(
        "${_pfiles_x86}/Microsoft Visual Studio/Installer/vswhere.exe"
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
                # Run vcvarsall.bat ONCE and capture the resulting environment.
                # We use cmd.exe to source it, then print all env vars, and
                # merge the differences into the current bash session.
                # Guard against Windows' built-in timeout.exe (System32) which
                # silently ignores the COMMAND argument and just sleeps — it
                # would cause an indefinite hang here.  Detect GNU coreutils
                # timeout via a functional test: GNU exits 124 when the timer
                # fires; Windows' built-in never runs the command and exits 0.
                VCVARS_WIN="$(cygpath -w "${VCVARSALL}" 2>/dev/null || echo "${VCVARSALL}")"
                _vcvars_timeout=""
                if command -v timeout &>/dev/null; then
                    # Functional test: GNU coreutils timeout wraps COMMAND and
                    # exits 124 when the timer fires.  Windows' built-in
                    # timeout.exe ignores the COMMAND argument (never runs it)
                    # and exits 0.  Note: duration=0 disables GNU timeout, so
                    # use 0.1s; sleep 1 caps worst-case delay to ~1s if the
                    # test unexpectedly does not terminate quickly.
                    timeout 0.1 sleep 1 2>/dev/null; _to_ec=$?
                    [[ "${_to_ec}" -eq 124 ]] && _vcvars_timeout="timeout 60"
                    unset _to_ec
                fi
                # shellcheck disable=SC2086  # intentional word-splitting of _vcvars_timeout
                VCVARS_ENV="$(${_vcvars_timeout} cmd.exe //c \
                    "\"${VCVARS_WIN}\" x64 > NUL 2>&1 && set" \
                    2>/dev/null || true)"
                unset _vcvars_timeout

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

                    # Extract the new PATH from the already-captured output
                    # (avoids a second, expensive cmd.exe //c vcvarsall call).
                    local win_path_val
                    win_path_val="$(grep -i '^PATH=' <<< "${VCVARS_ENV}" \
                                    | head -1 | cut -d'=' -f2- | tr -d '\r')"
                    if [[ -n "${win_path_val}" ]]; then
                        MSYS_PATHS=""
                        IFS=';' read -ra WIN_PARTS <<< "${win_path_val}"
                        for wp in "${WIN_PARTS[@]}"; do
                            [[ -z "${wp}" ]] && continue
                            mp="$(cygpath -u "${wp}" 2>/dev/null)" || continue
                            MSYS_PATHS="${mp}:${MSYS_PATHS}"
                        done
                        export PATH="${MSYS_PATHS}${PATH}"
                    fi

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
