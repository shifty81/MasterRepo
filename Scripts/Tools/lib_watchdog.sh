#!/usr/bin/env bash
# =============================================================================
# lib_watchdog.sh — Build Hang / Stall Detection for MasterRepo
#
# Source this file to get watchdog functionality that monitors a build process
# and generates a debug report if no activity is detected for a configurable
# timeout period.
#
# ── Hang vectors this library guards against ──────────────────────────────────
#
#   1. MSBuild worker nodes keeping a PIPE write-handle open:
#        Fixed upstream in _cmake_run / _cmake_bg (no pipe to cmake).
#        Watchdog is a second line of defence for any future regressions.
#
#   2. Watchdog background process never terminating:
#        Fixed by using a sentinel file instead of a signal to stop the loop.
#        watchdog_stop() writes the sentinel; the loop polls for it.
#        This avoids the 'kill + disown + wait' race that caused hangs in
#        earlier versions when 'wait "${_WD_PID}"' after 'disown' returned
#        undefined results in different bash versions.
#
#   3. Diagnostic report commands blocking:
#        All commands in _watchdog_report are time-boxed with 'timeout' (GNU
#        coreutils) where available, or skipped gracefully when not.
#
# API:
#   watchdog_start  <log_file> [timeout_s] [build_pid]
#   watchdog_stop
#
# Environment overrides:
#   WATCHDOG_TIMEOUT   — seconds of inactivity before hang detected (default: 600)
#   WATCHDOG_ACTION    — "report" | "report+kill" | "report+issue" (default: report)
#   WATCHDOG_ENABLED   — "true" | "false"  (default: true)
# =============================================================================

# Guard against double-sourcing
[[ -n "${_LIB_WATCHDOG_LOADED:-}" ]] && return 0
_LIB_WATCHDOG_LOADED=1

# ── Defaults ──────────────────────────────────────────────────────────────────
WATCHDOG_TIMEOUT="${WATCHDOG_TIMEOUT:-600}"
WATCHDOG_ACTION="${WATCHDOG_ACTION:-report}"
WATCHDOG_ENABLED="${WATCHDOG_ENABLED:-true}"

# ── Resolve paths ─────────────────────────────────────────────────────────────
_WD_REPO_ROOT="${_WD_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}"
_WD_LOG_DIR="${_WD_REPO_ROOT}/Logs/Build"
_WD_PID=""          # PID of the background watchdog subshell
_WD_SENTINEL=""     # Path to the sentinel file used to signal the loop to stop
_WD_BUILD_PID=""    # PID of the cmake/MSBuild process being watched

# ── Detect GNU coreutils timeout (once, at source time) ───────────────────────
# Windows has a TIMEOUT.EXE that ignores the COMMAND argument and exits 0.
# GNU coreutils timeout exits 124 when the timer fires — use that to tell them
# apart.  We redirect stdin from /dev/null so TIMEOUT.EXE does not block
# waiting for a keypress when stdin is not a terminal.
#
# IMPORTANT: this file is sourced by scripts that run with 'set -e'.  We MUST
# capture the timeout exit code via '|| _to_ec=$?' so that the expected
# non-zero exit (124) does not abort the calling script through set -e.
_WD_TIMEOUT_CMD=""
if command -v timeout &>/dev/null; then
    _to_ec=0
    timeout 0.1 sleep 1 </dev/null 2>/dev/null || _to_ec=$?
    [[ "${_to_ec}" -eq 124 ]] && _WD_TIMEOUT_CMD="timeout"
    unset _to_ec
fi

# ── _wd_safe_run: run a diagnostic command with an optional time limit ─────────
# Usage: _wd_safe_run [limit_s] cmd [args...]
# Silently skips if the command is not available.  Time limit only applied when
# GNU coreutils timeout is present.
_wd_safe_run() {
    local limit="$1"; shift
    local cmd="$1"
    command -v "${cmd}" &>/dev/null || return 0
    if [[ -n "${_WD_TIMEOUT_CMD}" && "${limit}" -gt 0 ]]; then
        "${_WD_TIMEOUT_CMD}" "${limit}" "$@" 2>/dev/null || true
    else
        "$@" 2>/dev/null || true
    fi
}

