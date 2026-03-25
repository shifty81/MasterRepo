#!/usr/bin/env bash
# =============================================================================
# lib_log.sh — Shared Logging Framework for MasterRepo
#
# Source this file from any script to get:
#   • Timestamped, leveled log messages (DEBUG, INFO, WARN, ERROR, FATAL)
#   • Dual output: console (coloured) + log file
#   • Automatic log file creation in Logs/Build/
#   • Log rotation (configurable retention)
#   • Elapsed-time tracking
#   • Structured context (script name, PID)
#
# Usage:
#   source "$(dirname "${BASH_SOURCE[0]}")/lib_log.sh"
#   log_init "my_script"          # creates Logs/Build/my_script_<ts>.log
#   log_info  "Starting build"
#   log_warn  "Something looks off"
#   log_error "Build failed"
#   log_fatal "Cannot continue"   # also exits 1
#   log_debug "Verbose detail"    # only when LOG_LEVEL=DEBUG
#   log_finish                    # prints summary, returns log path
#
# Environment overrides:
#   LOG_LEVEL       — DEBUG | INFO | WARN | ERROR  (default: INFO)
#   LOG_RETENTION   — days to keep old logs         (default: 30)
#   LOG_DIR         — override log directory
#   LOG_CONSOLE     — true/false  console output    (default: true)
#   LOG_TIMESTAMP   — true/false  prefix timestamps (default: true)
# =============================================================================

# Guard against double-sourcing
[[ -n "${_LIB_LOG_LOADED:-}" ]] && return 0
_LIB_LOG_LOADED=1

# ── Colours ───────────────────────────────────────────────────────────────────
_LOG_GREEN=$'\033[0;32m'
_LOG_YELLOW=$'\033[1;33m'
_LOG_RED=$'\033[0;31m'
_LOG_CYAN=$'\033[0;36m'
_LOG_BOLD=$'\033[1m'
_LOG_DIM=$'\033[2m'
_LOG_NC=$'\033[0m'

# ── Defaults ──────────────────────────────────────────────────────────────────
LOG_LEVEL="${LOG_LEVEL:-INFO}"
LOG_RETENTION="${LOG_RETENTION:-30}"
LOG_CONSOLE="${LOG_CONSOLE:-true}"
LOG_TIMESTAMP="${LOG_TIMESTAMP:-true}"

# ── Resolve repo root ────────────────────────────────────────────────────────
_LOG_REPO_ROOT="${_LOG_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}"
LOG_DIR="${LOG_DIR:-${_LOG_REPO_ROOT}/Logs/Build}"

# ── State ─────────────────────────────────────────────────────────────────────
_LOG_FILE=""
_LOG_SCRIPT_NAME=""
_LOG_START_SECONDS="${SECONDS}"
_LOG_ERROR_COUNT=0
_LOG_WARN_COUNT=0

# ── Level to numeric ─────────────────────────────────────────────────────────
_log_level_num() {
    case "${1^^}" in
        DEBUG) echo 0 ;;
        INFO)  echo 1 ;;
        WARN)  echo 2 ;;
        ERROR) echo 3 ;;
        FATAL) echo 4 ;;
        *)     echo 1 ;;
    esac
}

# ── Initialise logging ───────────────────────────────────────────────────────
# Call once at the start of your script.
#   log_init "script_name"
# Creates the log file and writes a header.
log_init() {
    _LOG_SCRIPT_NAME="${1:-$(basename "${BASH_SOURCE[1]:-unknown}" .sh)}"
    local ts
    ts="$(date +%Y%m%d_%H%M%S)"
    mkdir -p "${LOG_DIR}"
    _LOG_FILE="${LOG_DIR}/${_LOG_SCRIPT_NAME}_${ts}.log"
    : > "${_LOG_FILE}"

    _LOG_START_SECONDS="${SECONDS}"
    _LOG_ERROR_COUNT=0
    _LOG_WARN_COUNT=0

    # Header
    {
        echo "============================================================"
        echo "  MasterRepo Log — ${_LOG_SCRIPT_NAME}"
        echo "  Started: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "  PID:     $$"
        echo "  Host:    $(hostname 2>/dev/null || echo unknown)"
        echo "  OS:      $(uname -s 2>/dev/null || echo unknown)"
        echo "  User:    $(whoami 2>/dev/null || echo unknown)"
        echo "  Level:   ${LOG_LEVEL}"
        echo "============================================================"
        echo ""
    } >> "${_LOG_FILE}"

    # Rotate old logs
    _log_rotate
}

