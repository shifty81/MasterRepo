#!/usr/bin/env bash
# =============================================================================
# MasterRepo — build_all.sh
#
# Builds the entire project (all phases):
#   Core → Versions → Engine → UI → Editor → Runtime → AI → Agents →
#   Builder → PCG → IDE → Tools → Projects/NovaForge
#
# Features:
#   • Locked animated header: "AtlasForge" logo with per-letter wave flip
#   • Stage list with live spinner, elapsed time, and status icons
#   • Overall progress bar with time estimate (learned from previous runs)
#   • cmake runs in a background process — NO pipe, so MSBuild worker-node
#     handles can never keep a pipe open and stall the build indefinitely
#
# Output:
#   Binaries  →  Builds/debug/  and  Builds/release/
#   Logs      →  Logs/Build/build_all_<timestamp>.log
#   Report    →  Logs/Build/build_all_<timestamp>_report.md
#
# Usage:
#   ./Scripts/Tools/build_all.sh [--debug-only] [--release-only]
#                                [--jobs N] [--clean] [--help]
# =============================================================================
set -euo pipefail

# ── Resolve paths ─────────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${REPO_ROOT}/Logs/Build"
LOG_FILE="${LOG_DIR}/build_all_${TIMESTAMP}.log"
REPORT_FILE="${LOG_DIR}/build_all_${TIMESTAMP}_report.md"
BUILD_DEBUG="${REPO_ROOT}/Builds/debug"
BUILD_RELEASE="${REPO_ROOT}/Builds/release"

# ── Colours ($'...' embeds real ESC bytes so plain printf works) ───────────────
GREEN=$'\033[0;32m';  YELLOW=$'\033[1;33m'; RED=$'\033[0;31m'
CYAN=$'\033[0;36m';   BOLD=$'\033[1m';      DIM=$'\033[2m'
WHITE=$'\033[0;37m';  BWHITE=$'\033[1;97m'; NC=$'\033[0m'

# ── Logging — while the animated display is active, go to file only ────────────
_STATUS_ACTIVE=false
_log() {
    if [[ "${_STATUS_ACTIVE}" == "true" ]]; then
        echo -e "$*" >> "${LOG_FILE}"
    else
        echo -e "$*" | tee -a "${LOG_FILE}"
    fi
}
info()    { _log "${GREEN}[build_all]${NC} $*"; }
warn()    { _log "${YELLOW}[build_all]${NC} $*"; }
error()   { _log "${RED}[build_all ERROR]${NC} $*"; }
section() { _log "\n${CYAN}${BOLD}── $* ──${NC}\n"; }

# ── Pause until the user presses Enter (keeps terminal window open) ────────────
_wait_for_user() {
    printf "\n${BOLD}Press [Enter] to close...${NC}\n"
    read -r -p "" 2>/dev/null || true
}

# ── Defaults ──────────────────────────────────────────────────────────────────
DO_DEBUG=true
DO_RELEASE=true
DO_CLEAN=false
JOBS="$(nproc 2>/dev/null || echo 4)"
START_SECS="${SECONDS}"

GIT_HTTP_TIMEOUT=60
GIT_LOW_SPEED_LIMIT=1000
GIT_LOW_SPEED_TIME=30

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug-only)   DO_RELEASE=false ;;
        --release-only) DO_DEBUG=false   ;;
        --clean)        DO_CLEAN=true    ;;
        --jobs)         JOBS="$2"; shift ;;
        --help|-h)
            cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Builds the entire MasterRepo project (all phases) and writes logs.

Options:
  --debug-only      Build Debug configuration only
  --release-only    Build Release configuration only
  --clean           Remove existing build directories before building
  --jobs N          Parallel compile jobs (default: nproc = ${JOBS})
  --help            Show this message

Output:
  Binaries  →  ${BUILD_DEBUG}/   ${BUILD_RELEASE}/
  Log       →  Logs/Build/build_all_<timestamp>.log
  Report    →  Logs/Build/build_all_<timestamp>_report.md
EOF
            exit 0 ;;
        *) warn "Unknown option: $1" ;;
    esac
    shift
done

