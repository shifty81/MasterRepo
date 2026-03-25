#!/usr/bin/env bash
# =============================================================================
# MasterRepo — build.sh   (Windows / Git Bash / MSYS2)
#
# Automates CMake configure + build for Debug and Release configurations.
#
# ── Hang vectors fixed in this script ────────────────────────────────────────
#
#   1. cmake piped through 'tee' (original design):
#        MSBuild spawns persistent worker-node processes that inherit any pipe
#        write-handle.  When cmake exits the nodes are still alive, keeping
#        the pipe open.  The downstream 'tee' never sees EOF → infinite hang.
#        Fix: cmake output redirected directly to the log file (no pipe).
#        cmake runs as a background job; a kill-0 polling loop waits for it.
#
#   2. 'tail -f log &' + 'kill tail' for streaming (previous fix attempt):
#        On Windows, SIGTERM is not reliably delivered to Git Bash background
#        processes; 'wait "${tail_pid}"' could hang indefinitely.
#        Fix: No tail/streaming process — kill-0 polling only.
#
#   3. 'read -r -p ""' in _wait_for_user without a terminal check:
#        In CI, scheduled tasks, or when piped, stdin is not a terminal.
#        'read' with no timeout blocks forever.
#        Fix: _is_interactive() checks [[ -t 0 && -t 1 ]] and CI env var.
#        _wait_for_user returns immediately when not interactive.
#
# Usage:
#   build.sh debug     — Debug configure + build
#   build.sh release   — Release configure + build
#   build.sh all       — Both Debug and Release
#   build.sh clean     — Remove build directories
#   build.sh --help
#
# Options (appended after the command):
#   --no-wait   Skip the "Press [Enter]" pause even in interactive mode.
#
# Exit codes: 0=success  1=build failure
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DEBUG="${REPO_ROOT}/Builds/debug"
BUILD_RELEASE="${REPO_ROOT}/Builds/release"
LOG_DIR="${REPO_ROOT}/Logs/Build"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
JOBS="${NUMBER_OF_PROCESSORS:-4}"
BUILD_START="${SECONDS}"
NO_WAIT=false

# ── Source shared libraries ───────────────────────────────────────────────────
# shellcheck source=Scripts/Tools/lib_log.sh
source "${SCRIPT_DIR}/lib_log.sh"
log_init "build"
# shellcheck source=Scripts/Tools/lib_watchdog.sh
source "${SCRIPT_DIR}/lib_watchdog.sh"

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; BOLD='\033[1m'; NC='\033[0m'

info()  { echo -e "${GREEN}[build]${NC} $*"; }
warn()  { echo -e "${YELLOW}[warn]${NC}  $*"; }
error() { echo -e "${RED}[error]${NC} $*" >&2; }

# ── Interactive detection ─────────────────────────────────────────────────────
# Returns 0 (true) only when:
#   • Both stdin AND stdout are real terminals, AND
#   • The CI environment variable is not "true", AND
#   • TERM is not "dumb" (typical in non-interactive pipes), AND
#   • --no-wait was not passed.
_is_interactive() {
    [[ "${NO_WAIT}"   == "true"  ]] && return 1
    [[ "${CI:-}"      == "true"  ]] && return 1
    [[ "${TERM:-}"    == "dumb"  ]] && return 1
    [[ -t 0 && -t 1 ]]
}

_wait_for_user() {
    _is_interactive || return 0
    printf "\n${BOLD}Press [Enter] to close...${NC}\n"
    read -r -p "" 2>/dev/null || true
}

# ── Status tracking ────────────────────────────────────────────────────────────
_PASS=()
_FAIL=()

_track() {
    local label="$1"; shift
    local rc=0
    set +e; "$@"; rc=$?; set -e
    if [[ ${rc} -eq 0 ]]; then _PASS+=("${label}")
    else                        _FAIL+=("${label}"); fi
    return ${rc}
}