# ── Start the watchdog ────────────────────────────────────────────────────────
# Args: <log_file_to_monitor> [timeout_seconds] [build_pid]
watchdog_start() {
    [[ "${WATCHDOG_ENABLED}" != "true" ]] && return 0

    local monitor_file="${1:-}"
    local timeout="${2:-${WATCHDOG_TIMEOUT}}"
    _WD_BUILD_PID="${3:-}"

    [[ -z "${monitor_file}" ]] && return 0

    mkdir -p "${_WD_LOG_DIR}"

    # Create a unique sentinel file.  When watchdog_stop() is called it writes
    # to this file; the monitor loop polls for its existence and exits cleanly.
    _WD_SENTINEL="$(mktemp "${_WD_LOG_DIR}/.watchdog_sentinel_XXXXXXXX" 2>/dev/null \
                    || echo "${_WD_LOG_DIR}/.watchdog_sentinel_$$")"

    # Launch the monitor in a background subshell.  We do NOT call disown so
    # that 'wait "${_WD_PID}"' in watchdog_stop() works correctly and collects
    # the exit status, reaping the zombie.
    (
        _watchdog_loop "${monitor_file}" "${timeout}" "${_WD_BUILD_PID}" "${_WD_SENTINEL}"
    ) &
    _WD_PID=$!
}

# ── Stop the watchdog ─────────────────────────────────────────────────────────
watchdog_stop() {
    # Signal the loop to exit by writing to the sentinel file.
    if [[ -n "${_WD_SENTINEL}" ]]; then
        echo "stop" > "${_WD_SENTINEL}" 2>/dev/null || true
    fi

    # Give the loop up to 3 seconds to notice the sentinel and exit gracefully.
    if [[ -n "${_WD_PID}" ]]; then
        local deadline=$(( SECONDS + 3 ))
        while kill -0 "${_WD_PID}" 2>/dev/null && [[ "${SECONDS}" -lt "${deadline}" ]]; do
            sleep 0.2 2>/dev/null || true
        done
        # If it is still running, send SIGTERM to ensure it exits.
        kill "${_WD_PID}" 2>/dev/null || true
        wait "${_WD_PID}" 2>/dev/null || true
        _WD_PID=""
    fi

    # Clean up sentinel file.
    if [[ -n "${_WD_SENTINEL}" && -f "${_WD_SENTINEL}" ]]; then
        rm -f "${_WD_SENTINEL}" 2>/dev/null || true
        _WD_SENTINEL=""
    fi
}

# ── The monitoring loop (runs in background subshell) ─────────────────────────
_watchdog_loop() {
    local monitor_file="$1"
    local timeout="$2"
    local build_pid="$3"
    local sentinel="$4"
    local last_size=0
    local stall_start=""
    local check_interval=10   # poll every 10 s

    while true; do
        # ── Exit conditions ────────────────────────────────────────────────────
        # 1. Sentinel file has been written by watchdog_stop().
        if [[ -s "${sentinel}" ]]; then
            break
        fi
        # 2. The build process has finished — watchdog no longer needed.
        if [[ -n "${build_pid}" ]] && ! kill -0 "${build_pid}" 2>/dev/null; then
            break
        fi

        sleep "${check_interval}" 2>/dev/null || break

        # Re-check sentinel after sleep (avoids an extra poll cycle).
        [[ -s "${sentinel}" ]] && break

        # ── Activity check ─────────────────────────────────────────────────────
        local current_size=0
        if [[ -f "${monitor_file}" ]]; then
            current_size="$(wc -c < "${monitor_file}" 2>/dev/null || echo 0)"
        fi

        if [[ "${current_size}" -gt "${last_size}" ]]; then
            last_size="${current_size}"
            stall_start=""
        else
            [[ -z "${stall_start}" ]] && stall_start="${SECONDS}"
            local stall_duration=$(( SECONDS - stall_start ))

            if [[ "${stall_duration}" -ge "${timeout}" ]]; then
                _watchdog_report "${monitor_file}" "${stall_duration}" "${build_pid}"

                case "${WATCHDOG_ACTION}" in
                    report+kill)
                        [[ -n "${build_pid}" ]] && kill "${build_pid}" 2>/dev/null || true
                        ;;
                    report+issue)
                        _watchdog_submit_issue
                        ;;
                esac

                # Reset stall timer so we don't spam reports.
                stall_start="${SECONDS}"
            fi
        fi
    done

    # Clean up our sentinel copy.
    rm -f "${sentinel}" 2>/dev/null || true
}