# ── Prepare log dir ───────────────────────────────────────────────────────────
mkdir -p "${LOG_DIR}"
: > "${LOG_FILE}"

# ── Stage count (configure + build per config) ────────────────────────────────
_STAGE_TOTAL=0
[[ "${DO_DEBUG}"   == "true" ]] && _STAGE_TOTAL=$(( _STAGE_TOTAL + 2 ))
[[ "${DO_RELEASE}" == "true" ]] && _STAGE_TOTAL=$(( _STAGE_TOTAL + 2 ))

if [[ "${_STAGE_TOTAL}" -eq 0 ]]; then
    warn "Nothing to build (both --debug-only and --release-only were set)."
    exit 0
fi

# ── Time cache — per-stage seconds from the last successful run ────────────────
_CACHE_FILE="${REPO_ROOT}/Logs/Build/.build_time_cache"
_TC_debug_conf=0; _TC_debug_build=0; _TC_release_conf=0; _TC_release_build=0

_load_time_cache() {
    [[ -f "${_CACHE_FILE}" ]] || return 0
    local _k _v
    while IFS='=' read -r _k _v; do
        _k="${_k//[[:space:]]/}"; _v="${_v//[[:space:]]/}"
        [[ "${_k}" == \#* || -z "${_k}" ]] && continue
        case "${_k}" in
            debug_conf)    _TC_debug_conf="${_v}"    ;;
            debug_build)   _TC_debug_build="${_v}"   ;;
            release_conf)  _TC_release_conf="${_v}"  ;;
            release_build) _TC_release_build="${_v}" ;;
        esac
    done < "${_CACHE_FILE}"
}

_save_time_cache() {
    {   printf "# MasterRepo build time cache — %s\n" "$(date '+%Y-%m-%d %H:%M:%S')"
        printf "debug_conf=%d\ndebug_build=%d\nrelease_conf=%d\nrelease_build=%d\n" \
            "${_TC_debug_conf}" "${_TC_debug_build}" \
            "${_TC_release_conf}" "${_TC_release_build}"
    } > "${_CACHE_FILE}" 2>/dev/null || true
}

_load_time_cache

# ══════════════════════════════════════════════════════════════════════════════
# ANIMATED DISPLAY
# ══════════════════════════════════════════════════════════════════════════════
#
# Layout (DISPLAY_TOTAL_LINES = 7 + number_of_stages):
#   Line 1  :  ══════ border ══════
#   Line 2  :  ✦  A t l a s F o r g e  ✦   MasterRepo Build
#   Line 3  :  ══════ border ══════
#   Line 4  :  (blank)
#   Lines 5…:  one row per stage  (spinner / ✅ / ❌ / · )
#   …+1     :  (blank)
#   …+2     :  [████░░░░]  N/M stages   Total: T   ~R remaining
#   …+3     :  (blank)
#
# After every draw the cursor is returned to line 1 so the next draw
# overwrites this block in place — giving the "locked" effect.
#
# ── Logo wave animation ────────────────────────────────────────────────────────
# 10-frame cycle, one frame per letter offset, so all 10 letters of "AtlasForge"
# are simultaneously at different phases — a continuous rolling wave.
#
#   Frame 0 : BRIGHT  upright    (peak normal)
#   Frame 1 : WHITE   upright
#   Frame 2 : DIM     upright    (starting to tip forward)
#   Frame 3 : DIM     "─"        (edge-on)
#   Frame 4 : DIM     flipped    (just appeared upside-down)
#   Frame 5 : WHITE   flipped
#   Frame 6 : BRIGHT  flipped    (peak flipped)
#   Frame 7 : WHITE   flipped    (starting to tip back)
#   Frame 8 : DIM     flipped
#   Frame 9 : DIM     "─"        (edge-on, returning)

_LOGO_N=( "A"  "t"  "l"  "a"  "s"  "F"  "o"  "r"  "g"  "e"  )
_LOGO_F=( "∀"  "ʇ"  "l"  "ɐ"  "s"  "Ⅎ"  "o"  "ɹ"  "ᵷ"  "ǝ"  )
_LOGO_E="─"   # the "edge-on" glyph shown at frames 3 and 9
_ANIM_TICK=0  # incremented on every redraw
_LC=""        # output variable used by _logo_char (avoids subshell capture)