# ── Print summary ─────────────────────────────────────────────────────────────
_print_summary() {
    local elapsed=$(( SECONDS - BUILD_START ))
    echo ""
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo -e "${BOLD}  MasterRepo Build — ${elapsed}s${NC}"
    for c in "${_PASS[@]:-}"; do [[ -n "${c}" ]] && echo -e "  ${GREEN}✅ ${c}${NC}"; done
    for c in "${_FAIL[@]:-}"; do [[ -n "${c}" ]] && echo -e "  ${RED}❌ ${c}${NC}"; done
    if [[ ${#_PASS[@]} -eq 0 && ${#_FAIL[@]} -eq 0 ]]; then
        echo -e "  (no build steps ran)"
    fi
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo -e "  Log dir:  ${LOG_DIR}"
    echo -e "  Log file: $(log_file)"
    if [[ ${#_FAIL[@]} -gt 0 ]]; then
        echo ""
        echo -e "  ${YELLOW}To submit a bug report:${NC}"
        echo -e "    bash Scripts/Tools/submit_issue.sh --log \"$(log_file)\""
    fi
    echo ""
    log_finish
}

mkdir -p "${LOG_DIR}"

# ── C++ compiler detection ────────────────────────────────────────────────────
# shellcheck source=Scripts/Tools/detect_compiler.sh
source "${SCRIPT_DIR}/detect_compiler.sh" || true
if [[ "${CXX_FOUND}" == "true" ]]; then
    info "Compiler: ${CXX_NAME}"
else
    warn "No C++ compiler found — CMake will attempt its own detection."
fi

# ── cmake runner ──────────────────────────────────────────────────────────────
# Runs cmake as a background process writing directly to a log file.
# No pipe is created, so MSBuild worker-node processes cannot keep a pipe
# write-handle open after cmake exits (the original hang root cause).
# The kill-0 polling loop detects when cmake has exited without relying on
# any secondary process (tail, tee) that might not terminate reliably.
_cmake_run() {
    local log_file="$1"; shift
    : >> "${log_file}"

    "$@" >> "${log_file}" 2>&1 &
    local _pid=$!
    local _rc=0

    watchdog_start "${log_file}" "${WATCHDOG_TIMEOUT:-600}" "${_pid}"

    while kill -0 "${_pid}" 2>/dev/null; do
        sleep 0.5 2>/dev/null || true
    done
    wait "${_pid}" || _rc=$?

    watchdog_stop
    return "${_rc}"
}

# ── cmake --build with /nodeReuse:false for MSBuild generators ────────────────
_cmake_build() {
    local build_dir="$1" config="$2" log_file="$3"
    # If a .sln file is present the generator is MSBuild.  Pass /nodeReuse:false
    # so that worker-node processes exit immediately instead of staying alive
    # and potentially holding inherited file or pipe handles.
    local nr_args=()
    for _f in "${build_dir}"/*.sln; do
        [[ -f "${_f}" ]] && { nr_args=("--" "/nodeReuse:false"); break; }
    done
    _cmake_run "${log_file}" \
        cmake --build "${build_dir}" \
              --config "${config}" \
              --parallel "${JOBS}" \
              "${nr_args[@]}"
}

# ── Individual build functions ─────────────────────────────────────────────────
build_debug() {
    local log_file="${LOG_DIR}/build_debug_${TIMESTAMP}.log"
    info "Configuring Debug build → ${BUILD_DEBUG}"
    info "Log: ${log_file}"
    mkdir -p "${BUILD_DEBUG}"

    _cmake_run "${log_file}" \
        cmake -B "${BUILD_DEBUG}" \
              -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              "${REPO_ROOT}" || {
        error "Debug configure failed. See: ${log_file}"
        return 1
    }

    info "Building Debug..."
    _cmake_build "${BUILD_DEBUG}" "Debug" "${log_file}" || {
        error "Debug build failed. See: ${log_file}"
        return 1
    }

    info "Debug build complete."
}

build_release() {
    local log_file="${LOG_DIR}/build_release_${TIMESTAMP}.log"
    info "Configuring Release build → ${BUILD_RELEASE}"
    info "Log: ${log_file}"
    mkdir -p "${BUILD_RELEASE}"

    _cmake_run "${log_file}" \
        cmake -B "${BUILD_RELEASE}" \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              "${REPO_ROOT}" || {
        error "Release configure failed. See: ${log_file}"
        return 1
    }

    info "Building Release..."
    _cmake_build "${BUILD_RELEASE}" "Release" "${log_file}" || {
        error "Release build failed. See: ${log_file}"
        return 1
    }

    info "Release build complete."
}

build_all() {
    _track "Debug"   build_debug   || true
    _track "Release" build_release || true
}

do_clean() {
    warn "Removing build directories..."
    rm -rf "${BUILD_DEBUG}" "${BUILD_RELEASE}"
    info "Clean complete."
}

usage() {
    cat <<EOF
Usage: $(basename "$0") <command> [options]

Commands:
  debug      Configure and build Debug configuration
  release    Configure and build Release configuration
  all        Build both Debug and Release
  clean      Remove build directories
  --help     Show this help message

Options:
  --no-wait  Skip the interactive "Press [Enter] to close" pause

Build directories:
  Debug:   ${BUILD_DEBUG}
  Release: ${BUILD_RELEASE}

Logs directory:
  ${LOG_DIR}

Environment overrides:
  CI=true          Disable interactive pause (also set by most CI systems)
  WATCHDOG_TIMEOUT Seconds before a stalled cmake is flagged (default: 600)
  WATCHDOG_ACTION  "report" | "report+kill" | "report+issue" (default: report)
EOF
}

# ── Argument parsing ──────────────────────────────────────────────────────────
CMD=""
for _arg in "$@"; do
    case "${_arg}" in
        --no-wait)    NO_WAIT=true ;;
        --help|-h)    usage; exit 0 ;;
        debug|release|all|clean) CMD="${_arg}" ;;
        *) error "Unknown argument: ${_arg}"; usage; exit 1 ;;
    esac
done

if [[ -z "${CMD}" ]]; then
    usage; exit 1
fi

# ── Dispatch ──────────────────────────────────────────────────────────────────
case "${CMD}" in
    debug)   _track "Debug"   build_debug ;;
    release) _track "Release" build_release ;;
    all)     build_all ;;
    clean)   do_clean ;;
esac

_print_summary
_wait_for_user
[[ ${#_FAIL[@]} -gt 0 ]] && exit 1
exit 0
