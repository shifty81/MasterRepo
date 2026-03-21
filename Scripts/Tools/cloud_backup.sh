#!/usr/bin/env bash
# =============================================================================
# Atlas Engine — cloud_backup.sh  (Phase 10.8 — Cloud Backup)
#
# Backs up the MasterRepo project to a configurable remote target using rsync
# (over SSH) or rclone (for S3 / B2 / GDrive / OneDrive).  Designed for
# offline-first use: when the remote is unreachable the script queues a
# local "pending" marker and exits cleanly without blocking the build pipeline.
#
# Usage:
#   ./Scripts/Tools/cloud_backup.sh [OPTIONS]
#
# Options:
#   --target URL     Remote destination (rsync URL or rclone remote:bucket/path)
#   --backend rsync|rclone   Backend to use (default: rsync)
#   --exclude FILE   Extra exclude pattern (repeatable)
#   --dry-run        Print what would be transferred without doing it
#   --no-compress    Disable compression
#   --config FILE    Load settings from config file (JSON-like key=value)
#   --help           Show this message
#
# Config file example (Config/backup.cfg):
#   TARGET=myserver:/backups/MasterRepo
#   BACKEND=rsync
#   SSH_KEY=~/.ssh/atlas_backup_key
#   COMPRESS=true
#   RETENTION_DAYS=30
#
# Exit codes:
#   0 — backup succeeded
#   1 — configuration error
#   2 — transfer error (queued for retry)
#   3 — dependency missing
# =============================================================================
set -euo pipefail

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

info()    { echo -e "${GREEN}[BACKUP]${NC} $*"; }
warn()    { echo -e "${YELLOW}[BACKUP]${NC} $*"; }
error()   { echo -e "${RED}[BACKUP ERROR]${NC} $*" >&2; }
section() { echo -e "\n${CYAN}${BOLD}── $* ──${NC}\n"; }

# ── Paths ─────────────────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${REPO_ROOT}/Logs/Backup"
PENDING_FILE="${LOG_DIR}/pending_backup.flag"
CONFIG_FILE="${REPO_ROOT}/Config/backup.cfg"

mkdir -p "${LOG_DIR}"

# ── Defaults ─────────────────────────────────────────────────────────────────
TARGET=""
BACKEND="rsync"
SSH_KEY=""
COMPRESS=true
DRY_RUN=false
RETENTION_DAYS=30
EXTRA_EXCLUDES=()

# ── Load config file ──────────────────────────────────────────────────────────
load_config() {
    local cfg="${1:-${CONFIG_FILE}}"
    [[ ! -f "${cfg}" ]] && return 0
    while IFS='=' read -r key val; do
        # Skip comments and blank lines
        [[ "${key}" =~ ^[[:space:]]*# ]] && continue
        [[ -z "${key}" ]] && continue
        val="${val%%#*}"          # strip inline comments
        # Strip outer pair of double-quotes only (preserve internal quotes)
        if [[ "${val:0:1}" == '"' && "${val: -1}" == '"' ]]; then
            val="${val:1:${#val}-2}"
        fi
        val="${val#"${val%%[![:space:]]*}"}"  # ltrim
        val="${val%"${val##*[![:space:]]}"}"  # rtrim
        case "${key}" in
            TARGET)          TARGET="${val}" ;;
            BACKEND)         BACKEND="${val}" ;;
            SSH_KEY)         SSH_KEY="${val}" ;;
            COMPRESS)        COMPRESS="${val}" ;;
            RETENTION_DAYS)  RETENTION_DAYS="${val}" ;;
        esac
    done < "${cfg}"
    info "Loaded config from ${cfg}"
}

# ── Argument parsing ──────────────────────────────────────────────────────────
load_config

while [[ $# -gt 0 ]]; do
    case "$1" in
        --target)      TARGET="$2";         shift ;;
        --backend)     BACKEND="$2";        shift ;;
        --exclude)     EXTRA_EXCLUDES+=("$2"); shift ;;
        --dry-run)     DRY_RUN=true ;;
        --no-compress) COMPRESS=false ;;
        --config)      load_config "$2";    shift ;;
        --help|-h)
            cat <<EOF
Usage: $(basename "$0") [OPTIONS]

  --target URL         rsync URL or rclone remote:path
  --backend rsync|rclone
  --exclude PATTERN    Extra exclude (can repeat)
  --dry-run            Simulate without transferring
  --no-compress        Disable compression
  --config FILE        Load key=value config file
  --help               Show this message

