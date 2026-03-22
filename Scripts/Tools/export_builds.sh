#!/usr/bin/env bash
# =============================================================================
# MasterRepo — export_builds.sh   (Windows / Git Bash / MSYS2)
#
# Builds one or more Windows presets and packages each into a .zip archive
# in Builds/Packages/.
#
# Available presets  (defined in CMakePresets.json):
#   windows-x64          Windows 10/11 x64 Release  (MSVC or MinGW)
#   windows-x64-debug    Windows 10/11 x64 Debug
#   all                  Both of the above
#
# Options:
#   --no-package       Skip creating .zip archives
#   --output DIR       Override Builds/Packages output directory
#   --jobs N           Parallel compile jobs (default: nproc)
#   --clean            Remove existing build dirs before building
#   --help             Show this message
#
# Example:
#   ./Scripts/Tools/export_builds.sh windows-x64
#   ./Scripts/Tools/export_builds.sh all --no-package
# =============================================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m';  BOLD='\033[1m';      NC='\033[0m'

info()    { echo -e "${GREEN}[EXPORT]${NC} $*"; }
warn()    { echo -e "${YELLOW}[EXPORT]${NC} $*"; }
error()   { echo -e "${RED}[EXPORT ERROR]${NC} $*" >&2; }
section() { echo -e "\n${CYAN}${BOLD}── $* ──${NC}\n"; }

_wait_for_user() {
    echo ""
    echo -e "${BOLD}Press [Enter] to close...${NC}"
    read -r -p "" 2>/dev/null || true
}

# ── Helper: run cmake without a pipe to prevent MSBuild worker-node hang ──────
# On Windows, MSBuild spawns persistent worker-node processes that inherit any
# pipe write-handle, preventing tee from seeing EOF (= infinite hang).
# Fix: redirect output directly to the log file (no pipe), then stream via
# tail -f (an MSYS2 binary — reliable Unix-to-Unix I/O, no SIGPIPE issues).
_cmake_run() {
    local log_file="$1"; shift
    : >> "${log_file}"                     # ensure file exists before tail
    "$@" >> "${log_file}" 2>&1 &
    local _pid=$!
    tail -f "${log_file}" &
    local _tail_pid=$!
    local _rc=0
    wait "${_pid}" || _rc=$?
    sleep 0.1 2>/dev/null || true          # let tail flush last bytes
    kill "${_tail_pid}" 2>/dev/null || true
    wait "${_tail_pid}" 2>/dev/null || true
    return "${_rc}"
}

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
PACKAGE_DIR="${REPO_ROOT}/Builds/Packages"
JOBS="$(nproc 2>/dev/null || echo 4)"
DO_PACKAGE=true
CLEAN=false

# Windows-only presets
ALL_PRESETS=("windows-x64" "windows-x64-debug")
SELECTED_PRESETS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        all)          SELECTED_PRESETS=("${ALL_PRESETS[@]}") ;;
        --no-package) DO_PACKAGE=false ;;
        --output)     PACKAGE_DIR="$2"; shift ;;
        --jobs)       JOBS="$2"; shift ;;
        --clean)      CLEAN=true ;;
        --help|-h)
            cat <<EOF
Usage: $(basename "$0") [PRESET ...] [OPTIONS]

Windows presets: windows-x64  windows-x64-debug  all

Options:
  --no-package    Skip .zip creation
  --output DIR    Override output directory (default: Builds/Packages)
  --jobs N        Parallel compile jobs
  --clean         Remove existing build dirs first
  --help          Show this message
EOF
            exit 0 ;;
        -*) warn "Unknown option: $1" ;;
        *)  SELECTED_PRESETS+=("$1") ;;
    esac
    shift
done

