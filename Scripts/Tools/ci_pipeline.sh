#!/usr/bin/env bash
# =============================================================================
# MasterRepo — ci_pipeline.sh  (Windows / Git Bash / MSYS2)
#
# Full automated pipeline:
#   1. Validate environment
#   2. Configure + build (Debug & Release)
#   3. Run tests
#   4. Run static analysis (cppcheck if available)
#   5. Generate API documentation (DocGenerator if built)
#   6. Package release artifact
#   7. Produce a build report
#
# ── Hang vectors fixed in this script ────────────────────────────────────────
#
#   1. cmake piped through tee:
#        Same MSBuild worker-node handle issue as in build.sh.
#        Fix: cmake runs in background, output → log file directly (no pipe).
#
#   2. 'trap _wait_for_user EXIT' (previous design):
#        The EXIT trap fired on EVERY exit — including exit 0 on success —
#        causing 'read -r -p ""' to block indefinitely in CI, cron jobs,
#        or any non-interactive invocation.
#        Fix: The EXIT trap is removed.  _wait_for_user is called explicitly
#        only at the normal end of the script, and is itself guarded by
#        _is_interactive().
#
#   3. 'read -r -p ""' without a terminal check:
#        Fix: _is_interactive() gates every read call.
#
#   4. check_tool piping 'cmd --version' through 'head -1':
#        On Windows, SIGPIPE is not reliably delivered to native processes.
#        Fix: capture full output into a variable, extract first line with
#        bash parameter expansion (no subprocess involved).
#
# Usage:
#   ./Scripts/Tools/ci_pipeline.sh [options]
#
# Options:
#   --debug-only      Build Debug only
#   --release-only    Build Release only
#   --no-tests        Skip test execution
#   --no-docs         Skip documentation generation
#   --no-package      Skip artifact packaging
#   --no-lint         Skip cppcheck static analysis
#   --no-wait         Skip interactive pause at end
#   --build-dir DIR   Override CMake build base directory
#   --output DIR      Override output / artifact directory
#   --help
#
# Exit codes:  0=success  1=env  2=build  3=test  4=package
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# ── Source shared libraries ───────────────────────────────────────────────────
# shellcheck source=Scripts/Tools/lib_log.sh
source "${SCRIPT_DIR}/lib_log.sh"
log_init "ci_pipeline"
# shellcheck source=Scripts/Tools/lib_watchdog.sh
source "${SCRIPT_DIR}/lib_watchdog.sh"

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'
CYAN='\033[0;36m';  BOLD='\033[1m';      NC='\033[0m'

info()    { echo -e "${GREEN}[CI]${NC} $*"; }
warn()    { echo -e "${YELLOW}[CI]${NC} $*"; }
error()   { echo -e "${RED}[CI ERROR]${NC} $*" >&2; }
section() { echo -e "\n${CYAN}${BOLD}══ $* ══${NC}\n"; }

# ── Interactive detection ─────────────────────────────────────────────────────
# Returns 0 (true) when stdin & stdout are real terminals, not CI, not dumb,
# and --no-wait was not passed.
NO_WAIT=false
_is_interactive() {
    [[ "${NO_WAIT}"   == "true"  ]] && return 1
    [[ "${CI:-}"      == "true"  ]] && return 1
    [[ "${TERM:-}"    == "dumb"  ]] && return 1
    [[ -t 0 && -t 1 ]]
}

# Note: _wait_for_user is called EXPLICITLY at the end of the script, NOT via
# an EXIT trap.  The previous design used 'trap _wait_for_user EXIT' which
# triggered on every exit (including exit 0) and blocked indefinitely in any
# non-interactive context (CI, cron, piped invocation).
_wait_for_user() {
    _is_interactive || return 0
    printf "\n${BOLD}Press [Enter] to close...${NC}\n"
    read -r -p "" 2>/dev/null || true
}

