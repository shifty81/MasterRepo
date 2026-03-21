#!/usr/bin/env bash
# =============================================================================
# Atlas Engine — export_builds.sh  (Phase 10.9 — Desktop / Mobile / Web Builds)
#
# Wraps cmake --preset to build one or more platform targets and
# packages each into a distributable archive in Builds/Packages/.
#
# Usage:
#   ./Scripts/Tools/export_builds.sh [PRESET ...] [OPTIONS]
#
# Available presets  (must match CMakePresets.json):
#   linux-x64          Linux desktop (x86-64)
#   linux-arm64        Linux ARM64
#   windows-x64        Windows 10/11 x64  (requires VS2022 or MinGW)
#   macos-universal    macOS Universal (requires Xcode)
#   web-wasm           Browser WebAssembly (requires Emscripten)
#   android-arm64      Android ARM64  (requires Android NDK)
#   headless-server    Server-only headless build
#   all                Build all presets
#
# Options:
#   --no-package       Skip creating .tar.gz archives
#   --output DIR       Override Builds/Packages output directory
#   --jobs N           Parallel compile jobs (default: nproc)
#   --clean            Remove existing build dirs before building
#   --help             Show this message
#
# Example:
#   ./Scripts/Tools/export_builds.sh linux-x64 web-wasm
#   ./Scripts/Tools/export_builds.sh all --no-package
# =============================================================================
set -euo pipefail

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { echo -e "${GREEN}[EXPORT]${NC} $*"; }
warn()    { echo -e "${YELLOW}[EXPORT]${NC} $*"; }
error()   { echo -e "${RED}[EXPORT ERROR]${NC} $*" >&2; }
section() { echo -e "\n${CYAN}${BOLD}── $* ──${NC}\n"; }

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
PACKAGE_DIR="${REPO_ROOT}/Builds/Packages"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
DO_PACKAGE=true
CLEAN=false

SELECTED_PRESETS=()
ALL_PRESETS=(linux-x64 linux-arm64 windows-x64 macos-universal web-wasm android-arm64 headless-server)

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

Presets: linux-x64 linux-arm64 windows-x64 macos-universal web-wasm android-arm64 headless-server all

Options:
  --no-package    Skip .tar.gz creation
  --output DIR    Override output directory
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
    error "No presets specified. Use a preset name or 'all'."
    exit 1
fi

mkdir -p "${PACKAGE_DIR}"

PASSED=(); FAILED=(); SKIPPED=()

build_preset() {
    local preset="$1"
    section "Building: ${preset}"

    local build_dir="${REPO_ROOT}/Builds/${preset}"

    # ── Clean ────────────────────────────────────────────────────────────────
    if [[ "${CLEAN}" == "true" && -d "${build_dir}" ]]; then
        info "Removing ${build_dir}"
        rm -rf "${build_dir}"
    fi

    # ── Configure ────────────────────────────────────────────────────────────
    local configure_log="${PACKAGE_DIR}/${preset}_configure.log"
    if ! cmake --preset "${preset}" \
               -S "${REPO_ROOT}" \
               2>&1 | tee "${configure_log}"; then
        error "Configure failed for preset '${preset}' — see ${configure_log}"
        FAILED+=("${preset}")
        return 1
    fi

    # ── Build ─────────────────────────────────────────────────────────────────
    local build_log="${PACKAGE_DIR}/${preset}_build.log"
    if ! cmake --build --preset "${preset}" \
               --parallel "${JOBS}" \
               2>&1 | tee "${build_log}"; then
        error "Build failed for preset '${preset}' — see ${build_log}"
        FAILED+=("${preset}")
        return 1
    fi

    info "Build succeeded: ${preset}"
    PASSED+=("${preset}")

    # ── Package ───────────────────────────────────────────────────────────────
    if [[ "${DO_PACKAGE}" == "true" ]]; then
        local pkg_name="AtlasEngine-${preset}-${TIMESTAMP}"
        local pkg_dir="${PACKAGE_DIR}/${pkg_name}"
        local tarball="${PACKAGE_DIR}/${pkg_name}.tar.gz"

        mkdir -p "${pkg_dir}/bin"

        # Copy binaries (best-effort; layout differs per platform)
        for sub in bin lib; do
            [[ -d "${build_dir}/${sub}" ]] && cp -r "${build_dir}/${sub}/." "${pkg_dir}/${sub}/"
        done
        # Copy config and readme
        [[ -d "${REPO_ROOT}/Config" ]] && cp -r "${REPO_ROOT}/Config" "${pkg_dir}/"
        [[ -f "${REPO_ROOT}/README.md" ]] && cp "${REPO_ROOT}/README.md" "${pkg_dir}/"

        if (cd "${PACKAGE_DIR}" && tar czf "${pkg_name}.tar.gz" "${pkg_name}"); then
            info "📦 Package: ${tarball}"
            rm -rf "${pkg_dir}"
        else
            warn "Packaging failed for ${preset}"
        fi
    fi
}

# ── Main loop ─────────────────────────────────────────────────────────────────
for preset in "${SELECTED_PRESETS[@]}"; do
    # Check if preset requires a tool that is not present
    case "${preset}" in
        web-wasm)
            if [[ -z "${EMSDK:-}" ]]; then
                warn "Skipping ${preset}: EMSDK env var not set (Emscripten not found)"
                SKIPPED+=("${preset}"); continue
            fi ;;
        android-arm64)
            if [[ -z "${ANDROID_NDK:-}" ]]; then
                warn "Skipping ${preset}: ANDROID_NDK env var not set"
                SKIPPED+=("${preset}"); continue
            fi ;;
        macos-universal)
            if ! command -v xcodebuild &>/dev/null; then
                warn "Skipping ${preset}: Xcode not found"
                SKIPPED+=("${preset}"); continue
            fi ;;
        windows-x64)
            local sysname; sysname="$(uname -s 2>/dev/null || echo "unknown")"
            case "${sysname}" in
                MINGW*|MSYS*|CYGWIN*) : ;;  # on Windows — allow
                *)
                    if ! command -v x86_64-w64-mingw32-g++ &>/dev/null; then
                        warn "Skipping ${preset}: not on Windows and MinGW cross-compiler not found"
                        SKIPPED+=("${preset}"); continue
                    fi ;;
            esac ;;
    esac

    build_preset "${preset}" || true  # continue on failure
done

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}═══════════════════════════════════════${NC}"
echo -e "${BOLD}  Atlas Export Summary${NC}"
echo -e "  ✅ Passed:  ${#PASSED[@]}   — ${PASSED[*]:-none}"
echo -e "  ❌ Failed:  ${#FAILED[@]}   — ${FAILED[*]:-none}"
echo -e "  ⏭  Skipped: ${#SKIPPED[@]}  — ${SKIPPED[*]:-none}"
echo -e "${BOLD}═══════════════════════════════════════${NC}"

[[ ${#FAILED[@]} -gt 0 ]] && exit 1
exit 0
