#!/usr/bin/env bash
# =============================================================================
# Atlas Engine — ci_pipeline.sh  (Phase 10.7 — CI/CD Pipeline)
#
# Full automated pipeline:
#   1. Validate environment
#   2. Configure + build (Debug & Release)
#   3. Run tests
#   4. Run static analysis (cppcheck if available)
#   5. Generate API documentation (DocGenerator)
#   6. Package release artifact
#   7. Produce a build report
#
# Usage:
#   ./Scripts/Tools/ci_pipeline.sh [--debug-only] [--no-docs] [--no-package]
#                                  [--build-dir DIR] [--output DIR]
#
# Exit codes:
#   0  — all stages passed
#   1  — configuration error
#   2  — build failure
#   3  — test failure
#   4  — packaging failure
# =============================================================================
set -euo pipefail

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

info()    { echo -e "${GREEN}[CI]${NC} $*"; }
warn()    { echo -e "${YELLOW}[CI]${NC} $*"; }
error()   { echo -e "${RED}[CI ERROR]${NC} $*" >&2; }
section() { echo -e "\n${CYAN}${BOLD}══ $* ══${NC}\n"; }

# ── Pause until the user presses Enter (keeps the terminal window open) ────────
_wait_for_user() {
    echo ""
    echo -e "${BOLD}Press [Enter] to close...${NC}"
    read -r -p "" 2>/dev/null || true
}
trap '_wait_for_user' EXIT

# ── Paths ─────────────────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
DEFAULT_BUILD_BASE="${REPO_ROOT}/Builds/ci_${TIMESTAMP}"
DEFAULT_OUTPUT="${REPO_ROOT}/Builds/CI_${TIMESTAMP}"

# ── Defaults ─────────────────────────────────────────────────────────────────
BUILD_BASE="${DEFAULT_BUILD_BASE}"
OUTPUT_DIR="${DEFAULT_OUTPUT}"
BUILD_DEBUG=true
BUILD_RELEASE=true
RUN_TESTS=true
RUN_DOCS=true
RUN_PACKAGE=true
RUN_LINT=true

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug-only)   BUILD_RELEASE=false ;;
        --release-only) BUILD_DEBUG=false ;;
        --no-tests)     RUN_TESTS=false ;;
        --no-docs)      RUN_DOCS=false ;;
        --no-package)   RUN_PACKAGE=false ;;
        --no-lint)      RUN_LINT=false ;;
        --build-dir)    BUILD_BASE="$2"; shift ;;
        --output)       OUTPUT_DIR="$2"; shift ;;
        --help|-h)
            cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --debug-only      Only build Debug configuration
  --release-only    Only build Release configuration
  --no-tests        Skip test execution
  --no-docs         Skip documentation generation
  --no-package      Skip artifact packaging
  --no-lint         Skip cppcheck static analysis
  --build-dir DIR   Override CMake build base directory
  --output DIR      Override output / artifact directory
  --help            Show this message

Exit codes: 0=success 1=env 2=build 3=test 4=package
EOF
            exit 0 ;;
        *) warn "Unknown option: $1" ;;
    esac
    shift
done

BUILD_DIR_DEBUG="${BUILD_BASE}/debug"
BUILD_DIR_RELEASE="${BUILD_BASE}/release"
REPORT_FILE="${OUTPUT_DIR}/ci_report_${TIMESTAMP}.md"
ARTIFACT_DIR="${OUTPUT_DIR}/artifacts"

mkdir -p "${OUTPUT_DIR}" "${ARTIFACT_DIR}"

# ── State tracking ────────────────────────────────────────────────────────────
STAGES_PASSED=()
STAGES_FAILED=()
STAGES_SKIPPED=()
START_TIME="${SECONDS}"

stage_pass()   { STAGES_PASSED+=("$1");  info  "✅ $1"; }
stage_fail()   { STAGES_FAILED+=("$1");  error "❌ $1"; }
stage_skip()   { STAGES_SKIPPED+=("$1"); warn  "⏭  $1 (skipped)"; }