# Set _LC to the coloured string for logo character IDX at animation PHASE.
_logo_char() {
    local idx="$1" phase="$2"
    local n="${_LOGO_N[$idx]}" f="${_LOGO_F[$idx]}"
    case "${phase}" in
        0) _LC="${BWHITE}${n}${NC}" ;;
        1) _LC="${WHITE}${n}${NC}"  ;;
        2) _LC="${DIM}${n}${NC}"    ;;
        3) _LC="${DIM}${_LOGO_E}${NC}" ;;
        4) _LC="${DIM}${f}${NC}"    ;;
        5) _LC="${WHITE}${f}${NC}"  ;;
        6) _LC="${BWHITE}${f}${NC}" ;;
        7) _LC="${WHITE}${f}${NC}"  ;;
        8) _LC="${DIM}${f}${NC}"    ;;
        9) _LC="${DIM}${_LOGO_E}${NC}" ;;
    esac
}

# Print the 3-line animated header block.
_draw_header() {
    local i logo_str=""
    for (( i=0; i<10; i++ )); do
        _logo_char "${i}" $(( (_ANIM_TICK + i) % 10 ))
        logo_str+="${_LC} "
    done
    (( _ANIM_TICK++ )) || true

    local bd="${CYAN}${BOLD}══════════════════════════════════════════════════════════${NC}"
    printf "  %b\e[K\n"  "${bd}"
    printf "  ${CYAN}✦${NC}  %b ${CYAN}✦${NC}  ${BOLD}MasterRepo Build${NC}\e[K\n" "${logo_str}"
    printf "  %b\e[K\n"  "${bd}"
}

# ── Full status display ────────────────────────────────────────────────────────
_SPIN_CHARS=$'/-\\|'   # spinner frames: / - \ |
_STAGE_DONE=0
_SPIN_IDX=0
_STAGE_LABELS=()
_STAGE_STATES=()    # "pending" | "active" | "done" | "failed"
_STAGE_ELAPSED=()   # seconds taken, filled on completion
DISPLAY_TOTAL_LINES=0

_init_display() {
    _STAGE_LABELS=()
    [[ "${DO_DEBUG}"   == "true" ]] && \
        _STAGE_LABELS+=( "Debug   ›  Configure" "Debug   ›  Build" )
    [[ "${DO_RELEASE}" == "true" ]] && \
        _STAGE_LABELS+=( "Release ›  Configure" "Release ›  Build" )

    local n="${#_STAGE_LABELS[@]}" i
    _STAGE_STATES=(); _STAGE_ELAPSED=()
    for (( i=0; i<n; i++ )); do
        _STAGE_STATES+=("pending")
        _STAGE_ELAPSED+=("0")
    done

    # 3 (header) + 1 (blank) + n (stage rows) + 1 (blank) + 1 (bar) + 1 (blank)
    DISPLAY_TOTAL_LINES=$(( 7 + n ))
    _STATUS_ACTIVE=true
    _draw_display 0 0   # initial paint — parks cursor at line 1
}

