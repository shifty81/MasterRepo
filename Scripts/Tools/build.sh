#!/usr/bin/env bash
# =============================================================================
# Atlas Engine — build.sh
# Automates CMake configure + build for debug and release configurations.
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DEBUG="${TMPDIR:-/tmp}/atlas_build_debug"
BUILD_RELEASE="${TMPDIR:-/tmp}/atlas_build_release"

# ── Colours ──────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'   # no colour

info()  { echo -e "${GREEN}[build]${NC} $*"; }
warn()  { echo -e "${YELLOW}[warn]${NC}  $*"; }
error() { echo -e "${RED}[error]${NC} $*" >&2; }

# ── Individual build functions ────────────────────────────────────────────────

build_debug() {
    info "Configuring Debug build → ${BUILD_DEBUG}"
    cmake -B "${BUILD_DEBUG}" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          "${REPO_ROOT}"

    info "Building Debug..."
    cmake --build "${BUILD_DEBUG}" -- -j"$(nproc)"
    info "Debug build complete."
}

build_release() {
    info "Configuring Release build → ${BUILD_RELEASE}"
    cmake -B "${BUILD_RELEASE}" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          "${REPO_ROOT}"

    info "Building Release..."
    cmake --build "${BUILD_RELEASE}" -- -j"$(nproc)"
    info "Release build complete."
}

build_all() {
    build_debug
    build_release
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
  clean      Remove all build directories
  --help     Show this help message

Build directories:
  Debug:   ${BUILD_DEBUG}
  Release: ${BUILD_RELEASE}
EOF
}

# ── Argument dispatch ─────────────────────────────────────────────────────────

if [[ $# -eq 0 ]]; then
    usage
    exit 1
fi

case "$1" in
    debug)   build_debug   ;;
    release) build_release ;;
    all)     build_all     ;;
    clean)   clean         ;;
    --help|-h) usage       ;;
    *)
        error "Unknown command: $1"
        usage
        exit 1
        ;;
esac