# ── Stage 1: Environment validation ──────────────────────────────────────────
section "Stage 1: Environment Validation"

ENV_OK=true

check_tool() {
    local tool="$1"
    if command -v "${tool}" &>/dev/null; then
        # Capture ALL output first; do NOT pipe directly to 'head -1'.
        # On Windows, SIGPIPE is not reliably delivered to native processes, so
        # the subprocess can hang indefinitely on a broken pipe.  Extract the
        # first line with bash parameter expansion instead.
        local _out _ver
        _out="$("${tool}" --version 2>&1 || true)"
        _ver="${_out%%$'\n'*}"
        _ver="${_ver%%$'\r'*}"
        [[ -z "${_ver}" ]] && _ver="(version unknown)"
        info "  ${tool}: ${_ver}"
    else
        if [[ "${2:-required}" == "required" ]]; then
            error "  ${tool} not found (required)"
            ENV_OK=false
        else
            warn "  ${tool} not found (optional)"
        fi
    fi
}

check_tool cmake "" required
# ── C++ compiler detection (shared helper) ────────────────────────────────────
# shellcheck source=Scripts/Tools/detect_compiler.sh
source "$(dirname "${BASH_SOURCE[0]}")/detect_compiler.sh"
if [[ "${CXX_FOUND}" == "true" ]]; then
    info "  c++ compiler: ${CXX_NAME}"
else
    error "  No C++ compiler found — see instructions above"
    ENV_OK=false
fi
check_tool git  "" required
check_tool cppcheck "" optional
check_tool cpack "" optional

CMAKE_JOBS="$(nproc 2>/dev/null || echo 4)"
info "  Parallel jobs: ${CMAKE_JOBS}"

if [[ "${ENV_OK}" == "true" ]]; then
    stage_pass "Environment"
else
    stage_fail "Environment"
    error "Environment validation failed — aborting"
    exit 1
fi