Example:
  $(basename "$0") --target user@host:/backups/MasterRepo
  $(basename "$0") --backend rclone --target myremote:atlas-backups
EOF
            exit 0 ;;
        *) warn "Unknown option: $1" ;;
    esac
    shift
done

# ── Validate ─────────────────────────────────────────────────────────────────
section "Validating configuration"

if [[ -z "${TARGET}" ]]; then
    error "No backup target specified. Use --target or set TARGET in Config/backup.cfg"
    exit 1
fi

info "Target:   ${TARGET}"
info "Backend:  ${BACKEND}"
info "Compress: ${COMPRESS}"
info "Dry-run:  ${DRY_RUN}"

# Check backend binary
case "${BACKEND}" in
    rsync)
        if ! command -v rsync &>/dev/null; then
            error "rsync not found. Install it or use --backend rclone"
            exit 3
        fi
        ;;
    rclone)
        if ! command -v rclone &>/dev/null; then
            error "rclone not found. Install from https://rclone.org"
            exit 3
        fi
        ;;
    *)
        error "Unknown backend: ${BACKEND}  (use rsync or rclone)"
        exit 1
        ;;
esac

# ── Default excludes ──────────────────────────────────────────────────────────
DEFAULT_EXCLUDES=(
    ".git/"
    "Builds/"
    "External/"
    "*.o"
    "*.a"
    "*.so"
    "*.dylib"
    "*.dll"
    "CMakeFiles/"
    "CMakeCache.txt"
    "compile_commands.json"
    "*.log"
    "Logs/"
    "__pycache__/"
    "*.pyc"
    ".DS_Store"
)

# ── Build rsync exclude list ──────────────────────────────────────────────────
RSYNC_EXCL=()
for e in "${DEFAULT_EXCLUDES[@]}" "${EXTRA_EXCLUDES[@]}"; do
    RSYNC_EXCL+=("--exclude=${e}")
done

# ── Execute backup ────────────────────────────────────────────────────────────
section "Running backup"

LOG_FILE="${LOG_DIR}/backup_${TIMESTAMP}.log"
EXIT_CODE=0

run_rsync() {
    local flags=(-av --delete --stats)
    [[ "${COMPRESS}" == "true" ]] && flags+=(-z)
    [[ "${DRY_RUN}"  == "true" ]] && flags+=(--dry-run)
    [[ -n "${SSH_KEY}" ]] && flags+=(-e "ssh -i ${SSH_KEY}")

    if rsync "${flags[@]}" "${RSYNC_EXCL[@]}" \
             "${REPO_ROOT}/" "${TARGET}/" \
             2>&1 | tee "${LOG_FILE}"; then
        return 0
    else
        return 2
    fi
}

run_rclone() {
    local flags=(sync "${REPO_ROOT}" "${TARGET}")
    [[ "${DRY_RUN}"  == "true" ]] && flags+=(--dry-run)
    [[ "${COMPRESS}" == "true" ]] && flags+=(--transfers 4)
    flags+=(--progress --log-file "${LOG_FILE}")

    for e in "${DEFAULT_EXCLUDES[@]}" "${EXTRA_EXCLUDES[@]}"; do
        flags+=(--exclude "${e}")
    done

    if rclone "${flags[@]}"; then
        return 0
    else
        return 2
    fi
}

case "${BACKEND}" in
    rsync)  run_rsync  || EXIT_CODE=$? ;;
    rclone) run_rclone || EXIT_CODE=$? ;;
esac

# ── Handle result ─────────────────────────────────────────────────────────────
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    info "✅ Backup completed successfully — ${TIMESTAMP}"
    info "Log: ${LOG_FILE}"

    # Clear any pending flag
    rm -f "${PENDING_FILE}"

    # Apply retention (keep last N day-marker files)
    if [[ "${RETENTION_DAYS}" -gt 0 ]]; then
        find "${LOG_DIR}" -name "backup_*.log" -mtime "+${RETENTION_DAYS}" -delete 2>/dev/null || true
        info "Retention: kept logs for last ${RETENTION_DAYS} days"
    fi
else
    warn "⚠ Backup failed or incomplete (exit ${EXIT_CODE})"
    warn "Log: ${LOG_FILE}"

    # Queue for retry
    echo "${TIMESTAMP}: FAILED target=${TARGET} backend=${BACKEND}" >> "${PENDING_FILE}"
    warn "Queued in: ${PENDING_FILE}"
    warn "Re-run this script to retry the queued backup."

    exit "${EXIT_CODE}"
fi
