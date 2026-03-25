#!/usr/bin/env bash
# =============================================================================
# lib_watchdog.sh — Build Hang / Stall Detection for MasterRepo
#
# Source this file to get watchdog functionality that monitors a build process
# and generates a debug report if no activity is detected for a configurable
# timeout period.
#
# Usage:
#   source "$(dirname "${BASH_SOURCE[0]}")/lib_watchdog.sh"
#
#   # Start monitoring a log file for activity
#   watchdog_start "path/to/build.log" 300   # 300s = 5 min inactivity timeout
#
#   # ... run your build ...
#
#   # Stop the watchdog (call when build finishes)
#   watchdog_stop
#
# What happens on hang detection:
#   1. A diagnostic report is written to Logs/Build/hang_report_<ts>.md
#   2. System state is captured (processes, disk, memory)
#   3. The last N lines of the monitored log are included
#   4. If submit_issue.sh is available, it can auto-file a GitHub issue
#
# Environment overrides:
#   WATCHDOG_TIMEOUT   — seconds of inactivity before hang detected (default: 600)
#   WATCHDOG_ACTION    — "report" | "report+kill" | "report+issue" (default: report)
#   WATCHDOG_ENABLED   — true/false (default: true)
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
_WD_PID=""
_WD_MONITOR_FILE=""
_WD_BUILD_PID=""

# ── Start the watchdog ────────────────────────────────────────────────────────
# Args: <log_file_to_monitor> [timeout_seconds] [build_pid]
watchdog_start() {
    [[ "${WATCHDOG_ENABLED}" != "true" ]] && return 0

    _WD_MONITOR_FILE="${1:-}"
    local timeout="${2:-${WATCHDOG_TIMEOUT}}"
    _WD_BUILD_PID="${3:-}"

    [[ -z "${_WD_MONITOR_FILE}" ]] && return 0

    mkdir -p "${_WD_LOG_DIR}"

    # Launch the monitor as a background subshell
    (
        _watchdog_loop "${_WD_MONITOR_FILE}" "${timeout}" "${_WD_BUILD_PID}"
    ) &
    _WD_PID=$!
    disown "${_WD_PID}" 2>/dev/null || true
}

# ── Stop the watchdog ─────────────────────────────────────────────────────────
watchdog_stop() {
    if [[ -n "${_WD_PID}" ]]; then
        kill "${_WD_PID}" 2>/dev/null || true
        wait "${_WD_PID}" 2>/dev/null || true
        _WD_PID=""
    fi
}

# ── The monitoring loop (runs in background) ─────────────────────────────────
_watchdog_loop() {
    local monitor_file="$1"
    local timeout="$2"
    local build_pid="$3"
    local last_size=0
    local stall_start=""
    local check_interval=10  # check every 10 seconds

    while true; do
        sleep "${check_interval}" 2>/dev/null || break

        # If a build PID was given, check if it's still running
        if [[ -n "${build_pid}" ]] && ! kill -0 "${build_pid}" 2>/dev/null; then
            # Build process ended — watchdog no longer needed
            break
        fi

        # Check file size for activity
        local current_size=0
        if [[ -f "${monitor_file}" ]]; then
            current_size="$(wc -c < "${monitor_file}" 2>/dev/null || echo 0)"
        fi

        if [[ "${current_size}" -gt "${last_size}" ]]; then
            # Activity detected — reset stall timer
            last_size="${current_size}"
            stall_start=""
        else
            # No activity
            if [[ -z "${stall_start}" ]]; then
                stall_start="${SECONDS}"
            fi

            local stall_duration=$(( SECONDS - stall_start ))

            if [[ "${stall_duration}" -ge "${timeout}" ]]; then
                # HANG DETECTED — generate report
                _watchdog_report "${monitor_file}" "${stall_duration}" "${build_pid}"

                # Take action based on WATCHDOG_ACTION
                case "${WATCHDOG_ACTION}" in
                    report+kill)
                        if [[ -n "${build_pid}" ]]; then
                            kill "${build_pid}" 2>/dev/null || true
                        fi
                        ;;
                    report+issue)
                        _watchdog_submit_issue "${monitor_file}" "${stall_duration}"
                        ;;
                esac

                # Reset so we don't spam reports — wait another full timeout
                stall_start="${SECONDS}"
            fi
        fi
    done
}

# ── Generate hang diagnostic report ──────────────────────────────────────────
_watchdog_report() {
    local monitor_file="$1"
    local stall_seconds="$2"
    local build_pid="$3"
    local ts
    ts="$(date +%Y%m%d_%H%M%S)"
    local report="${_WD_LOG_DIR}/hang_report_${ts}.md"

    {
        echo "# 🚨 Build Hang Detected — Watchdog Report"
        echo ""
        echo "**Generated:** $(date '+%Y-%m-%d %H:%M:%S')"
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
        echo "## Running Processes"
        echo ""
        echo '```'
        ps aux --sort=-%cpu 2>/dev/null | head -30 || tasklist 2>/dev/null | head -30 || echo "(unavailable)"
        echo '```'
        echo ""
        echo "## Disk Usage"
        echo ""
        echo '```'
        df -h . 2>/dev/null || echo "(unavailable)"
        echo '```'
        echo ""
        echo "## Memory"
        echo ""
        echo '```'
        free -h 2>/dev/null || cat /proc/meminfo 2>/dev/null | head -10 || echo "(unavailable)"
        echo '```'
        echo ""
        echo "## Build PID Process Tree"
        echo ""
        echo '```'
        if [[ -n "${build_pid}" ]]; then
            pstree -p "${build_pid}" 2>/dev/null || \
            ps --forest -g "${build_pid}" 2>/dev/null || \
            echo "(process tree unavailable)"
        else
            echo "(no build PID provided)"
        fi
        echo '```'
        echo ""
        echo "## Environment (filtered)"
        echo ""
        echo '```'
        env | grep -iE '^(PATH|CXX|CC|CMAKE|MSYS|MINGW|VS|OS|COMSPEC|JOBS|BUILD)' 2>/dev/null | sort || true
        echo '```'
        echo ""
        echo "---"
        echo ""
        echo "To submit this report as a GitHub issue, run:"
        echo '```'
        echo "bash Scripts/Tools/submit_issue.sh --from-hang-report \"${report}\""
        echo '```'
    } > "${report}" 2>&1

    # Also echo to stderr so the user sees it even if console is redirected
    echo "" >&2
    echo "⚠️  WATCHDOG: Build appears to be hanging (no output for ${stall_seconds}s)" >&2
    echo "⚠️  Report written to: ${report}" >&2
    echo "" >&2
}

# ── Auto-submit issue from watchdog ──────────────────────────────────────────
_watchdog_submit_issue() {
    local monitor_file="$1"
    local stall_seconds="$2"
    local submit_script="${_WD_REPO_ROOT}/Scripts/Tools/submit_issue.sh"

    if [[ -x "${submit_script}" ]]; then
        local report
        report="$(ls -t "${_WD_LOG_DIR}"/hang_report_*.md 2>/dev/null | head -1)"
        if [[ -n "${report}" ]]; then
            bash "${submit_script}" --from-hang-report "${report}" 2>/dev/null || true
        fi
    fi
}