# ── Stage 2: Debug Build ──────────────────────────────────────────────────────
if [[ "${BUILD_DEBUG}" == "true" ]]; then
    section "Stage 2: Debug Build"
    # Configure
    cmake -B "${BUILD_DIR_DEBUG}" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
          -DBUILD_TESTS=ON \
          "${REPO_ROOT}" 2>&1 | tee "${OUTPUT_DIR}/cmake_debug.log" || {
        stage_fail "Debug Build"
        error "Debug configure failed — see ${OUTPUT_DIR}/cmake_debug.log"
        exit 2
    }
    # Build — detect MSBuild generator (.sln) and pass /nodeReuse:false to
    # prevent worker-node handles from keeping any pipe open after the build.
    _ci_nr_debug=()
    for _f in "${BUILD_DIR_DEBUG}"/*.sln; do
        [[ -f "${_f}" ]] && { _ci_nr_debug=("--" "/nodeReuse:false"); break; }
    done
    if cmake --build "${BUILD_DIR_DEBUG}" \
             --parallel "${CMAKE_JOBS}" \
             "${_ci_nr_debug[@]}" \
             2>&1 | tee "${OUTPUT_DIR}/build_debug.log"; then
        stage_pass "Debug Build"
        # Copy compile_commands.json to repo root for IDE tooling
        cp "${BUILD_DIR_DEBUG}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || true
    else
        stage_fail "Debug Build"
        error "Debug build failed — see ${OUTPUT_DIR}/build_debug.log"
        exit 2
    fi
else
    stage_skip "Debug Build"
fi

# ── Stage 3: Release Build ────────────────────────────────────────────────────
if [[ "${BUILD_RELEASE}" == "true" ]]; then
    section "Stage 3: Release Build"
    cmake -B "${BUILD_DIR_RELEASE}" \
          -DCMAKE_BUILD_TYPE=Release \
          "${REPO_ROOT}" 2>&1 | tee "${OUTPUT_DIR}/cmake_release.log" || {
        stage_fail "Release Build"
        error "Release configure failed — see ${OUTPUT_DIR}/cmake_release.log"
        exit 2
    }
    _ci_nr_release=()
    for _f in "${BUILD_DIR_RELEASE}"/*.sln; do
        [[ -f "${_f}" ]] && { _ci_nr_release=("--" "/nodeReuse:false"); break; }
    done
    if cmake --build "${BUILD_DIR_RELEASE}" \
             --parallel "${CMAKE_JOBS}" \
             "${_ci_nr_release[@]}" \
             2>&1 | tee "${OUTPUT_DIR}/build_release.log"; then
        stage_pass "Release Build"
    else
        stage_fail "Release Build"
        error "Release build failed — see ${OUTPUT_DIR}/build_release.log"
        exit 2
    fi
else
    stage_skip "Release Build"
fi

# ── Stage 4: Tests ────────────────────────────────────────────────────────────
if [[ "${RUN_TESTS}" == "true" ]]; then
    section "Stage 4: Tests"
    TEST_DIR=""
    if [[ -f "${BUILD_DIR_DEBUG}/CTestTestfile.cmake" ]]; then
        TEST_DIR="${BUILD_DIR_DEBUG}"
    elif [[ -f "${BUILD_DIR_RELEASE}/CTestTestfile.cmake" ]]; then
        TEST_DIR="${BUILD_DIR_RELEASE}"
    fi

    if [[ -z "${TEST_DIR}" ]]; then
        warn "No CTestTestfile.cmake found — was BUILD_TESTS=ON set?"
        stage_skip "Tests"
    elif (cd "${TEST_DIR}" && ctest --output-on-failure --parallel "${CMAKE_JOBS}" \
                                   --output-log "${OUTPUT_DIR}/ctest.log"); then
        stage_pass "Tests"
    else
        stage_fail "Tests"
        error "Tests failed — see ${OUTPUT_DIR}/ctest.log"
        exit 3
    fi
else
    stage_skip "Tests"
fi

# ── Stage 5: Static Analysis ──────────────────────────────────────────────────
if [[ "${RUN_LINT}" == "true" ]] && command -v cppcheck &>/dev/null; then
    section "Stage 5: Static Analysis (cppcheck)"
    CPPCHECK_OUT="${OUTPUT_DIR}/cppcheck.xml"
    cppcheck \
        --enable=warning,performance,portability \
        --std=c++20 \
        --suppress=missingIncludeSystem \
        --xml --xml-version=2 \
        "${REPO_ROOT}/Core" "${REPO_ROOT}/Engine" "${REPO_ROOT}/Runtime" \
        "${REPO_ROOT}/Editor" "${REPO_ROOT}/AI" "${REPO_ROOT}/Tools" \
        2>"${CPPCHECK_OUT}" || true
    ISSUES="$(grep -c '<error ' "${CPPCHECK_OUT}" 2>/dev/null || echo 0)"
    if [[ "${ISSUES}" -gt 0 ]]; then
        warn "cppcheck found ${ISSUES} issue(s) — see ${CPPCHECK_OUT}"
    else
        info "cppcheck: no issues found"
    fi
    stage_pass "Static Analysis"
else
    stage_skip "Static Analysis"
fi

# ── Stage 6: Documentation ────────────────────────────────────────────────────
if [[ "${RUN_DOCS}" == "true" ]]; then
    section "Stage 6: Documentation"
    DOC_OUTPUT="${REPO_ROOT}/Docs/API"
    mkdir -p "${DOC_OUTPUT}"
    # The DocGenerator executable is built as part of the project.
    # Invoke it if available, otherwise fall back to the Markdown manual.
    if [[ -x "${BUILD_DIR_RELEASE}/bin/AtlasDocGen" ]]; then
        "${BUILD_DIR_RELEASE}/bin/AtlasDocGen" \
            --scan "${REPO_ROOT}" \
            --output "${DOC_OUTPUT}" \
            2>&1 | tee "${OUTPUT_DIR}/docgen.log"
    else
        info "DocGenerator executable not found — skipping auto-docs (manual already present)"
    fi
    stage_pass "Documentation"
else
    stage_skip "Documentation"
fi

# ── Stage 7: Package ─────────────────────────────────────────────────────────
if [[ "${RUN_PACKAGE}" == "true" ]] && [[ "${BUILD_RELEASE}" == "true" ]]; then
    section "Stage 7: Packaging"
    PKG_NAME="AtlasEngine-${TIMESTAMP}"

    # Copy release binaries
    mkdir -p "${ARTIFACT_DIR}/${PKG_NAME}/bin"
    cp -r "${BUILD_DIR_RELEASE}/bin/"* "${ARTIFACT_DIR}/${PKG_NAME}/bin/" 2>/dev/null || true

    # Copy configs and scripts
    cp -r "${REPO_ROOT}/Config"         "${ARTIFACT_DIR}/${PKG_NAME}/"
    cp -r "${REPO_ROOT}/Scripts"        "${ARTIFACT_DIR}/${PKG_NAME}/"
    cp    "${REPO_ROOT}/README.md"      "${ARTIFACT_DIR}/${PKG_NAME}/"

    # Create zip archive (Windows-native format)
    ZIPFILE="${ARTIFACT_DIR}/${PKG_NAME}.zip"
    if command -v zip &>/dev/null; then
        (cd "${ARTIFACT_DIR}" && zip -r "${ZIPFILE}" "${PKG_NAME}") && \
            info "Package created: ${ZIPFILE}"
    else
        warn "  zip not found — skipping archive (folder left at ${ARTIFACT_DIR}/${PKG_NAME})"
        warn "  Install zip via MSYS2: pacman -S zip"
    fi

    stage_pass "Packaging"
else
    stage_skip "Packaging"
fi

# ── Stage 8: Report ───────────────────────────────────────────────────────────
section "Stage 8: Build Report"

ELAPSED=$(( SECONDS - START_TIME ))

cat > "${REPORT_FILE}" <<REPORT
# Atlas Engine — CI Pipeline Report

**Run date:** $(date '+%Y-%m-%d %H:%M:%S')
**Elapsed:**  ${ELAPSED}s
**Repo:**     ${REPO_ROOT}
**Build:**    ${BUILD_BASE}

## Stage Results

| Stage | Result |
|-------|--------|
$(for s in "${STAGES_PASSED[@]:-}";  do echo "| ${s} | ✅ Pass |"; done)
$(for s in "${STAGES_FAILED[@]:-}";  do echo "| ${s} | ❌ Fail |"; done)
$(for s in "${STAGES_SKIPPED[@]:-}"; do echo "| ${s} | ⏭  Skip |"; done)

## Summary

- **Passed:**  ${#STAGES_PASSED[@]}
- **Failed:**  ${#STAGES_FAILED[@]}
- **Skipped:** ${#STAGES_SKIPPED[@]}

## Artifacts

Output directory: \`${OUTPUT_DIR}\`
REPORT

info "Report written to: ${REPORT_FILE}"

# ── Final summary ─────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}═══════════════════════════════════════${NC}"
echo -e "${BOLD}  Atlas CI — Completed in ${ELAPSED}s${NC}"
echo -e "  ✅ Passed:  ${#STAGES_PASSED[@]}"
echo -e "  ❌ Failed:  ${#STAGES_FAILED[@]}"
echo -e "  ⏭  Skipped: ${#STAGES_SKIPPED[@]}"
echo -e "${BOLD}═══════════════════════════════════════${NC}"

if [[ ${#STAGES_FAILED[@]} -gt 0 ]]; then
    exit 1
fi

exit 0
