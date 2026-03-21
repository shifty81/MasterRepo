#!/usr/bin/env bash
# =============================================================================
# MasterRepo — build.sh
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

# ── Colours ──────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'   # no colour

info()  { echo -e "${GREEN}[build]${NC} $*"; }
warn()  { echo -e "${YELLOW}[warn]${NC}  $*"; }
error() { echo -e "${RED}[error]${NC} $*" >&2; }

# ── Status tracking ───────────────────────────────────────────────────────────
_PASS=()
_FAIL=()

# Run a named build step; records pass/fail without aborting the script.
_track() {
    local label="$1"; shift
    local rc=0
    set +e
    "$@"
    rc=$?
    set -e
    if [[ ${rc} -eq 0 ]]; then
        _PASS+=("${label}")
    else
        _FAIL+=("${label}")
    fi
    return ${rc}
}

# ── Build status report + pause ───────────────────────────────────────────────
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

# ── Ensure log directory exists ───────────────────────────────────────────────
mkdir -p "${LOG_DIR}"

# ── Individual build functions ────────────────────────────────────────────────

build_debug() {
    local log_file="${LOG_DIR}/build_debug_${TIMESTAMP}.log"
    info "Configuring Debug build → ${BUILD_DEBUG}"
    info "Log: ${log_file}"
    mkdir -p "${BUILD_DEBUG}"
    {
        cmake -B "${BUILD_DEBUG}" \
              -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              "${REPO_ROOT}"
        echo ""
        info "Building Debug..."
        cmake --build "${BUILD_DEBUG}" --config Debug --parallel "${JOBS}"
    } 2>&1 | tee "${log_file}"
    info "Debug build complete. Log: ${log_file}"
}

build_release() {
    local log_file="${LOG_DIR}/build_release_${TIMESTAMP}.log"
    info "Configuring Release build → ${BUILD_RELEASE}"
    info "Log: ${log_file}"
    mkdir -p "${BUILD_RELEASE}"
    {
        cmake -B "${BUILD_RELEASE}" \
              -DCMAKE_BUILD_TYPE=Release \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              "${REPO_ROOT}"
        echo ""
        info "Building Release..."
        cmake --build "${BUILD_RELEASE}" --config Release --parallel "${JOBS}"
    } 2>&1 | tee "${log_file}"
    info "Release build complete. Log: ${log_file}"
}

build_all() {
    # Run both configs; || true lets the second run even if the first fails so
    # that both results are captured in the status report.
    _track "Debug"   build_debug   || true
    _track "Release" build_release || true
}

clean() {
    warn "Removing build directories..."
    rm -rf "${BUILD_DEBUG}" "${BUILD_RELEASE}"
    info "Clean complete."
}

# ── Help ──────────────────────────────────────────────────────────────────────

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

if [[ $# -eq 0 ]]; then
    usage
    exit 1
fi

case "$1" in
    debug)
        _track "Debug"   build_debug
        ;;
    release)
        _track "Release" build_release
        ;;
    all)
        build_all
        ;;
    clean)
        clean
        ;;
    --help|-h)
        usage
        exit 0
        ;;
    *)
        error "Unknown command: $1"
        usage
        exit 1
        ;;
esac

# ── Final summary + pause ─────────────────────────────────────────────────────
_print_summary_and_wait
[[ ${#_FAIL[@]} -gt 0 ]] && exit 1
exit 0
