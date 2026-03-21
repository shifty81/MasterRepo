#!/usr/bin/env bash
# =============================================================================
# MasterRepo — build_all.sh
#
# Builds the entire project in its current state (all phases):
#   Core → Versions → Engine → UI → Editor → Runtime → AI → Agents →
#   Builder → PCG → IDE → Tools → Projects/NovaForge
#
# Output:
#   Binaries  →  Builds/debug/  and  Builds/release/
#   Logs      →  Logs/Build/build_all_<timestamp>.log
#   Report    →  Logs/Build/build_all_<timestamp>_report.md
#
# Usage:
#   ./Scripts/Tools/build_all.sh [--debug-only] [--release-only]
#                                [--jobs N] [--clean] [--help]
# =============================================================================
set -euo pipefail

# ── Resolve paths ─────────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${REPO_ROOT}/Logs/Build"
LOG_FILE="${LOG_DIR}/build_all_${TIMESTAMP}.log"
REPORT_FILE="${LOG_DIR}/build_all_${TIMESTAMP}_report.md"
BUILD_DEBUG="${REPO_ROOT}/Builds/debug"
BUILD_RELEASE="${REPO_ROOT}/Builds/release"

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# Log to both terminal and file
_log()  { echo -e "$*" | tee -a "${LOG_FILE}"; }
info()  { _log "${GREEN}[build_all]${NC} $*"; }
warn()  { _log "${YELLOW}[build_all]${NC} $*"; }
error() { _log "${RED}[build_all ERROR]${NC} $*"; }
section() { _log "\n${CYAN}${BOLD}── $* ──${NC}\n"; }

# ── Defaults ──────────────────────────────────────────────────────────────────
DO_DEBUG=true
DO_RELEASE=true
DO_CLEAN=false
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"  # default 4 is a safe minimum on any machine
START_SECS="${SECONDS}"

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug-only)   DO_RELEASE=false ;;
        --release-only) DO_DEBUG=false   ;;
        --clean)        DO_CLEAN=true    ;;
        --jobs)         JOBS="$2"; shift ;;
        --help|-h)
            cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Builds the entire MasterRepo project (all phases) and writes logs.

Options:
  --debug-only      Build Debug configuration only
  --release-only    Build Release configuration only
  --clean           Remove existing build directories before building
  --jobs N          Parallel compile jobs (default: nproc = ${JOBS})
  --help            Show this message

Output:
  Binaries  →  ${BUILD_DEBUG}/   ${BUILD_RELEASE}/
  Log       →  Logs/Build/build_all_<timestamp>.log
  Report    →  Logs/Build/build_all_<timestamp>_report.md
EOF
            exit 0 ;;
        *) warn "Unknown option: $1" ;;
    esac
    shift
done

# ── Prepare log dir ───────────────────────────────────────────────────────────
mkdir -p "${LOG_DIR}"
: > "${LOG_FILE}"   # create / truncate log

# ── Header ────────────────────────────────────────────────────────────────────
section "MasterRepo — Build All  [${TIMESTAMP}]"
info "Repo root : ${REPO_ROOT}"
info "Debug dir : ${BUILD_DEBUG}"
info "Release dir: ${BUILD_RELEASE}"
info "Log file  : ${LOG_FILE}"
info "Jobs      : ${JOBS}"
echo ""

# ── Environment check ─────────────────────────────────────────────────────────
section "Environment"

check_cmd() {
    local cmd="$1" label="${2:-$1}"
    if command -v "${cmd}" &>/dev/null; then
        info "  ${label}: $(${cmd} --version 2>&1 | head -1)"
    else
        error "  ${label}: NOT FOUND (required)"
        return 1
    fi
}

check_cmd cmake
# Accept any C++ compiler: honour $CXX env var first, then probe common names
_cxx_found=false
if [[ -n "${CXX:-}" ]] && command -v "${CXX}" &>/dev/null; then
    info "  c++ compiler: ${CXX} — $("${CXX}" --version 2>&1 | head -1)"
    _cxx_found=true
elif command -v g++ &>/dev/null; then
    info "  c++ compiler: $(g++ --version | head -1)"
    _cxx_found=true
elif command -v clang++ &>/dev/null; then
    info "  c++ compiler: $(clang++ --version | head -1)"
    _cxx_found=true
elif command -v c++ &>/dev/null; then
    info "  c++ compiler: $(c++ --version | head -1)"
    _cxx_found=true
elif command -v gcc &>/dev/null; then
    info "  c++ compiler: gcc (via $(gcc --version | head -1))"
    _cxx_found=true
elif command -v cc &>/dev/null; then
    info "  c++ compiler: cc (via $(cc --version 2>&1 | head -1))"
    _cxx_found=true
elif command -v cl &>/dev/null; then
    info "  c++ compiler: $(cl 2>&1 | head -1)"
    _cxx_found=true
fi
if [[ "${_cxx_found}" == "false" ]]; then
    warn "  No C++ compiler found in PATH (\$CXX / g++ / clang++ / c++ / gcc / cc / cl)."
    warn "  CMake will attempt its own compiler detection — configure may fail if none is installed."
fi

