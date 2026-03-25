#!/usr/bin/env bash
# =============================================================================
# MasterRepo — run_tests.sh   (Windows / Git Bash / MSYS2)
# Discovers the CMake build directory and runs ctest with verbose output.
# Build directories match build_all.sh: Builds/debug and Builds/release
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DEBUG="${REPO_ROOT}/Builds/debug"
BUILD_RELEASE="${REPO_ROOT}/Builds/release"
JOBS="$(nproc 2>/dev/null || echo 4)"

# ── Source shared logging library ─────────────────────────────────────────────
# shellcheck source=Scripts/Tools/lib_log.sh
source "$(dirname "${BASH_SOURCE[0]}")/lib_log.sh"
log_init "run_tests"

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
    echo -e "  Log file: $(log_file)"
    if [[ "${_TEST_RESULT}" == "FAIL" ]]; then
        echo ""
        echo -e "  ${YELLOW}To submit a bug report with logs:${NC}"
        echo -e "    bash Scripts/Tools/submit_issue.sh --log \"$(log_file)\""
    fi
    echo -e "${BOLD}═══════════════════════════════════════════${NC}"
    echo ""
    log_finish
    echo -e "${BOLD}Press [Enter] to close...${NC}"
    read -r -p "" 2>/dev/null || true
}

# ── Find a build directory that contains CTestTestfile.cmake ──────────────────
find_build_dir() {
    for dir in "${BUILD_DEBUG}" "${BUILD_RELEASE}"; do
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
        error "No built directory found. Run build_all.sh first."
        error "Checked: ${BUILD_DEBUG}, ${BUILD_RELEASE}"
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

if ! ctest --output-on-failure --parallel "${JOBS}" "$@"; then
    error "Some tests FAILED."
    _TEST_RESULT="FAIL"
    _print_summary_and_wait
    exit 1
fi

info "All tests passed."
_TEST_RESULT="PASS"
_print_summary_and_wait
exit 0