# ── Generate hang diagnostic report ──────────────────────────────────────────
_watchdog_report() {
    local monitor_file="$1"
    local stall_seconds="$2"
    local build_pid="$3"
    local ts
    ts="$(date +%Y%m%d_%H%M%S 2>/dev/null || echo "unknown")"
    local report="${_WD_LOG_DIR}/hang_report_${ts}.md"

    {
        echo "# Build Hang Detected — Watchdog Report"
        echo ""
        echo "**Generated:** $(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null || echo unknown)"
        echo "**Stall duration:** ${stall_seconds}s (timeout: ${WATCHDOG_TIMEOUT}s)"
        echo "**Monitored file:** \`${monitor_file}\`"
        echo "**Build PID:** ${build_pid:-N/A}"
        echo "**Host:** $(hostname 2>/dev/null || echo unknown)"
        echo ""
        echo "## Last 100 Lines of Build Log"
        echo ""
        echo '```'
        if [[ -f "${monitor_file}" ]]; then
            tail -100 "${monitor_file}" 2>/dev/null || echo "(could not read log)"
        else
            echo "(log file not found)"
        fi
        echo '```'
        echo ""
        echo "## Running Processes (top 20 by CPU)"
        echo ""
        echo '```'
        _wd_safe_run 5 ps aux --sort=-%cpu | head -20
        echo '```'
        echo ""
        echo "## Disk Usage"
        echo ""
        echo '```'
        _wd_safe_run 5 df -h .
        echo '```'
        echo ""
        echo "## Memory"
        echo ""
        echo '```'
        if [[ -f /proc/meminfo ]]; then
            head -10 /proc/meminfo 2>/dev/null || true
        else
            _wd_safe_run 5 free -h
        fi
        echo '```'
        echo ""
        echo "## Environment (filtered)"
        echo ""
        echo '```'
        env 2>/dev/null | grep -iE '^(PATH|CXX|CC|CMAKE|MSYS|MINGW|VS|OS|COMSPEC|JOBS|BUILD)' | sort 2>/dev/null || true
        echo '```'
        echo ""
        echo "---"
        echo ""
        echo "To submit this report as a GitHub issue, run:"
        echo '```'
        echo "bash Scripts/Tools/submit_issue.sh --from-hang-report \"${report}\""
        echo '```'
    } > "${report}" 2>&1

    echo "" >&2
    echo "WATCHDOG: Build appears to be hanging (no output for ${stall_seconds}s)" >&2
    echo "WATCHDOG: Report written to: ${report}" >&2
    echo "" >&2
}

# ── Auto-submit issue from watchdog report ────────────────────────────────────
_watchdog_submit_issue() {
    local submit_script="${_WD_REPO_ROOT}/Scripts/Tools/submit_issue.sh"
    [[ -x "${submit_script}" ]] || return 0
    local report
    report="$(ls -t "${_WD_LOG_DIR}"/hang_report_*.md 2>/dev/null | head -1)"
    [[ -n "${report}" ]] && bash "${submit_script}" --from-hang-report "${report}" 2>/dev/null || true
}
