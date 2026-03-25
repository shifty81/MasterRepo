#!/usr/bin/env bash
# =============================================================================
# submit_issue.sh — Submit GitHub Issues with Build Logs
#
# Creates a GitHub issue containing build failure or hang diagnostic
# information.  Uses the `gh` CLI if available, otherwise generates a
# ready-to-paste Markdown file.
#
# Usage:
#   ./Scripts/Tools/submit_issue.sh [OPTIONS]
#
# Options:
#   --log FILE              Attach a build log file
#   --from-hang-report FILE Use a watchdog hang report as the issue body
#   --title TEXT            Custom issue title (auto-generated if omitted)
#   --labels L1,L2          Comma-separated labels (default: bug,build)
#   --dry-run               Generate the issue body without submitting
#   --help                  Show this message
#
# Examples:
#   # Submit latest build failure log
#   ./Scripts/Tools/submit_issue.sh --log Logs/Build/build_all_20260325.log
#
#   # Submit from a watchdog hang report
#   ./Scripts/Tools/submit_issue.sh --from-hang-report Logs/Build/hang_report_20260325.md
#
#   # Dry-run: just generate the issue file
#   ./Scripts/Tools/submit_issue.sh --log Logs/Build/build_debug.log --dry-run
#
# Exit codes:
#   0 — issue created (or dry-run file generated)
#   1 — configuration error
#   2 — gh CLI not available (issue body saved to file)
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="${REPO_ROOT}/Logs/Build"

# ── Colours ───────────────────────────────────────────────────────────────────
GREEN=$'\033[0;32m'
YELLOW=$'\033[1;33m'
RED=$'\033[0;31m'
BOLD=$'\033[1m'
NC=$'\033[0m'

info()  { echo -e "${GREEN}[submit]${NC} $*"; }
warn()  { echo -e "${YELLOW}[submit]${NC} $*"; }
error() { echo -e "${RED}[submit]${NC} $*" >&2; }

# ── Defaults ──────────────────────────────────────────────────────────────────
LOG_FILE=""
HANG_REPORT=""
TITLE=""
LABELS="bug,build"
DRY_RUN=false

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --log)              LOG_FILE="$2"; shift ;;
        --from-hang-report) HANG_REPORT="$2"; shift ;;
        --title)            TITLE="$2"; shift ;;
        --labels)           LABELS="$2"; shift ;;
        --dry-run)          DRY_RUN=true ;;
        --help|-h)
            cat <<EOF
Usage: $(basename "$0") [OPTIONS]

  --log FILE              Build log to include in the issue
  --from-hang-report FILE Watchdog hang report to use as issue body
  --title TEXT            Custom issue title
  --labels L1,L2          Issue labels (default: bug,build)
  --dry-run               Generate file without submitting
  --help                  Show this message
EOF
            exit 0 ;;
        *) warn "Unknown option: $1" ;;
    esac
    shift
done

# ── Validate inputs ──────────────────────────────────────────────────────────
if [[ -z "${LOG_FILE}" && -z "${HANG_REPORT}" ]]; then
    # Auto-detect: look for the most recent build log or hang report
    HANG_REPORT="$(ls -t "${LOG_DIR}"/hang_report_*.md 2>/dev/null | head -1 || true)"
    if [[ -z "${HANG_REPORT}" ]]; then
        LOG_FILE="$(ls -t "${LOG_DIR}"/*.log 2>/dev/null | head -1 || true)"
    fi

    if [[ -z "${LOG_FILE}" && -z "${HANG_REPORT}" ]]; then
        error "No log file or hang report found."
        error "Use --log FILE or --from-hang-report FILE"
        exit 1
    fi
fi

# ── Build issue body ─────────────────────────────────────────────────────────
ISSUE_BODY=""
ISSUE_FILE="${LOG_DIR}/issue_${TIMESTAMP}.md"

if [[ -n "${HANG_REPORT}" && -f "${HANG_REPORT}" ]]; then
    # Use hang report directly
    if [[ -z "${TITLE}" ]]; then
        TITLE="[Build Hang] Build stalled — $(date '+%Y-%m-%d %H:%M')"
    fi
    ISSUE_BODY="$(cat "${HANG_REPORT}")"