# ── Clean ─────────────────────────────────────────────────────────────────────
if [[ "${DO_CLEAN}" == "true" ]]; then
    section "Clean"
    if [[ "${DO_DEBUG}" == "true" && -d "${BUILD_DEBUG}" ]]; then
        warn "Removing ${BUILD_DEBUG}"
        rm -rf "${BUILD_DEBUG}"
    fi
    if [[ "${DO_RELEASE}" == "true" && -d "${BUILD_RELEASE}" ]]; then
        warn "Removing ${BUILD_RELEASE}"
        rm -rf "${BUILD_RELEASE}"
    fi
    info "Clean done."
fi

# ── Build tracking ────────────────────────────────────────────────────────────
PASS=(); FAIL=()

run_phase() {
    local label="$1" build_dir="$2" build_type="$3"
    local config_log="${LOG_DIR}/${label,,}_${TIMESTAMP}.log"

    section "${label} (${build_type})"
    info "CMake configure → ${build_dir}"
    mkdir -p "${build_dir}"

    local ok=true

    {
        # Configure
        cmake -S "${REPO_ROOT}" \
              -B "${build_dir}" \
              -DCMAKE_BUILD_TYPE="${build_type}" \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DBUILD_EDITOR=ON \
              -DBUILD_RUNTIME=ON \
              -DBUILD_AI=ON \
              -DBUILD_TOOLS=ON
    } 2>&1 | tee -a "${LOG_FILE}" "${config_log}" || ok=false

    if [[ "${ok}" == "true" ]]; then
        info "CMake build → ${build_dir}"
        {
            cmake --build "${build_dir}" -- -j"${JOBS}"
        } 2>&1 | tee -a "${LOG_FILE}" "${config_log}" || ok=false
    fi

    if [[ "${ok}" == "true" ]]; then
        info "✅ ${label} complete. Log: ${config_log}"
        PASS+=("${label}")
    else
        error "❌ ${label} FAILED. Log: ${config_log}"
        FAIL+=("${label}")
    fi
}

# ── Debug build ───────────────────────────────────────────────────────────────
if [[ "${DO_DEBUG}" == "true" ]]; then
    run_phase "Debug" "${BUILD_DEBUG}" "Debug"
fi

# ── Release build ─────────────────────────────────────────────────────────────
if [[ "${DO_RELEASE}" == "true" ]]; then
    run_phase "Release" "${BUILD_RELEASE}" "Release"
fi

# ── Symlink compile_commands.json to repo root (for IDE tooling) ──────────────
# Prefer Debug (more diagnostics), fall back to Release.
if [[ -f "${BUILD_DEBUG}/compile_commands.json" ]]; then
    ln -sf "${BUILD_DEBUG}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || true
    info "compile_commands.json → ${BUILD_DEBUG}/compile_commands.json"
elif [[ -f "${BUILD_RELEASE}/compile_commands.json" ]]; then
    ln -sf "${BUILD_RELEASE}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || true
    info "compile_commands.json → ${BUILD_RELEASE}/compile_commands.json"
fi

# ── Report ────────────────────────────────────────────────────────────────────
ELAPSED=$(( SECONDS - START_SECS ))

section "Build Summary"

{
    echo "# MasterRepo — Build All Report"
    echo ""
    echo "**Date:**      $(date '+%Y-%m-%d %H:%M:%S')"
    echo "**Elapsed:**   ${ELAPSED}s"
    echo "**Repo:**      ${REPO_ROOT}"
    echo "**Jobs:**      ${JOBS}"
    echo ""
    echo "## Results"
    echo ""
    echo "| Configuration | Result |"
    echo "|---------------|--------|"
    _has_rows=false
    for s in "${PASS[@]:-}"; do [[ -n "${s}" ]] && echo "| ${s} | ✅ Pass |" && _has_rows=true; done
    for s in "${FAIL[@]:-}"; do [[ -n "${s}" ]] && echo "| ${s} | ❌ Fail |" && _has_rows=true; done
    [[ "${_has_rows}" == "false" ]] && echo "| (no builds run) | — |"
    echo ""
    echo "## Binaries"
    echo ""
    if [[ "${DO_DEBUG}" == "true" && -d "${BUILD_DEBUG}/bin" ]]; then
        echo "### Debug"
        echo '```'
        ls "${BUILD_DEBUG}/bin/" 2>/dev/null || echo "(none)"
        echo '```'
    fi
    if [[ "${DO_RELEASE}" == "true" && -d "${BUILD_RELEASE}/bin" ]]; then
        echo "### Release"
        echo '```'
        ls "${BUILD_RELEASE}/bin/" 2>/dev/null || echo "(none)"
        echo '```'
    fi
    echo ""
    echo "## Log Files"
    echo ""
    echo "- Full log: \`${LOG_FILE}\`"
    echo "- Report:   \`${REPORT_FILE}\`"
} | tee -a "${LOG_FILE}" > "${REPORT_FILE}"

# ── Terminal summary ──────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}═══════════════════════════════════════════${NC}"
echo -e "${BOLD}  MasterRepo Build All — ${ELAPSED}s${NC}"
echo -e "  ✅ Passed : ${#PASS[@]}"
echo -e "  ❌ Failed : ${#FAIL[@]}"
echo -e "${BOLD}═══════════════════════════════════════════${NC}"
echo -e "  Log    : ${LOG_FILE}"
echo -e "  Report : ${REPORT_FILE}"
echo ""

if [[ ${#FAIL[@]} -gt 0 ]]; then
    error "Build had failures. Check the log above."
    exit 1
fi

exit 0
