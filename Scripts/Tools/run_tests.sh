#!/usr/bin/env bash
# =============================================================================
# Atlas Engine — run_tests.sh
# Discovers a CMake build directory and runs ctest with verbose output.
# =============================================================================
set -euo pipefail

BUILD_DEBUG="${TMPDIR:-/tmp}/atlas_build_debug"
BUILD_RELEASE="${TMPDIR:-/tmp}/atlas_build_release"
BUILD_ALL="${TMPDIR:-/tmp}/build_all"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
NC='\033[0m'

info()  { echo -e "${GREEN}[test]${NC} $*"; }
warn()  { echo -e "${YELLOW}[test]${NC} $*"; }
error() { echo -e "${RED}[test]${NC} $*" >&2; }

_TEST_START="${SECONDS}"
_TEST_RESULT="UNKNOWN"
_TEST_BUILD_DIR=""

# ── Build status report + pause ───────────────────────────────────────────────
_print_summary_and_wait() {
    local elapsed=$(( SECONDS - _TEST_START ))
    echo ""
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo -e "${BOLD}  MasterRepo Tests — ${elapsed}s${NC}"
    case "${_TEST_RESULT}" in
        PASS)   echo -e "  ${GREEN}✅ All tests passed${NC}" ;;
        FAIL)   echo -e "  ${RED}❌ One or more tests FAILED${NC}" ;;
        SKIP)   echo -e "  ${YELLOW}⏭  Tests skipped (no CTest file found)${NC}" ;;
        *)      echo -e "  ${YELLOW}⚠  Test run did not complete${NC}" ;;
    esac
    [[ -n "${_TEST_BUILD_DIR}" ]] && echo -e "  Build dir: ${_TEST_BUILD_DIR}"
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo ""
    echo -e "${BOLD}Press [Enter] to close...${NC}"
    read -r -p "" 2>/dev/null || true
}

# ── Find a built directory that contains CTestTestfile.cmake ──────────────────

find_build_dir() {
    for dir in "${BUILD_DEBUG}" "${BUILD_RELEASE}" "${BUILD_ALL}"; do
        if [[ -f "${dir}/CTestTestfile.cmake" ]]; then
            echo "${dir}"
            return 0
        fi
    done
    return 1
}

BUILD_DIR="${1:-}"

if [[ -z "${BUILD_DIR}" ]]; then
    if ! BUILD_DIR="$(find_build_dir)"; then
        error "No built directory found. Run build.sh first."
        error "Checked: ${BUILD_DEBUG}, ${BUILD_RELEASE}, ${BUILD_ALL}"
        _TEST_RESULT="SKIP"
        _print_summary_and_wait
        exit 1
    fi
fi

if [[ ! -f "${BUILD_DIR}/CTestTestfile.cmake" ]]; then
    error "No CTestTestfile.cmake in: ${BUILD_DIR}"
    error "Ensure BUILD_TESTS=ON was passed to CMake."
    _TEST_RESULT="SKIP"
    _print_summary_and_wait
    exit 1
fi

_TEST_BUILD_DIR="${BUILD_DIR}"
info "Running tests in: ${BUILD_DIR}"
cd "${BUILD_DIR}"

if ! ctest --output-on-failure --parallel "$(nproc)" "$@"; then
    error "Some tests FAILED."
    _TEST_RESULT="FAIL"
    _print_summary_and_wait
    exit 1
fi

info "All tests passed."
_TEST_RESULT="PASS"
_print_summary_and_wait
exit 0