# Draw all display lines then return cursor to line 1 of the block.
_draw_display() {
    local step_start="$1"   # SECONDS when active stage started (0 = none)
    local step_est="$2"     # estimated seconds for active stage  (0 = unknown)
    local now="${SECONDS}"
    local tot_el=$(( now - START_SECS ))

    local sc="${_SPIN_CHARS:$(( _SPIN_IDX % 4 )):1}"
    (( _SPIN_IDX++ )) || true

    # ── Header (3 lines) ──────────────────────────────────────────────────────
    _draw_header

    # ── Blank separator ────────────────────────────────────────────────────────
    printf "\e[K\n"

    # ── Stage rows ────────────────────────────────────────────────────────────
    local i n="${#_STAGE_LABELS[@]}"
    for (( i=0; i<n; i++ )); do
        local state="${_STAGE_STATES[$i]}"
        local label="${_STAGE_LABELS[$i]}"
        local el="${_STAGE_ELAPSED[$i]}"
        local icon ts lc le

        case "${state}" in
            done)
                icon="✅"; lc="${GREEN}"; le="${NC}"
                printf -v ts "%d:%02d" $(( el/60 )) $(( el%60 ))
                ;;
            failed)
                icon="❌"; lc="${RED}"; le="${NC}"
                printf -v ts "%d:%02d" $(( el/60 )) $(( el%60 ))
                ;;
            active)
                icon="${YELLOW}${sc}${NC}"; lc="${BOLD}"; le="${NC}"
                local cur_el=$(( (step_start > 0) ? now - step_start : 0 ))
                printf -v ts "%d:%02d" $(( cur_el/60 )) $(( cur_el%60 ))
                ;;
            *)  # pending
                icon="${DIM}·${NC}"; lc="${DIM}"; le="${NC}"
                ts="----"
                ;;
        esac
        printf "  %b  ${lc}%-40s${le}  %s\e[K\n" "${icon}" "${label}" "${ts}"
    done

    # ── Blank separator ────────────────────────────────────────────────────────
    printf "\e[K\n"

    # ── Overall progress bar ──────────────────────────────────────────────────
    local bw=32 fill
    fill=$(( _STAGE_DONE * bw / _STAGE_TOTAL ))
    # Add intra-stage fraction when an estimate is available
    if [[ "${step_est}" -gt 0 && "${step_start}" -gt 0 ]]; then
        local cur_el=$(( now - step_start ))
        local sw=$(( bw / _STAGE_TOTAL ))
        local sf=$(( cur_el * sw / step_est ))
        fill=$(( fill + sf ))
    fi
    [[ "${fill}" -gt "${bw}" ]] && fill="${bw}"

    local bar="" j
    for (( j=0; j<fill;  j++ )); do bar+="█"; done
    for (( j=fill; j<bw; j++ )); do bar+="░"; done

    local te; printf -v te "%d:%02d" $(( tot_el/60 )) $(( tot_el%60 ))

    # Total estimate (computed inline — no subshell)
    local total_est=0
    [[ "${DO_DEBUG}"   == "true" ]] && \
        total_est=$(( total_est + _TC_debug_conf   + _TC_debug_build   ))
    [[ "${DO_RELEASE}" == "true" ]] && \
        total_est=$(( total_est + _TC_release_conf + _TC_release_build ))

    local est_str="calculating..."
    if [[ "${total_est}" -gt 0 ]]; then
        local rem=$(( total_est - tot_el ))
        [[ "${rem}" -lt 0 ]] && rem=0
        printf -v est_str "~%d:%02d remaining" $(( rem/60 )) $(( rem%60 ))
    fi

    printf "  [${GREEN}%s${NC}]  %d/%d stages   Total: ${BOLD}%s${NC}   ${YELLOW}%s${NC}\e[K\n" \
        "${bar}" "${_STAGE_DONE}" "${_STAGE_TOTAL}" "${te}" "${est_str}"

    # ── Blank line ────────────────────────────────────────────────────────────
    printf "\e[K\n"

    # Return cursor to line 1 of this display block
    printf "\e[%dA\r" "${DISPLAY_TOTAL_LINES}"
}

# Erase the display block and restore normal terminal output.
_finalize_display() {
    [[ "${_STATUS_ACTIVE}" == "true" ]] || return 0
    # Cursor is at line 1 of the block; erase from here to end of screen.
    printf "\e[J"
    _STATUS_ACTIVE=false
}

# Clean up on Ctrl-C / SIGTERM
trap '_finalize_display 2>/dev/null; _wait_for_user; exit 130' INT TERM