# ── cmake runner ──────────────────────────────────────────────────────────────
# cmake runs as a background process with output redirected directly to the log.
# No pipe is created between cmake and this script, so MSBuild worker nodes
# cannot keep a write handle alive after cmake exits.
_cmake_run() {
    local log_file="$1"; shift
    : >> "${log_file}"

    "$@" >> "${log_file}" 2>&1 &
    local _pid=$!
    local _rc=0

    watchdog_start "${log_file}" "${WATCHDOG_TIMEOUT:-600}" "${_pid}"
    while kill -0 "${_pid}" 2>/dev/null; do
        sleep 0.5 2>/dev/null || true
    done
    wait "${_pid}" || _rc=$?
    watchdog_stop

    return "${_rc}"
}

# ── Paths & defaults ──────────────────────────────────────────────────────────
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
DEFAULT_BUILD_BASE="${REPO_ROOT}/Builds/ci_${TIMESTAMP}"
DEFAULT_OUTPUT="${REPO_ROOT}/Builds/CI_${TIMESTAMP}"

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
        --no-wait)      NO_WAIT=true ;;
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
  --no-wait         Skip interactive "Press [Enter]" pause
  --build-dir DIR   Override CMake build base directory
  --output DIR      Override output / artifact directory
  --help            Show this message

Environment overrides:
  CI=true            Treated as --no-wait (also set by most CI systems)
  WATCHDOG_TIMEOUT   Seconds before a stalled cmake is flagged (default: 600)

Exit codes: 0=success  1=env  2=build  3=test  4=package
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

CMAKE_JOBS="$(nproc 2>/dev/null || echo 4)"

# ── Stage tracking ────────────────────────────────────────────────────────────
STAGES_PASSED=()
STAGES_FAILED=()
STAGES_SKIPPED=()
START_TIME="${SECONDS}"

stage_pass() { STAGES_PASSED+=("$1"); info  "✅ $1"; }
stage_fail() { STAGES_FAILED+=("$1"); error "❌ $1"; }
stage_skip() { STAGES_SKIPPED+=("$1"); warn  "⏭  $1 (skipped)"; }

# ── Tool check helper ─────────────────────────────────────────────────────────
# Capture full version output first; extract the first line with bash parameter
# expansion.  Never pipe cmd --version through head -1: on Windows, SIGPIPE is
# not reliably delivered to native processes and the writer can hang.
ENV_OK=true
check_tool() {
    local tool="$1"
    local required="${2:-required}"
    if command -v "${tool}" &>/dev/null; then
        local _out _ver
        _out="$("${tool}" --version 2>&1 || true)"
        _ver="${_out%%$'\n'*}"
        _ver="${_ver%%$'\r'*}"
        [[ -z "${_ver}" ]] && _ver="(version unknown)"
        info "  ${tool}: ${_ver}"
    else
        if [[ "${required}" == "required" ]]; then
            error "  ${tool} not found (required)"
            ENV_OK=false
        else
            warn "  ${tool} not found (optional)"
        fi
    fi
}

# ── Stage 1: Environment validation ──────────────────────────────────────────
section "Stage 1: Environment Validation"

check_tool cmake required
# shellcheck source=Scripts/Tools/detect_compiler.sh
source "${SCRIPT_DIR}/detect_compiler.sh"
if [[ "${CXX_FOUND}" == "true" ]]; then
    info "  c++ compiler: ${CXX_NAME}"
else
    error "  No C++ compiler found — see instructions above"
    ENV_OK=false
fi
check_tool git required
check_tool cppcheck optional
check_tool cpack optional

info "  Parallel jobs: ${CMAKE_JOBS}"

if [[ "${ENV_OK}" == "true" ]]; then
    stage_pass "Environment"
else
    stage_fail "Environment"
    error "Environment validation failed — aborting"
    _wait_for_user
    exit 1
fi

