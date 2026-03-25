#!/usr/bin/env bash
# =============================================================================
# MasterRepo — clean.sh   (Windows / Git Bash / MSYS2)
# Removes CMake build artifact directories.
# Build directories match build_all.sh: Builds/debug and Builds/release
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DEBUG="${REPO_ROOT}/Builds/debug"
BUILD_RELEASE="${REPO_ROOT}/Builds/release"

# ── Source shared logging library ─────────────────────────────────────────────
# shellcheck source=Scripts/Tools/lib_log.sh
source "$(dirname "${BASH_SOURCE[0]}")/lib_log.sh"
log_init "clean"

YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BOLD='\033[1m'
NC='\033[0m'

warn() { echo -e "${YELLOW}[clean]${NC} $*"; }
info() { echo -e "${GREEN}[clean]${NC} $*"; }

removed=0

for dir in "${BUILD_DEBUG}" "${BUILD_RELEASE}"; do
    if [[ -d "${dir}" ]]; then
        warn "Removing: ${dir}"
        rm -rf "${dir}"
        removed=$(( removed + 1 ))
    fi
done

# Also remove the time-estimate cache so next build starts fresh
CACHE="${REPO_ROOT}/Logs/Build/.build_time_cache"
if [[ -f "${CACHE}" ]]; then
    warn "Removing: ${CACHE}"
    rm -f "${CACHE}"
fi

if [[ ${removed} -eq 0 ]]; then
    info "Nothing to clean."
else
    info "Removed ${removed} build director$([ ${removed} -eq 1 ] && echo y || echo ies)."
fi

log_finish
echo ""
echo -e "${BOLD}Press [Enter] to close...${NC}"
read -r -p "" 2>/dev/null || true