# ── Run cmake in the background with the animated display ─────────────────────
# cmake output goes directly to a temp file — NO pipe.  This means MSBuild
# worker-node processes cannot hold a pipe write-handle open after the build
# finishes, which was the root cause of the indefinite hang.
#
# Usage: _cmake_bg  STAGE_IDX  PHASE_LOG  STEP_EST_SECS  cmake-args...
_cmake_bg() {
    local sidx="$1" plog="$2" sest="$3"
    shift 3
    local t0="${SECONDS}"
    _STAGE_STATES[$sidx]="active"

    # Launch cmake — output appends to the phase log, not a pipe.
    # A bash zombie (process finished but not yet waited) still answers
    # kill -0 with success, so the loop below exits naturally when cmake
    # finishes.  We always call wait "${_pid}" after the loop, which
    # atomically collects the exit code and reaps the zombie.
    "$@" >> "${plog}" 2>&1 &
    local _pid=$!

    while kill -0 "${_pid}" 2>/dev/null; do
        _draw_display "${t0}" "${sest}"
        sleep 0.5 2>/dev/null || true
    done

    local _rc=0
    wait "${_pid}" || _rc=$?

    local elapsed=$(( SECONDS - t0 ))
    _STAGE_ELAPSED[$sidx]="${elapsed}"
    [[ "${_rc}" -eq 0 ]] && _STAGE_STATES[$sidx]="done" \
                         || _STAGE_STATES[$sidx]="failed"

    # Final redraw so completion state is visible briefly
    _draw_display "${t0}" "${sest}"
    sleep 0.1 2>/dev/null || true

    # Flush phase output into the master log
    cat "${plog}" >> "${LOG_FILE}" 2>/dev/null || true

    return "${_rc}"
}

# ── Build tracking ─────────────────────────────────────────────────────────────
PASS=(); FAIL=()
declare -A PHASE_ERRORS

run_phase() {
    local label="$1" build_dir="$2" build_type="$3"
    local plog; plog="$(mktemp)"

    # Map build type → stage indices and time-cache variable names
    local cidx bidx ck bk cest best
    case "${build_type,,}" in
        debug)
            cidx=0; bidx=1
            ck="_TC_debug_conf";    cest="${_TC_debug_conf}"
            bk="_TC_debug_build";   best="${_TC_debug_build}"
            ;;
        release)
            if [[ "${DO_DEBUG}" == "true" ]]; then
                cidx=2; bidx=3
            else
                cidx=0; bidx=1
            fi
            ck="_TC_release_conf";  cest="${_TC_release_conf}"
            bk="_TC_release_build"; best="${_TC_release_build}"
            ;;
    esac

    local ok=true t0

    # ── Configure ─────────────────────────────────────────────────────────────
    t0="${SECONDS}"
    if _cmake_bg "${cidx}" "${plog}" "${cest}" \
        cmake -S "${REPO_ROOT}" \
              -B "${build_dir}" \
              -DCMAKE_BUILD_TYPE="${build_type}" \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DBUILD_EDITOR=ON \
              -DBUILD_RUNTIME=ON \
              -DBUILD_AI=ON \
              -DBUILD_TOOLS=ON \
              -DFETCHCONTENT_QUIET=OFF; then
        printf -v "${ck}" '%d' "$(( SECONDS - t0 ))"
        (( _STAGE_DONE++ )) || true
    else
        ok=false
    fi

    # ── Build ─────────────────────────────────────────────────────────────────
    if [[ "${ok}" == "true" ]]; then
        t0="${SECONDS}"
        : > "${plog}"
        if _cmake_bg "${bidx}" "${plog}" "${best}" \
            cmake --build "${build_dir}" \
                  --config "${build_type}" \
                  --parallel "${JOBS}"; then
            printf -v "${bk}" '%d' "$(( SECONDS - t0 ))"
            (( _STAGE_DONE++ )) || true
        else
            ok=false
        fi
    fi

    # Extract error lines for the Markdown report
    PHASE_ERRORS["${label}"]=$(
        grep -iE "(error[^(]*:|FAILED|fatal error)" "${plog}" \
            | grep -v " 0 error" \
            | head -60 \
        || true
    )
    rm -f "${plog}"

    if [[ "${ok}" == "true" ]]; then
        info "✅ ${label} complete."
        PASS+=("${label}")
    else
        error "❌ ${label} FAILED."
        FAIL+=("${label}")
    fi
}

# ══════════════════════════════════════════════════════════════════════════════
# Main — environment, dependencies, build
# ══════════════════════════════════════════════════════════════════════════════
section "MasterRepo — Build All  [${TIMESTAMP}]"
info "Repo root  : ${REPO_ROOT}"
info "Debug dir  : ${BUILD_DEBUG}"
info "Release dir: ${BUILD_RELEASE}"
info "Log file   : ${LOG_FILE}"
info "Jobs       : ${JOBS}"
printf "\n"