# ── Stage 2: Debug Build ──────────────────────────────────────────────────────
if [[ "${BUILD_DEBUG}" == "true" ]]; then
    section "Stage 2: Debug Build"
    if ! _cmake_run "${OUTPUT_DIR}/cmake_debug.log" \
            cmake -B "${BUILD_DIR_DEBUG}" \
                  -DCMAKE_BUILD_TYPE=Debug \
                  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                  -DBUILD_TESTS=ON \
                  "${REPO_ROOT}"; then
        stage_fail "Debug Build"
        error "Debug configure failed — see ${OUTPUT_DIR}/cmake_debug.log"
        _wait_for_user; exit 2
    fi

    local_nr_debug=()
    for _f in "${BUILD_DIR_DEBUG}"/*.sln; do
        [[ -f "${_f}" ]] && { local_nr_debug=("--" "/nodeReuse:false"); break; }
    done

    if _cmake_run "${OUTPUT_DIR}/build_debug.log" \
            cmake --build "${BUILD_DIR_DEBUG}" \
                  --parallel "${CMAKE_JOBS}" \
                  "${local_nr_debug[@]}"; then
        stage_pass "Debug Build"
        cp "${BUILD_DIR_DEBUG}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || true
    else
        stage_fail "Debug Build"
        error "Debug build failed — see ${OUTPUT_DIR}/build_debug.log"
        _wait_for_user; exit 2
    fi
else
    stage_skip "Debug Build"
fi

# ── Stage 3: Release Build ────────────────────────────────────────────────────
if [[ "${BUILD_RELEASE}" == "true" ]]; then
    section "Stage 3: Release Build"
    if ! _cmake_run "${OUTPUT_DIR}/cmake_release.log" \
            cmake -B "${BUILD_DIR_RELEASE}" \
                  -DCMAKE_BUILD_TYPE=Release \
                  "${REPO_ROOT}"; then
        stage_fail "Release Build"
        error "Release configure failed — see ${OUTPUT_DIR}/cmake_release.log"
        _wait_for_user; exit 2
    fi

    local_nr_release=()
    for _f in "${BUILD_DIR_RELEASE}"/*.sln; do
        [[ -f "${_f}" ]] && { local_nr_release=("--" "/nodeReuse:false"); break; }
    done

    if _cmake_run "${OUTPUT_DIR}/build_release.log" \
            cmake --build "${BUILD_DIR_RELEASE}" \
                  --parallel "${CMAKE_JOBS}" \
                  "${local_nr_release[@]}"; then
        stage_pass "Release Build"
    else
        stage_fail "Release Build"
        error "Release build failed — see ${OUTPUT_DIR}/build_release.log"
        _wait_for_user; exit 2
    fi
else
    stage_skip "Release Build"
fi

# ── Stage 4: Tests ────────────────────────────────────────────────────────────
if [[ "${RUN_TESTS}" == "true" ]]; then
    section "Stage 4: Tests"
    TEST_DIR=""
    [[ -f "${BUILD_DIR_DEBUG}/CTestTestfile.cmake"   ]] && TEST_DIR="${BUILD_DIR_DEBUG}"
    [[ -f "${BUILD_DIR_RELEASE}/CTestTestfile.cmake" ]] && TEST_DIR="${BUILD_DIR_RELEASE}"

    if [[ -z "${TEST_DIR}" ]]; then
        warn "No CTestTestfile.cmake found — was BUILD_TESTS=ON set?"
        stage_skip "Tests"
    elif (cd "${TEST_DIR}" && ctest --output-on-failure --parallel "${CMAKE_JOBS}" \
                                    --output-log "${OUTPUT_DIR}/ctest.log"); then
        stage_pass "Tests"
    else
        stage_fail "Tests"
        error "Tests failed — see ${OUTPUT_DIR}/ctest.log"
        _wait_for_user; exit 3
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
    if [[ -x "${BUILD_DIR_RELEASE}/bin/AtlasDocGen" ]]; then
        "${BUILD_DIR_RELEASE}/bin/AtlasDocGen" \
            --scan "${REPO_ROOT}" \
            --output "${DOC_OUTPUT}" \
            >> "${OUTPUT_DIR}/docgen.log" 2>&1 || true
    else
        info "DocGenerator executable not found — skipping auto-docs"
    fi
    stage_pass "Documentation"
else
    stage_skip "Documentation"
fi

# ── Stage 7: Package ─────────────────────────────────────────────────────────
if [[ "${RUN_PACKAGE}" == "true" && "${BUILD_RELEASE}" == "true" ]]; then
    section "Stage 7: Packaging"
    PKG_NAME="AtlasEngine-${TIMESTAMP}"
    mkdir -p "${ARTIFACT_DIR}/${PKG_NAME}/bin"

    cp -r "${BUILD_DIR_RELEASE}/bin/"* "${ARTIFACT_DIR}/${PKG_NAME}/bin/" 2>/dev/null || true
    [[ -d "${REPO_ROOT}/Config"  ]] && cp -r "${REPO_ROOT}/Config"  "${ARTIFACT_DIR}/${PKG_NAME}/"
    [[ -d "${REPO_ROOT}/Scripts" ]] && cp -r "${REPO_ROOT}/Scripts" "${ARTIFACT_DIR}/${PKG_NAME}/"
    [[ -f "${REPO_ROOT}/README.md" ]] && cp "${REPO_ROOT}/README.md" "${ARTIFACT_DIR}/${PKG_NAME}/"

    ZIPFILE="${ARTIFACT_DIR}/${PKG_NAME}.zip"
    if command -v zip &>/dev/null; then
        (cd "${ARTIFACT_DIR}" && zip -r "${ZIPFILE}" "${PKG_NAME}") && \
            info "Package created: ${ZIPFILE}"
    else
        warn "zip not found — skipping archive (folder at ${ARTIFACT_DIR}/${PKG_NAME})"
        warn "Install zip via MSYS2: pacman -S zip"
    fi
    stage_pass "Packaging"
else
    stage_skip "Packaging"
fi

# ── Stage 8: Report ───────────────────────────────────────────────────────────
section "Stage 8: Build Report"
ELAPSED=$(( SECONDS - START_TIME ))

# Write report directly to a file — no tee pipe (avoids any pipe-related hang).
{
    printf "# MasterRepo — CI Pipeline Report\n\n"
    printf "**Run date:** %s\n" "$(date '+%Y-%m-%d %H:%M:%S')"
    printf "**Elapsed:**  %ds\n" "${ELAPSED}"
    printf "**Repo:**     %s\n" "${REPO_ROOT}"
    printf "**Build:**    %s\n\n" "${BUILD_BASE}"
    printf "## Stage Results\n\n"
    printf "| Stage | Result |\n|-------|--------|\n"
    for s in "${STAGES_PASSED[@]:-}";  do [[ -n "${s}" ]] && printf "| %s | ✅ Pass |\n" "${s}"; done
    for s in "${STAGES_FAILED[@]:-}";  do [[ -n "${s}" ]] && printf "| %s | ❌ Fail |\n" "${s}"; done
    for s in "${STAGES_SKIPPED[@]:-}"; do [[ -n "${s}" ]] && printf "| %s | ⏭  Skip |\n" "${s}"; done
    printf "\n## Summary\n\n"
    printf "- **Passed:**  %d\n" "${#STAGES_PASSED[@]}"
    printf "- **Failed:**  %d\n" "${#STAGES_FAILED[@]}"
    printf "- **Skipped:** %d\n\n" "${#STAGES_SKIPPED[@]}"
    printf "## Artifacts\n\nOutput directory: \`%s\`\n" "${OUTPUT_DIR}"
} > "${REPORT_FILE}"

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
    echo ""
    echo -e "  ${YELLOW}To submit a bug report:${NC}"
    echo -e "    bash Scripts/Tools/submit_issue.sh --log \"$(log_file)\""
fi

log_finish

# _wait_for_user is called here explicitly (not via EXIT trap) so it never
# fires accidentally in CI or non-interactive contexts.
_wait_for_user

[[ ${#STAGES_FAILED[@]} -gt 0 ]] && exit 1
exit 0
