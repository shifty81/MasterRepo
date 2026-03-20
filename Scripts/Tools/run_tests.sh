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
NC='\033[0m'

info()  { echo -e "${GREEN}[test]${NC} $*"; }
warn()  { echo -e "${YELLOW}[test]${NC} $*"; }
error() { echo -e "${RED}[test]${NC} $*" >&2; }

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
        exit 1
    fi
fi

if [[ ! -f "${BUILD_DIR}/CTestTestfile.cmake" ]]; then
    error "No CTestTestfile.cmake in: ${BUILD_DIR}"
    error "Ensure BUILD_TESTS=ON was passed to CMake."
    exit 1
fi

info "Running tests in: ${BUILD_DIR}"
cd "${BUILD_DIR}"

if ! ctest --output-on-failure --parallel "$(nproc)" "$@"; then
    error "Some tests FAILED."
    exit 1
fi

info "All tests passed."