# ── Environment check ─────────────────────────────────────────────────────────
section "Environment"

check_cmd() {
    local cmd="$1" label="${2:-$1}"
    if command -v "${cmd}" &>/dev/null; then
        local _out _ver
        _out="$("${cmd}" --version 2>&1 || true)"
        _ver="${_out%%$'\n'*}"; _ver="${_ver%%$'\r'*}"
        [[ -z "${_ver}" ]] && _ver="(version unknown)"
        info "  ${label}: ${_ver}"
    else
        error "  ${label}: NOT FOUND (required)"
        return 1
    fi
}

check_cmd cmake
if command -v git &>/dev/null; then
    check_cmd git
else
    warn "  git: NOT FOUND — FetchContent (GLFW source download) will not work."
    warn "  Install git from https://git-scm.com and re-run."
fi

# shellcheck source=Scripts/Tools/detect_compiler.sh
source "$(dirname "${BASH_SOURCE[0]}")/detect_compiler.sh" || true
if [[ "${CXX_FOUND}" == "true" ]]; then
    info "  c++ compiler: ${CXX_NAME}"
else
    warn "  No C++ compiler found in PATH — CMake will attempt its own detection."
    warn "  Configure may fail; see instructions printed above."
fi

# ── Dependency pre-install ────────────────────────────────────────────────────
section "Dependencies"

if command -v pacman &>/dev/null; then
    info "  Detected MSYS2 pacman — installing build dependencies…"
    pacman -S --noconfirm --needed \
        mingw-w64-ucrt-x86_64-cmake \
        mingw-w64-ucrt-x86_64-gcc \
        mingw-w64-ucrt-x86_64-glfw \
        git \
        2>&1 | tee -a "${LOG_FILE}" || \
    warn "  pacman install failed (packages may already be present)."
else
    info "  Git Bash (non-MSYS2): GLFW will be downloaded via CMake FetchContent."
    info "  Ensure git is installed (https://git-scm.com) so FetchContent can clone GLFW."
fi

# ── Git network timeouts (prevents FetchContent stall) ────────────────────────
if command -v git &>/dev/null; then
    info "  Configuring git network timeouts…"
    git -C "${REPO_ROOT}" config http.timeout       "${GIT_HTTP_TIMEOUT}"    2>/dev/null || true
    git -C "${REPO_ROOT}" config http.lowSpeedLimit "${GIT_LOW_SPEED_LIMIT}" 2>/dev/null || true
    git -C "${REPO_ROOT}" config http.lowSpeedTime  "${GIT_LOW_SPEED_TIME}"  2>/dev/null || true
fi

# ── Clean ─────────────────────────────────────────────────────────────────────
if [[ "${DO_CLEAN}" == "true" ]]; then
    section "Clean"
    if [[ "${DO_DEBUG}" == "true" && -d "${BUILD_DEBUG}" ]]; then
        warn "  Removing ${BUILD_DEBUG}"
        rm -rf "${BUILD_DEBUG}"
    fi
    if [[ "${DO_RELEASE}" == "true" && -d "${BUILD_RELEASE}" ]]; then
        warn "  Removing ${BUILD_RELEASE}"
        rm -rf "${BUILD_RELEASE}"
    fi
    info "  Clean done."
fi

# ── Start animated build display ──────────────────────────────────────────────
# From this point _log() only appends to the log file.  The terminal shows
# only the animated header + stage list + progress bar, redrawn every ~0.5 s.
printf "\n"
_init_display

# ── Builds ────────────────────────────────────────────────────────────────────
[[ "${DO_DEBUG}"   == "true" ]] && run_phase "Debug"   "${BUILD_DEBUG}"   "Debug"
[[ "${DO_RELEASE}" == "true" ]] && run_phase "Release" "${BUILD_RELEASE}" "Release"

# ── Persist timing data for the next run's estimates ──────────────────────────
_save_time_cache

# ── Restore normal terminal output ────────────────────────────────────────────
_finalize_display

