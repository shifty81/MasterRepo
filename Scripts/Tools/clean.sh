#!/usr/bin/env bash
# =============================================================================
# Atlas Engine — clean.sh
# Removes all CMake build artifact directories.
# =============================================================================
set -euo pipefail

BUILD_DEBUG="${TMPDIR:-/tmp}/atlas_build_debug"
BUILD_RELEASE="${TMPDIR:-/tmp}/atlas_build_release"
BUILD_ALL="${TMPDIR:-/tmp}/build_all"

YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

warn()  { echo -e "${YELLOW}[clean]${NC} $*"; }
info()  { echo -e "${GREEN}[clean]${NC} $*"; }

removed=0

for dir in "${BUILD_DEBUG}" "${BUILD_RELEASE}" "${BUILD_ALL}"; do
    if [[ -d "${dir}" ]]; then
        warn "Removing: ${dir}"
        rm -rf "${dir}"
        removed=$((removed + 1))
    fi
done

if [[ $removed -eq 0 ]]; then
    info "Nothing to clean."
else
    info "Removed ${removed} build director$([ $removed -eq 1 ] && echo y || echo ies)."
fi
