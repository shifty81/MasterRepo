#!/usr/bin/env bash
# =============================================================================
# MasterRepo — build.sh   (Windows / Git Bash / MSYS2)
# Automates CMake configure + build for debug and release configurations.
# Logs are written to Logs/Build/ inside the repository.
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DEBUG="${REPO_ROOT}/Builds/debug"
BUILD_RELEASE="${REPO_ROOT}/Builds/release"
LOG_DIR="${REPO_ROOT}/Logs/Build"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
JOBS="$(nproc 2>/dev/null || echo 4)"
_BUILD_START="${SECONDS}"

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'

info()  { echo -e "${GREEN}[build]${NC} $*"; }
warn()  { echo -e "${YELLOW}[warn]${NC}  $*"; }
error() { echo -e "${RED}[error]${NC} $*" >&2; }

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

_print_summary_and_wait() {
    local elapsed=$(( SECONDS - _BUILD_START ))
    echo ""
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo -e "${BOLD}  MasterRepo Build — ${elapsed}s${NC}"
    for c in "${_PASS[@]:-}"; do [[ -n "${c}" ]] && echo -e "  ${GREEN}✅ ${c}${NC}"; done
    for c in "${_FAIL[@]:-}"; do [[ -n "${c}" ]] && echo -e "  ${RED}❌ ${c}${NC}"; done
    if [[ ${#_PASS[@]} -eq 0 && ${#_FAIL[@]} -eq 0 ]]; then
        echo -e "  (no build steps run)"
    fi
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo -e "  Log dir: ${LOG_DIR}"
    echo ""
    echo -e "${BOLD}Press [Enter] to close...${NC}"
    read -r -p "" 2>/dev/null || true
}

mkdir -p "${LOG_DIR}"

# ── C++ compiler detection ────────────────────────────────────────────────────
# shellcheck source=Scripts/Tools/detect_compiler.sh
source "$(dirname "${BASH_SOURCE[0]}")/detect_compiler.sh" || true
if [[ "${CXX_FOUND}" == "true" ]]; then
    info "Compiler: ${CXX_NAME}"
else
    warn "No C++ compiler found — CMake will attempt its own detection."
fi

# ── Helper: detect MSBuild generator and run cmake --build safely ─────────────
# MSBuild spawns persistent worker-node processes that inherit any pipe
# write-handle, preventing a downstream 'tee' from seeing EOF (= infinite hang).
# Passing /nodeReuse:false tells MSBuild to shut those nodes down after the
# build.  We detect the VS generator by checking for a .sln after configure.
_cmake_build() {
    local build_dir="$1" config="$2" log_file="$3"
    local _nr=()
    for _f in "${build_dir}"/*.sln; do
        [[ -f "${_f}" ]] && { _nr=("--" "/nodeReuse:false"); break; }
    done
    cmake --build "${build_dir}" --config "${config}" \
          --parallel "${JOBS}" "${_nr[@]}" \
          2>&1 | tee -a "${log_file}"
}

# ── Individual build functions ─────────────────────────────────────────────────
build_debug() {
    local log_file="${LOG_DIR}/build_debug_${TIMESTAMP}.log"
    info "Configuring Debug build → ${BUILD_DEBUG}"
    info "Log: ${log_file}"
    mkdir -p "${BUILD_DEBUG}"
    cmake -B "${BUILD_DEBUG}" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          "${REPO_ROOT}" \
          2>&1 | tee "${log_file}" || return 1
    echo ""
    info "Building Debug..."
    _cmake_build "${BUILD_DEBUG}" "Debug" "${log_file}" || return 1
    info "Debug build complete. Log: ${log_file}"
}

build_release() {
    local log_file="${LOG_DIR}/build_release_${TIMESTAMP}.log"
    info "Configuring Release build → ${BUILD_RELEASE}"
    info "Log: ${log_file}"
    mkdir -p "${BUILD_RELEASE}"
    cmake -B "${BUILD_RELEASE}" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          "${REPO_ROOT}" \
          2>&1 | tee "${log_file}" || return 1
    echo ""
    info "Building Release..."
    _cmake_build "${BUILD_RELEASE}" "Release" "${log_file}" || return 1
    info "Release build complete. Log: ${log_file}"
}

build_all() {
    _track "Debug"   build_debug   || true
    _track "Release" build_release || true
}

clean() {
    warn "Removing build directories..."
    rm -rf "${BUILD_DEBUG}" "${BUILD_RELEASE}"
    info "Clean complete."
}

usage() {
    cat <<EOF
Usage: $(basename "$0") <command>

Commands:
  debug      Configure and build Debug configuration
  release    Configure and build Release configuration
  all        Build both Debug and Release
  clean      Remove build directories
  --help     Show this help message

Build directories:
  Debug:   ${BUILD_DEBUG}
  Release: ${BUILD_RELEASE}

Logs directory:
  ${LOG_DIR}
EOF
}

# ── Argument dispatch ─────────────────────────────────────────────────────────
if [[ $# -eq 0 ]]; then usage; exit 1; fi

case "$1" in
    debug)     _track "Debug"   build_debug ;;
    release)   _track "Release" build_release ;;
    all)       build_all ;;
    clean)     clean ;;
    --help|-h) usage; exit 0 ;;
    *)         error "Unknown command: $1"; usage; exit 1 ;;
esac

_print_summary_and_wait
[[ ${#_FAIL[@]} -gt 0 ]] && exit 1
exit 0