# ── Core write function ──────────────────────────────────────────────────────
_log_write() {
    local level="$1" colour="$2" msg="$3"

    # Check level threshold
    local current_num set_num
    current_num="$(_log_level_num "${level}")"
    set_num="$(_log_level_num "${LOG_LEVEL}")"
    (( current_num < set_num )) && return 0

    # Build timestamp prefix
    local ts_prefix=""
    if [[ "${LOG_TIMESTAMP}" == "true" ]]; then
        ts_prefix="$(date '+%H:%M:%S') "
    fi

    local file_line="${ts_prefix}[${level}] [${_LOG_SCRIPT_NAME}:$$] ${msg}"
    local console_line="${_LOG_DIM}${ts_prefix}${_LOG_NC}${colour}[${level}]${_LOG_NC} ${msg}"

    # Write to file
    if [[ -n "${_LOG_FILE}" ]]; then
        echo "${file_line}" >> "${_LOG_FILE}"
    fi

    # Write to console
    if [[ "${LOG_CONSOLE}" == "true" ]]; then
        if [[ "${level}" == "ERROR" || "${level}" == "FATAL" ]]; then
            echo -e "${console_line}" >&2
        else
            echo -e "${console_line}"
        fi
    fi
}

# ── Public logging functions ──────────────────────────────────────────────────
log_debug() { _log_write "DEBUG" "${_LOG_DIM}"    "$*"; }
log_info()  { _log_write "INFO"  "${_LOG_GREEN}"  "$*"; }
log_warn()  { _log_write "WARN"  "${_LOG_YELLOW}" "$*"; (( _LOG_WARN_COUNT++ )) || true; }
log_error() { _log_write "ERROR" "${_LOG_RED}"     "$*"; (( _LOG_ERROR_COUNT++ )) || true; }
log_fatal() { _log_write "FATAL" "${_LOG_RED}${_LOG_BOLD}" "$*"; (( _LOG_ERROR_COUNT++ )) || true; exit 1; }

# ── Section header ────────────────────────────────────────────────────────────
log_section() {
    local msg="$*"
    _log_write "INFO" "${_LOG_CYAN}${_LOG_BOLD}" "══ ${msg} ══"
}

# ── Log command execution with output capture ────────────────────────────────
# Usage: log_cmd "label" command arg1 arg2 ...
# Returns the command's exit code. Output goes to both log file and console.
log_cmd() {
    local label="$1"; shift
    log_info "Running: ${label}"
    log_debug "  → $*"
    local rc=0
    if [[ -n "${_LOG_FILE}" ]]; then
        "$@" >> "${_LOG_FILE}" 2>&1 || rc=$?
    else
        "$@" || rc=$?
    fi
    if [[ ${rc} -eq 0 ]]; then
        log_info "  ✅ ${label} succeeded"
    else
        log_error "  ❌ ${label} failed (exit ${rc})"
    fi
    return ${rc}
}

# ── Get current log file path ────────────────────────────────────────────────
log_file() {
    echo "${_LOG_FILE}"
}

# ── Finish logging — print summary ───────────────────────────────────────────
log_finish() {
    local elapsed=$(( SECONDS - _LOG_START_SECONDS ))
    local status="SUCCESS"
    [[ ${_LOG_ERROR_COUNT} -gt 0 ]] && status="FAILED"

    {
        echo ""
        echo "============================================================"
        echo "  Finished: $(date '+%Y-%m-%d %H:%M:%S')"
        echo "  Elapsed:  ${elapsed}s"
        echo "  Errors:   ${_LOG_ERROR_COUNT}"
        echo "  Warnings: ${_LOG_WARN_COUNT}"
        echo "  Status:   ${status}"
        echo "============================================================"
    } >> "${_LOG_FILE}"

    if [[ "${LOG_CONSOLE}" == "true" ]]; then
        echo ""
        echo -e "${_LOG_BOLD}Log: ${_LOG_FILE}${_LOG_NC}"
        if [[ ${_LOG_ERROR_COUNT} -gt 0 ]]; then
            echo -e "${_LOG_RED}${_LOG_BOLD}  ${_LOG_ERROR_COUNT} error(s) occurred — check log for details${_LOG_NC}"
        fi
    fi
}

# ── Log rotation ─────────────────────────────────────────────────────────────
_log_rotate() {
    if [[ "${LOG_RETENTION}" -gt 0 && -d "${LOG_DIR}" ]]; then
        find "${LOG_DIR}" -maxdepth 1 -name "*.log" -mtime "+${LOG_RETENTION}" -delete 2>/dev/null || true
    fi
}

# ── Dump system diagnostics to log ───────────────────────────────────────────
# Useful when investigating hangs or failures.
log_diagnostics() {
    log_section "System Diagnostics"
    {
        echo "--- date ---"
        date 2>/dev/null || true
        echo "--- uname -a ---"
        uname -a 2>/dev/null || true
        echo "--- env (filtered) ---"
        env | grep -iE '^(PATH|CXX|CC|CMAKE|MSYS|MINGW|VCINSTALL|VS|PROGRAMFILES|OS|COMSPEC|HOME|USER|HOSTNAME)' 2>/dev/null || true
        echo "--- disk usage ---"
        df -h . 2>/dev/null || true
        echo "--- memory ---"
        free -h 2>/dev/null || cat /proc/meminfo 2>/dev/null | head -5 || true
        echo "--- running processes (top 20 by CPU) ---"
        ps aux --sort=-%cpu 2>/dev/null | head -20 || tasklist 2>/dev/null | head -20 || true
    } >> "${_LOG_FILE}" 2>&1
}