elif [[ -n "${LOG_FILE}" && -f "${LOG_FILE}" ]]; then
    # Build an issue from a log file
    if [[ -z "${TITLE}" ]]; then
        TITLE="[Build Failure] Build failed — $(date '+%Y-%m-%d %H:%M')"
    fi

    # Extract error lines from the log
    error_lines=""
    error_lines="$(grep -iE '(error[^(]*:|FAILED|fatal error|cannot find|undefined reference)' \
                    "${LOG_FILE}" | head -50 || true)"

    ISSUE_BODY="$(cat <<BODY
# Build Failure Report

**Date:** $(date '+%Y-%m-%d %H:%M:%S')
**Host:** $(hostname 2>/dev/null || echo unknown)
**OS:** Windows (Git Bash / MSYS2)

## Error Summary

\`\`\`
${error_lines:-No specific error lines found — see full log below.}
\`\`\`

## Last 200 Lines of Build Log

\`\`\`
$(tail -200 "${LOG_FILE}" 2>/dev/null || echo "(could not read log)")
\`\`\`

## Environment

\`\`\`
$(env | grep -iE '^(PATH|CXX|CC|CMAKE|MSYS|MINGW|VS|OS|COMSPEC)' 2>/dev/null | sort || true)
\`\`\`

## System Info

\`\`\`
$(cmd.exe /c "ver" 2>/dev/null || true)
$(wmic OS get FreePhysicalMemory,TotalVisibleMemorySize /Value 2>/dev/null || true)
$(df -h . 2>/dev/null || true)
\`\`\`

---
*Auto-generated by \`Scripts/Tools/submit_issue.sh\`*
BODY
)"
else
    error "Specified file not found: ${LOG_FILE:-${HANG_REPORT}}"
    exit 1
fi

# ── Write issue body to file ─────────────────────────────────────────────────
echo "${ISSUE_BODY}" > "${ISSUE_FILE}"
info "Issue body saved to: ${ISSUE_FILE}"

# ── Submit or dry-run ─────────────────────────────────────────────────────────
if [[ "${DRY_RUN}" == "true" ]]; then
    info "Dry-run: issue body written to ${ISSUE_FILE}"
    info "Title: ${TITLE}"
    info "Labels: ${LABELS}"
    echo ""
    echo "To submit manually, run:"
    echo "  gh issue create --title \"${TITLE}\" --label \"${LABELS}\" --body-file \"${ISSUE_FILE}\""
    exit 0
fi

# Try to submit via gh CLI
if command -v gh &>/dev/null; then
    info "Submitting issue via gh CLI..."

    # Check if we're in a git repo and gh is authenticated
    if gh auth status &>/dev/null 2>&1; then
        if gh issue create \
            --title "${TITLE}" \
            --label "${LABELS}" \
            --body-file "${ISSUE_FILE}" 2>/dev/null; then
            info "✅ Issue created successfully"
            exit 0
        else
            warn "gh issue create failed — issue body saved to ${ISSUE_FILE}"
            exit 2
        fi
    else
        warn "gh CLI not authenticated. Run 'gh auth login' first."
        warn "Issue body saved to: ${ISSUE_FILE}"
        echo ""
        echo "To submit manually:"
        echo "  1. Run: gh auth login"
        echo "  2. Run: gh issue create --title \"${TITLE}\" --label \"${LABELS}\" --body-file \"${ISSUE_FILE}\""
        exit 2
    fi
else
    warn "gh CLI not installed. Issue body saved to: ${ISSUE_FILE}"
    echo ""
    echo "To submit this issue:"
    echo "  Option A: Install gh CLI (https://cli.github.com) and run:"
    echo "    gh issue create --title \"${TITLE}\" --label \"${LABELS}\" --body-file \"${ISSUE_FILE}\""
    echo ""
    echo "  Option B: Copy the contents of ${ISSUE_FILE} and create an issue manually"
    echo "    on GitHub at your repository's Issues page."
    exit 2
fi