# ── Symlink / copy compile_commands.json to repo root for IDE tooling ─────────
if [[ -f "${BUILD_DEBUG}/compile_commands.json" ]]; then
    ln -sf "${BUILD_DEBUG}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || \
    cp     "${BUILD_DEBUG}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || true
    info "compile_commands.json → ${BUILD_DEBUG}"
elif [[ -f "${BUILD_RELEASE}/compile_commands.json" ]]; then
    ln -sf "${BUILD_RELEASE}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || \
    cp     "${BUILD_RELEASE}/compile_commands.json" \
           "${REPO_ROOT}/compile_commands.json" 2>/dev/null || true
    info "compile_commands.json → ${BUILD_RELEASE}"
fi

# ── Build report ──────────────────────────────────────────────────────────────
ELAPSED=$(( SECONDS - START_SECS ))
section "Build Summary"

{
    printf "# MasterRepo — Build All Report\n\n"
    printf "**Date:**      %s\n"   "$(date '+%Y-%m-%d %H:%M:%S')"
    printf "**Elapsed:**   %ds\n"  "${ELAPSED}"
    printf "**Repo:**      %s\n"   "${REPO_ROOT}"
    printf "**Jobs:**      %s\n\n" "${JOBS}"
    printf "## Results\n\n"
    printf "| Configuration | Result |\n|---------------|--------|\n"
    _has_rows=false
    for s in "${PASS[@]:-}"; do
        [[ -n "${s}" ]] && printf "| %s | ✅ Pass |\n" "${s}" && _has_rows=true
    done
    for s in "${FAIL[@]:-}"; do
        [[ -n "${s}" ]] && printf "| %s | ❌ Fail |\n" "${s}" && _has_rows=true
    done
    [[ "${_has_rows}" == "false" ]] && printf "| (no builds run) | — |\n"
    printf "\n"

    _any_errors=false
    for s in "${FAIL[@]:-}"; do [[ -n "${s}" ]] && _any_errors=true; done
    if [[ "${_any_errors}" == "true" ]]; then
        printf "## Errors\n\n"
        for s in "${FAIL[@]:-}"; do
            [[ -n "${s}" ]] || continue
            printf "### %s\n\`\`\`\n" "${s}"
            if [[ -n "${PHASE_ERRORS[${s}]:-}" ]]; then
                printf "%s\n" "${PHASE_ERRORS[${s}]}"
            else
                printf "(no error lines captured — see full log)\n"
            fi
            printf "\`\`\`\n\n"
        done
    fi

    printf "## Binaries\n\n"
    if [[ "${DO_DEBUG}" == "true" && -d "${BUILD_DEBUG}/bin" ]]; then
        printf "### Debug\n\`\`\`\n"
        ls "${BUILD_DEBUG}/bin/" 2>/dev/null || printf "(none)\n"
        printf "\`\`\`\n"
    fi
    if [[ "${DO_RELEASE}" == "true" && -d "${BUILD_RELEASE}/bin" ]]; then
        printf "### Release\n\`\`\`\n"
        ls "${BUILD_RELEASE}/bin/" 2>/dev/null || printf "(none)\n"
        printf "\`\`\`\n"
    fi

    printf "\n## Log Files\n\n"
    printf "- Full log: \`%s\`\n" "${LOG_FILE}"
    printf "- Report:   \`%s\`\n" "${REPORT_FILE}"
} | tee -a "${LOG_FILE}" > "${REPORT_FILE}"

# ── Terminal summary ───────────────────────────────────────────────────────────
printf "\n${BOLD}═══════════════════════════════════════════${NC}\n"
printf "${BOLD}  MasterRepo Build All — %ds${NC}\n" "${ELAPSED}"
printf "  ✅ Passed : %d\n" "${#PASS[@]}"
printf "  ❌ Failed : %d\n" "${#FAIL[@]}"
printf "${BOLD}═══════════════════════════════════════════${NC}\n"
printf "  Log    : %s\n" "${LOG_FILE}"
printf "  Report : %s\n\n" "${REPORT_FILE}"

if [[ ${#FAIL[@]} -gt 0 ]]; then
    printf "%b\n" "${RED}[build_all ERROR]${NC} Build had failures. Check the log above."
    _wait_for_user
    exit 1
fi

_wait_for_user
exit 0