if [[ ${#SELECTED_PRESETS[@]} -eq 0 ]]; then
    error "No presets specified.  Use a preset name or 'all'."
    error "Available: windows-x64  windows-x64-debug  all"
    exit 1
fi

mkdir -p "${PACKAGE_DIR}"

PASSED=(); FAILED=(); SKIPPED=()

build_preset() {
    local preset="$1"
    section "Building: ${preset}"

    # Validate preset is a known Windows preset
    local valid=false
    local p
    for p in "${ALL_PRESETS[@]}"; do
        [[ "${p}" == "${preset}" ]] && valid=true && break
    done
    if [[ "${valid}" != "true" ]]; then
        warn "Unknown preset '${preset}' — only Windows presets are supported."
        warn "Valid presets: ${ALL_PRESETS[*]}"
        SKIPPED+=("${preset}")
        return 0
    fi

    local build_dir="${REPO_ROOT}/Builds/${preset}"

    if [[ "${CLEAN}" == "true" && -d "${build_dir}" ]]; then
        info "Removing ${build_dir}"
        rm -rf "${build_dir}"
    fi

    # ── Configure ─────────────────────────────────────────────────────────────
    local configure_log="${PACKAGE_DIR}/${preset}_configure.log"
    info "Configuring preset '${preset}'…"
    if ! _cmake_run "${configure_log}" \
            cmake --preset "${preset}" \
                  -S "${REPO_ROOT}"; then
        error "Configure failed for '${preset}' — see ${configure_log}"
        FAILED+=("${preset}")
        return 1
    fi

    # ── Build ─────────────────────────────────────────────────────────────────
    local build_log="${PACKAGE_DIR}/${preset}_build.log"
    info "Building preset '${preset}'…"

    # Detect MSBuild generator (.sln present) and disable node reuse to prevent
    # worker-node handles from keeping a pipe open after the build finishes.
    local _nr=()
    for _f in "${build_dir}"/*.sln; do
        [[ -f "${_f}" ]] && { _nr=("--" "/nodeReuse:false"); break; }
    done

    if ! _cmake_run "${build_log}" \
            cmake --build --preset "${preset}" \
                  --parallel "${JOBS}" \
                  "${_nr[@]}"; then
        error "Build failed for '${preset}' — see ${build_log}"
        FAILED+=("${preset}")
        return 1
    fi

    info "Build succeeded: ${preset}"
    PASSED+=("${preset}")

    # ── Package ───────────────────────────────────────────────────────────────
    if [[ "${DO_PACKAGE}" == "true" ]]; then
        local pkg_name="AtlasEngine-${preset}-${TIMESTAMP}"
        local pkg_dir="${PACKAGE_DIR}/${pkg_name}"
        local zipfile="${PACKAGE_DIR}/${pkg_name}.zip"

        mkdir -p "${pkg_dir}/bin"

        for sub in bin lib; do
            [[ -d "${build_dir}/${sub}" ]] && \
                cp -r "${build_dir}/${sub}/." "${pkg_dir}/${sub}/" 2>/dev/null || true
        done
        [[ -d "${REPO_ROOT}/Config"  ]] && cp -r "${REPO_ROOT}/Config"  "${pkg_dir}/"
        [[ -f "${REPO_ROOT}/README.md" ]] && cp "${REPO_ROOT}/README.md" "${pkg_dir}/"

        if command -v zip &>/dev/null; then
            (cd "${PACKAGE_DIR}" && zip -r "${zipfile}" "${pkg_name}") && \
                info "📦 Package: ${zipfile}" || \
                warn "  zip failed for ${preset} — folder at ${pkg_dir}"
        else
            warn "  zip not found — package folder left at ${pkg_dir}"
            warn "  Install zip: pacman -S zip  (MSYS2) or use 7-Zip"
        fi

        rm -rf "${pkg_dir}"
    fi
}

# ── Main loop ─────────────────────────────────────────────────────────────────
for preset in "${SELECTED_PRESETS[@]}"; do
    build_preset "${preset}" || true
done

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}═══════════════════════════════════════${NC}"
echo -e "${BOLD}  Atlas Export Summary${NC}"
echo -e "  ✅ Passed:  ${#PASSED[@]}   — ${PASSED[*]:-none}"
echo -e "  ❌ Failed:  ${#FAILED[@]}   — ${FAILED[*]:-none}"
echo -e "  ⏭  Skipped: ${#SKIPPED[@]}  — ${SKIPPED[*]:-none}"
echo -e "${BOLD}═══════════════════════════════════════${NC}"

_wait_for_user
[[ ${#FAILED[@]} -gt 0 ]] && exit 1
exit 0
